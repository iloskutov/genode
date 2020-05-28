/*
 * \brief  USB HID driver
 * \author Stefan Kalkowski
 * \date   2018-06-07
 */

/*
 * Copyright (C) 2018 Genode Labs GmbH
 *
 * This file is distributed under the terms of the GNU General Public License
 * version 2.
 */

#include <base/component.h>

#include <driver.h>
#include <lx_emul.h>

#include <lx_kit/env.h>
#include <lx_kit/malloc.h>
#include <lx_kit/scheduler.h>
#include <lx_kit/timer.h>
#include <lx_kit/work.h>

#include <lx_emul/extern_c_begin.h>
#include <linux/usb.h>
#include <linux/tty_driver.h>
#include <linux/tty.h>
#include <lx_emul/extern_c_end.h>


void Driver::Device::register_device()
{
	Genode::log(__PRETTY_FUNCTION__);
	if (udev) {
		Genode::error("device already registered!");
		return;
	}

	Usb::Device_descriptor dev_desc;
	Usb::Config_descriptor config_desc;
	usb.config_descriptor(&dev_desc, &config_desc);

	udev = (usb_device*) kzalloc(sizeof(usb_device), GFP_LX_DMA);
	udev->bus = (usb_bus*) kzalloc(sizeof(usb_bus), GFP_LX_DMA);
	udev->config = (usb_host_config*) kzalloc(sizeof(usb_host_config), GFP_LX_DMA);
	udev->bus->bus_name = "usbbus";
	udev->bus->controller = (device*) (&usb);
	udev->bus_mA = 900; /* set to maximum USB3.0 */

	Genode::memcpy(&udev->descriptor,   &dev_desc,    sizeof(usb_device_descriptor));
	Genode::memcpy(&udev->config->desc, &config_desc, sizeof(usb_config_descriptor));
	udev->devnum = dev_desc.num;
	udev->speed  = (usb_device_speed) dev_desc.speed;
	udev->authorized = 1;

	int cfg = usb_get_configuration(udev);
	if (cfg < 0) {
		Genode::error("usb_get_configuration returned error ", cfg);
		return;
	}

	cfg = usb_choose_configuration(udev);
	usb_set_configuration(udev, cfg);

	for (int i = 0; i < udev->config->desc.bNumInterfaces; i++) {
		Genode::log("iface: ", i);

		struct usb_interface      * iface = udev->config->interface[i];
		struct usb_host_interface * alt   = iface->cur_altsetting;

		for (int j = 0; j < alt->desc.bNumEndpoints; ++j) {
			Genode::log("alt ep: ", j);
			struct usb_host_endpoint * ep = &alt->endpoint[j];
			int epnum  = usb_endpoint_num(&ep->desc);
			int is_out = usb_endpoint_dir_out(&ep->desc);
			if (is_out) udev->ep_out[epnum] = ep;
			else        udev->ep_in[epnum]  = ep;
		}

		struct usb_device_id   id;
		probe_interface(iface, &id);
	}
}


void Driver::Device::unregister_device()
{
	Genode::log(__PRETTY_FUNCTION__);
	for (unsigned i = 0; i < USB_MAXINTERFACES; i++) {
		if (!udev->config->interface[i]) break;
		else remove_interface(udev->config->interface[i]);
	}
	kfree(udev->bus);
	kfree(udev->config);
	kfree(udev);
	udev = nullptr;
}


void Driver::Device::state_task_entry(void * arg)
{
	Device & dev = *reinterpret_cast<Device*>(arg);

	for (;;) {
		if (dev.usb.plugged() && !dev.udev)
			dev.register_device();

		if (!dev.usb.plugged() && dev.udev)
			dev.unregister_device();

		Lx::scheduler().current()->block_and_schedule();
	}
}


void Driver::Device::urb_task_entry(void * arg)
{
	Device & dev = *reinterpret_cast<Device*>(arg);

	for (;;) {

		while (dev.udev && dev.usb.source()->ack_avail()) {
			Usb::Packet_descriptor p = dev.usb.source()->get_acked_packet();
			if (p.completion) {
				p.completion->complete(p);
			}
			dev.usb.source()->release_packet(p);
		}

		Lx::scheduler().current()->block_and_schedule();
	}
}


Driver::Device::Device(Driver & driver, Label label)
: label(label),
  driver(driver),
  env(driver.env),
  alloc(driver.alloc),
  state_task(env.ep(), state_task_entry, reinterpret_cast<void*>(this),
             "usb_state", Lx::Task::PRIORITY_0, Lx::scheduler()),
  urb_task(env.ep(), urb_task_entry, reinterpret_cast<void*>(this),
             "usb_urb", Lx::Task::PRIORITY_0, Lx::scheduler())
{
	usb.tx_channel()->sigh_ack_avail(urb_task.handler);
	driver.devices.insert(&le);
}


Driver::Device::~Device()
{
	driver.devices.remove(&le);
	if (udev) unregister_device();
}


static Driver * driver = nullptr;
struct workqueue_struct *tasklet_wq;

void Driver::main_task_entry(void * arg)
{
	driver = reinterpret_cast<Driver*>(arg);

	tasklet_wq = alloc_workqueue("tasklet_wq", 0, 0);

	module_usb_serial_init();
	module_usb_serial_module_init_ftdi_so();
	module_usb_serial_module_init_option();
	module_usb_serial_module_init_pl2303();

	for (;;) {
		driver->scan_report();
		Lx::scheduler().current()->block_and_schedule();
	}
}


void Driver::scan_report()
{
	Genode::log(__PRETTY_FUNCTION__, ":", __LINE__);

	if (!report_rom.constructed()) {
		report_rom.construct(env, "report");
		report_rom->sigh(main_task->handler);
	} else {
		report_rom->update();
	}

	devices.for_each([&] (Device & d) { d.updated = false; });

	try {
		Genode::Xml_node report_node = report_rom->xml();
		report_node.for_each_sub_node([&] (Genode::Xml_node & dev_node)
		{
			unsigned long c = 0;
			unsigned long vid = 0;
			unsigned long pid = 0;

			dev_node.attribute("class").value(c);
			dev_node.attribute("vendor_id").value(vid);
			dev_node.attribute("product_id").value(pid);
			Genode::log(Genode::Hex(vid), ":", Genode::Hex(pid), " - class: ", Genode::Hex(c));

			/* TODO */
			if (c != 0xff) return;

			Label label;
			dev_node.attribute("label").value(label);

			bool found = false;

			devices.for_each([&] (Device & d) {
				if (d.label == label) d.updated = found = true; });

			if (!found)
				new (heap) Device(*this, label);
		});
	} catch(...) {
		Genode::error("Error parsing USB devices report");
	};

	devices.for_each([&] (Device & d) {
		if (!d.updated) destroy(heap, &d); });
}


Driver::Driver(Genode::Env &env) : env(env)
{
	Genode::log("--- USB Serial driver ---");

	Lx_kit::construct_env(env);

	LX_MUTEX_INIT(table_lock);

	Lx::scheduler(&env);
	Lx::malloc_init(env, heap);
	Lx::timer(&env, &ep, &heap, &jiffies);
	Lx::Work::work_queue(&heap);

	main_task.construct(env.ep(), main_task_entry, reinterpret_cast<void*>(this),
	                    "main", Lx::Task::PRIORITY_0, Lx::scheduler());

	/* give all task a first kick before returning */
	Lx::scheduler().schedule();
}


void Component::construct(Genode::Env &env)
{
	env.exec_static_constructors();
	static Driver driver(env);
}

Uart::Driver & Uart::Driver_factory::create(unsigned index, unsigned baudrate,
                                            Uart::Char_avail_functor &functor,
											tty_device *dev)
{
	if (index >= UARTS_NUM) throw Not_available();

	if (!drivers[index])
		drivers[index] = new (&heap) Driver(env, index, baudrate, functor, dev);
	return *drivers[index];
}


Uart::Driver::Driver(Genode::Env &env, unsigned, unsigned,
		       Uart::Char_avail_functor &func,
			   tty_device *dev)
: _char_avail(func),
	_timer(env),
	_timer_handler(env.ep(), *this, &Driver::_timeout),
	_ttydev(dev),
	_tx_data(dev)
{
	Genode::log(__PRETTY_FUNCTION__, ":", __LINE__);

	_timer.sigh(_timer_handler);
	_timer.trigger_periodic(10*1000);

	_ttydev->_rx_buf = (void*) &_rx_buf;
}

static void tty_put_char(tty_device *d, char c)
{
	if (d) {
		if (d->tty) {
			if (d->tty->ops) {
				if (d->tty->ops->write) {
					d->tty->ops->write(d->tty, (const unsigned char *) &c, 1);
				} else {
					Genode::error(__PRETTY_FUNCTION__, ": empty tty_operations write");
				}
			} else {
				Genode::error(__PRETTY_FUNCTION__, ": empty tty_operations");
			}
		} else {
			Genode::error(__PRETTY_FUNCTION__, ": empty tty_struct");
		}
	} else {
		Genode::error(__PRETTY_FUNCTION__, ": empty ttydev");
	}
}


void Uart::Driver::_run_put_task(void * args)
{
	Xmit_data  *data = static_cast<Xmit_data*>(args);

	while (1) {
		Lx::scheduler().current()->block_and_schedule();

		// TODO: set some limits??
		while (!data->buf.empty())
			tty_put_char(data->ttydev, data->buf.get());
	}
}

void Uart::Driver::put_char(char c) {
	// save data and put it to put_task ...
	// fifo ???
	_tx_data.buf.add(c);
	_put_task.unblock();
	Lx::scheduler().schedule();
}

bool Uart::Driver::char_avail()
{
	return !_rx_buf.empty();
}

char Uart::Driver::get_char()
{
	return _rx_buf.get();
}

void Uart::Driver::baud_rate(int) { }