
#include <base/env.h>
#include <base/registry.h>
#include <usb_session/client.h>
#include <util/bit_allocator.h>

#include <driver.h>
#include <lx_emul.h>

#include <lx_emul/extern_c_begin.h>
#include <linux/usb.h>
#include <uapi/asm-generic/termbits.h>
#include <uapi/asm-generic/termios.h>
#include <linux/tty_driver.h>
#include <linux/tty.h>
#include <lx_emul/extern_c_end.h>

#define TRACE do { ; } while (0)

#include <lx_emul/impl/kernel.h>
#include <lx_emul/impl/delay.h>
#include <lx_emul/impl/slab.h>
#include <lx_emul/impl/work.h>
#include <lx_emul/impl/spinlock.h>
#include <lx_emul/impl/mutex.h>
#include <lx_emul/impl/sched.h>
#include <lx_emul/impl/timer.h>
#include <lx_emul/impl/completion.h>
#include <lx_emul/impl/wait.h>
#include <lx_emul/impl/usb.h>
#include <lx_emul/impl/gfp.h>

#include <lx_kit/backend_alloc.h>


#define TRACE_AND_STOP \
	do { \
		lx_printf("%s not implemented\n", __func__); \
		BUG(); \
	} while (0)


extern "C" int usb_match_device(struct usb_device *dev,
                                const struct usb_device_id *id)
{
	if ((id->match_flags & USB_DEVICE_ID_MATCH_VENDOR) &&
	    id->idVendor != le16_to_cpu(dev->descriptor.idVendor))
		return 0;

	if ((id->match_flags & USB_DEVICE_ID_MATCH_PRODUCT) &&
	    id->idProduct != le16_to_cpu(dev->descriptor.idProduct))
		return 0;

	if ((id->match_flags & USB_DEVICE_ID_MATCH_DEV_LO) &&
	    (id->bcdDevice_lo > le16_to_cpu(dev->descriptor.bcdDevice)))
		return 0;

	if ((id->match_flags & USB_DEVICE_ID_MATCH_DEV_HI) &&
	    (id->bcdDevice_hi < le16_to_cpu(dev->descriptor.bcdDevice)))
		return 0;

	if ((id->match_flags & USB_DEVICE_ID_MATCH_DEV_CLASS) &&
	    (id->bDeviceClass != dev->descriptor.bDeviceClass))
		return 0;

	if ((id->match_flags & USB_DEVICE_ID_MATCH_DEV_SUBCLASS) &&
	    (id->bDeviceSubClass != dev->descriptor.bDeviceSubClass))
		return 0;

	if ((id->match_flags & USB_DEVICE_ID_MATCH_DEV_PROTOCOL) &&
	    (id->bDeviceProtocol != dev->descriptor.bDeviceProtocol))
		return 0;

	return 1;
}


extern "C" int usb_match_one_id_intf(struct usb_device *dev,
                                     struct usb_host_interface *intf,
                                     const struct usb_device_id *id)
{
	if (dev->descriptor.bDeviceClass == USB_CLASS_VENDOR_SPEC &&
	    !(id->match_flags & USB_DEVICE_ID_MATCH_VENDOR) &&
	    (id->match_flags & (USB_DEVICE_ID_MATCH_INT_CLASS |
	                        USB_DEVICE_ID_MATCH_INT_SUBCLASS |
	                        USB_DEVICE_ID_MATCH_INT_PROTOCOL |
	                        USB_DEVICE_ID_MATCH_INT_NUMBER)))
		return 0;

	if ((id->match_flags & USB_DEVICE_ID_MATCH_INT_CLASS) &&
	    (id->bInterfaceClass != intf->desc.bInterfaceClass))
		return 0;

	if ((id->match_flags & USB_DEVICE_ID_MATCH_INT_SUBCLASS) &&
	    (id->bInterfaceSubClass != intf->desc.bInterfaceSubClass))
		return 0;

	if ((id->match_flags & USB_DEVICE_ID_MATCH_INT_PROTOCOL) &&
	    (id->bInterfaceProtocol != intf->desc.bInterfaceProtocol))
		return 0;

	if ((id->match_flags & USB_DEVICE_ID_MATCH_INT_NUMBER) &&
	    (id->bInterfaceNumber != intf->desc.bInterfaceNumber))
		return 0;

	return 1;
}

struct Lx_driver
{
	using Element = Genode::List_element<Lx_driver>;
	using List    = Genode::List<Element>;

	usb_driver & drv;
	Element      le { this };

	Lx_driver(usb_driver & drv) : drv(drv) { list().insert(&le); }

	usb_device_id * match(usb_interface * iface)
	{
		struct usb_device_id * id = const_cast<usb_device_id*>(drv.id_table);
		for (; id->idVendor || id->idProduct || id->bDeviceClass ||
		       id->bInterfaceClass || id->driver_info; id++)
			if (usb_match_one_id(iface, id)) return id;
		return nullptr;
	}

	int probe(usb_interface * iface, usb_device_id * id)
	{
		iface->dev.driver = &drv.drvwrap.driver;
		if (drv.probe) return drv.probe(iface, id);
		return 0;
	}

	static List & list()
	{
		static List _list;
		return _list;
	}
};


struct Lx_dev_driver
{
	using Element = Genode::List_element<Lx_dev_driver>;
	using List    = Genode::List<Element>;

	device_driver & dev_drv;
	Element         le { this };

	Lx_dev_driver(device_driver & drv) : dev_drv(drv) { list().insert(&le); }

	bool match(struct device *dev) {
		return dev_drv.bus->match ? dev_drv.bus->match(dev, &dev_drv)
		                          : false; }

	int probe(struct device *dev)
	{
		Genode::log(__PRETTY_FUNCTION__);
		dev->driver = &dev_drv;
		if (dev_drv.bus->probe) return dev_drv.bus->probe(dev);
		return 0;
	}

	static List & list()
	{
		static List _list;
		return _list;
	}
};

struct task_struct *current;
struct workqueue_struct *system_wq;
unsigned long jiffies;

Genode::Ram_dataspace_capability Lx::backend_alloc(Genode::addr_t size, Genode::Cache_attribute cached) {
	return Lx_kit::env().env().ram().alloc(size, cached); }

void Lx::backend_free(Genode::Ram_dataspace_capability cap) {
	return Lx_kit::env().env().ram().free(cap); }

const char *dev_name(const struct device *dev) { return dev->name; }

size_t strlen(const char *s) { return Genode::strlen(s); }

int  mutex_lock_interruptible(struct mutex *m)
{
	mutex_lock(m);
	return 0;
}

int driver_register(struct device_driver *drv)
{
	if (drv) new (Lx::Malloc::mem()) Lx_dev_driver(*drv);
	return 0;
}

int usb_register_driver(struct usb_driver * driver, struct module *, const char *)
{
	INIT_LIST_HEAD(&driver->dynids.list);
	if (driver) new (Lx::Malloc::mem()) Lx_driver(*driver);
	return 0;
}

namespace Usb_serial {
	struct DeviceBusy : Genode::Exception { };
	struct OutOfMemory : Genode::Exception { };
};

static tty_device * single_tty_device = nullptr;



extern "C" struct device *tty_register_device(struct tty_driver *driver,
					  unsigned index, struct device *device)
{
	struct device *dev;
	dev_t devt = MKDEV(driver->major, driver->minor_start) + index;
	char name[64];

	Genode::log(__PRETTY_FUNCTION__);

	dev = (struct device *) kzalloc(sizeof(*dev), GFP_LX_DMA);
	if (!dev)
		return (struct device *) ERR_PTR(-ENOMEM);


	sprintf(name, "%s%d", driver->name, index + driver->name_base);

	dev->devt = devt;
	// dev->class = tty_class;
	dev->parent = device;
	// dev->release = tty_device_create_release;
	dev_set_name(dev, "%s", name);
	// dev->groups = attr_grp;
	dev_set_drvdata(dev, NULL);

	// dev_set_uevent_suppress(dev, 1);

	// retval = device_register(dev);
	// if (retval)
	// 	goto err_put;
	if (single_tty_device)
		throw Usb_serial::DeviceBusy();

	Genode::log(__PRETTY_FUNCTION__, ": register device ", (const char*) name);

	// TODO: register somewhere in Driver class
	single_tty_device = (struct tty_device *) kzalloc(sizeof(*single_tty_device), GFP_LX_DMA);
	if (!single_tty_device)
		throw Usb_serial::OutOfMemory();

	single_tty_device->dev = dev;
	single_tty_device->index = index;
	single_tty_device->driver = driver;

	return dev;
}

tty_device * Uart::Session_component::_register_session_component(Uart::Session_component & s)
{
	if (single_tty_device) single_tty_device->session_component = (void*) &s;
	return single_tty_device;
}

void tty_port_tty_set(struct tty_port *port, struct tty_struct *tty)
{
	port->tty = tty;
}

struct tty_struct *tty_port_tty_get(struct tty_port *port)
{
	return port->tty;
}

extern "C" int tty_insert_flip_string_fixed_flag(struct tty_port *port,
		const unsigned char *chars, char flag, size_t size)
{
	int copied = 0;
	if (single_tty_device && size) {
		Uart::Driver::Ring_buffer *buf = (Uart::Driver::Ring_buffer *) single_tty_device->_rx_buf;
		const unsigned char *p = chars;
		// TODO: flags ???
		while (size--) {
			buf->add(*p);
			p++;
			copied++;
		}
	}
	return copied;
}


int tty_port_open(struct tty_port *port, struct tty_struct *tty, struct file *filp)
{
	++port->count;
	tty_port_tty_set(port, tty);

	/*
	 * Do the device-specific open only if the hardware isn't
	 * already initialized. Serialize open and shutdown using the
	 * port mutex.
	 */

	if (!tty_port_initialized(port)) {
		clear_bit(TTY_IO_ERROR, &tty->flags);
		if (port->ops->activate) {
			int retval = port->ops->activate(port, tty);
			if (retval) {
				return retval;
			}
		}
		tty_port_set_initialized(port, 1);
	}

	return 0;
}



void Driver::Device::probe_interface(usb_interface * iface, usb_device_id * id)
{
	int rc = 1;
	Genode::log(__PRETTY_FUNCTION__);
	// Genode::log("serial_driver: ", serial_driver);

	using Le = Genode::List_element<Lx_driver>;
	for (Le *le = Lx_driver::list().first(); le; le = le->next()) {
		usb_device_id * id = le->object()->match(iface);

		if (id && le->object()->probe(iface, id) == 0) {
			rc =0;
			break;
		}
	}

	if (rc != 0) {
		Genode::log(__PRETTY_FUNCTION__, ":", __LINE__, " rc: ", rc);
		throw Genode::Exception();
	}

	// TODO ???
	// iface->dev.driver = &serial_driver->drvwrap.driver;
	// int rc = serial_driver->probe(iface, id);

	Genode::log(__PRETTY_FUNCTION__, ":", __LINE__, " rc: ", rc);

	if (rc == 0 && single_tty_device) {

		Genode::log(__PRETTY_FUNCTION__, ":", __LINE__);

		struct tty_struct * tty = tty_init_dev(single_tty_device->driver, single_tty_device->index);

		Genode::log(__PRETTY_FUNCTION__, ":", __LINE__);

		if (tty->ops && tty->ops->open) {
			rc = tty->ops->open(tty, NULL);
			if (rc) {
				Genode::error(__PRETTY_FUNCTION__, ":", __LINE__);
				while(1) ;
			}
		} else {
			Genode::error(__PRETTY_FUNCTION__, ":", __LINE__);
			while(1) ;
		}

		Genode::log(__PRETTY_FUNCTION__, ":", __LINE__);

		single_tty_device->tty = tty;

		driver.env.parent().announce(driver.ep.manage(driver.root));

	} else {
		Genode::log(__PRETTY_FUNCTION__, ":", __LINE__, " rc: ", rc);
		throw Genode::Exception();
	}
}

int tty_standard_install(struct tty_driver *driver, struct tty_struct *tty)
{
	// tty_init_termios(tty);
	// tty_driver_kref_get(driver);
	tty->count++;
	driver->ttys[tty->index] = tty;
	return 0;
}

int tty_port_install(struct tty_port *port, struct tty_driver *driver, struct tty_struct *tty)
{
	tty->port = port;
	return tty_standard_install(driver, tty);;
}


void Driver::Device::remove_interface(usb_interface * iface)
{
	to_usb_driver(iface->dev.driver)->disconnect(iface);
	kfree(iface);
}


long __wait_completion(struct completion *work, unsigned long timeout)
{
	Lx::timer_update_jiffies();
	struct process_timer timer { *Lx::scheduler().current() };
	unsigned long        expire = timeout + jiffies;

	if (timeout) {
		timer_setup(&timer.timer, process_timeout, 0);
		mod_timer(&timer.timer, expire);
	}

	while (!work->done) {

		if (timeout && expire <= jiffies) return 0;

		Lx::Task * task = Lx::scheduler().current();
		work->task = (void *)task;
		task->block_and_schedule();
	}

	if (timeout) del_timer(&timer.timer);

	work->done = 0;
	return (expire > jiffies) ? (expire - jiffies) : 1;
}


struct workqueue_struct *create_singlethread_workqueue(char const *name)
{
	workqueue_struct *wq = (workqueue_struct *)kzalloc(sizeof(workqueue_struct), 0);
	Lx::Work *work = Lx::Work::alloc_work_queue(&Lx::Malloc::mem(), name);
	wq->task       = (void *)work;

	return wq;
}


struct workqueue_struct *alloc_workqueue(const char *fmt, unsigned int flags,
                                         int max_active, ...)
{
	return create_singlethread_workqueue(fmt);
}



void tasklet_schedule(struct tasklet_struct *t)
{
	Lx::Work *lx_work = (Lx::Work *)tasklet_wq->task;
	lx_work->schedule_tasklet(t);
	lx_work->unblock();
}


int dev_set_drvdata(struct device *dev, void *data)
{
	dev->driver_data = data;
	return 0;
}


void *dev_get_drvdata(const struct device *dev)
{
	return dev->driver_data;
}


size_t strlcat(char *dest, const char *src, size_t dest_size)
{
	size_t len_d = strlen(dest);

	if (len_d > dest_size) return 0;

	size_t len = dest_size - len_d - 1;

	memcpy(dest + len_d, src, len);
	dest[len_d + len] = 0;
	return len;
}


int __usb_get_extra_descriptor(char *buffer, unsigned size, unsigned char type, void **ptr)
{
	struct usb_descriptor_header *header;

	while (size >= sizeof(struct usb_descriptor_header)) {
		header = (struct usb_descriptor_header *)buffer;

		if (header->bLength < 2) {
			printk(KERN_ERR
				   "%s: bogus descriptor, type %d length %d\n",
				   usbcore_name,
				   header->bDescriptorType,
				   header->bLength);
			return -1;
		}

		if (header->bDescriptorType == type) {
			*ptr = header;
			return 0;
		}

		buffer += header->bLength;
		size -= header->bLength;
	}

	return -1;
}


void *vzalloc(unsigned long size)
{
	size_t *addr;
	try { addr = (size_t *)Lx::Malloc::mem().alloc_large(size); }
	catch (...) { return 0; }

	memset(addr, 0, size);

	return addr;
}


void vfree(void *addr)
{
	if (!addr) return;
	Lx::Malloc::mem().free_large(addr);
}


int device_add(struct device *dev)
{
	if (dev->driver) return 0;

	/* foreach driver match and probe device */
	using Le = Genode::List_element<Lx_dev_driver>;
	for (Le *le = Lx_dev_driver::list().first(); le; le = le->next())
		if (le->object()->match(dev)) {
			int ret = le->object()->probe(dev);
			if (!ret) return 0;
		}

	return 0;
}


void device_del(struct device *dev)
{
	if (dev->bus && dev->bus->remove)
		dev->bus->remove(dev);
}


void *usb_alloc_coherent(struct usb_device *dev, size_t size, gfp_t mem_flags, dma_addr_t *dma)
{
	return kmalloc(size, GFP_LX_DMA);
}


struct device *get_device(struct device *dev)
{
	//dev->ref++;
	return dev;
}


void cdev_init(struct cdev *c, const struct file_operations *fops)
{
	c->ops = fops;
}


void usb_free_coherent(struct usb_device *dev, size_t size, void *addr, dma_addr_t dma)
{
	//kfree(dev);
}


int  mutex_lock_killable(struct mutex *lock)
{
	mutex_lock(lock);
	return 0;
}


u16 get_unaligned_le16(const void *p)
{
	const struct __una_u16 *ptr = (const struct __una_u16 *)p;
	return ptr->x;
}


unsigned long find_next_bit(const unsigned long *addr, unsigned long size, unsigned long offset)
{
	unsigned long i  = offset / BITS_PER_LONG;
	offset -= (i * BITS_PER_LONG);

	for (; offset < size; offset++)
		if (addr[i] & (1UL << offset))
			return offset + (i * BITS_PER_LONG);

	return size;
}


long find_next_zero_bit_le(const void *addr,
                           unsigned long size, unsigned long offset)
{
	unsigned long max_size = sizeof(long) * 8;
	if (offset >= max_size) {
		Genode::warning("Offset greater max size");
		return offset + size;
	}

	for (; offset < max_size; offset++)
		if (!(*(unsigned long*)addr & (1L << offset)))
			return offset;

	return offset + size;
}


u32 get_unaligned_le32(const void *p)
{
	const struct __una_u32 *ptr = (const struct __una_u32 *)p;
	return ptr->x;
}


void *devm_kzalloc(struct device *dev, size_t size, gfp_t gfp)
{
	return kzalloc(size, gfp);
}


extern "C" int usb_get_descriptor(struct usb_device *dev, unsigned char type,
                       unsigned char index, void *buf, int size)
{
	int i;
	int result;

	memset(buf, 0, size);

	for (i = 0; i < 3; ++i) {
		/* retry on length 0 or error; some devices are flakey */
		result = usb_control_msg(dev, usb_rcvctrlpipe(dev, 0),
		                         USB_REQ_GET_DESCRIPTOR, USB_DIR_IN,
		                         (type << 8) + index, 0, buf, size,
		                         USB_CTRL_GET_TIMEOUT);
		if (result <= 0 && result != -ETIMEDOUT)
			continue;
		if (result > 1 && ((u8 *)buf)[1] != type) {
			result = -ENODATA;
			continue;
		}
		break;
	}
	return result;
}



static struct usb_interface_assoc_descriptor *find_iad(struct usb_device *dev,
                                                       struct usb_host_config *config,
                                                       u8 inum)
{
	struct usb_interface_assoc_descriptor *retval = NULL;
	struct usb_interface_assoc_descriptor *intf_assoc;
	int first_intf;
	int last_intf;
	int i;

	for (i = 0; (i < USB_MAXIADS && config->intf_assoc[i]); i++) {
		intf_assoc = config->intf_assoc[i];
		if (intf_assoc->bInterfaceCount == 0)
			continue;

		first_intf = intf_assoc->bFirstInterface;
		last_intf = first_intf + (intf_assoc->bInterfaceCount - 1);
		if (inum >= first_intf && inum <= last_intf) {
			if (!retval)
				retval = intf_assoc;
			else
				dev_err(&dev->dev, "Interface #%d referenced"
						" by multiple IADs\n", inum);
		}
	}

	return retval;
}


struct usb_host_interface *usb_altnum_to_altsetting(const struct usb_interface *intf,
                                                    unsigned int altnum)
{
	for (unsigned i = 0; i < intf->num_altsetting; i++) {
		if (intf->altsetting[i].desc.bAlternateSetting == altnum)
			return &intf->altsetting[i];
	}
	return NULL;
}


int usb_set_configuration(struct usb_device *dev, int configuration)
{
	int i, ret;
	struct usb_host_config *cp = NULL;
	struct usb_interface **new_interfaces = NULL;
	int n, nintf;

	if (dev->authorized == 0 || configuration == -1)
		configuration = 0;
	else {
		for (i = 0; i < dev->descriptor.bNumConfigurations; i++) {
			if (dev->config[i].desc.bConfigurationValue ==
				configuration) {
				cp = &dev->config[i];
				break;
			}
		}
	}
	if ((!cp && configuration != 0))
		return -EINVAL;

	/* The USB spec says configuration 0 means unconfigured.
	 * But if a device includes a configuration numbered 0,
	 * we will accept it as a correctly configured state.
	 * Use -1 if you really want to unconfigure the device.
	 */
	if (cp && configuration == 0)
		dev_warn(&dev->dev, "config 0 descriptor??\n");

	/* Allocate memory for new interfaces before doing anything else,
	 * so that if we run out then nothing will have changed. */
	n = nintf = 0;
	if (cp) {
		nintf = cp->desc.bNumInterfaces;
		new_interfaces = (struct usb_interface **)
			kmalloc(nintf * sizeof(*new_interfaces), GFP_LX_DMA);
		if (!new_interfaces)
			return -ENOMEM;

		for (; n < nintf; ++n) {
			new_interfaces[n] = (struct usb_interface*)
				kzalloc( sizeof(struct usb_interface), GFP_LX_DMA);
			if (!new_interfaces[n]) {
				ret = -ENOMEM;
				while (--n >= 0)
					kfree(new_interfaces[n]);
				kfree(new_interfaces);
				return ret;
			}
		}
	}

	/*
	 * Initialize the new interface structures and the
	 * hc/hcd/usbcore interface/endpoint state.
	 */
	for (i = 0; i < nintf; ++i) {
		struct usb_interface_cache *intfc;
		struct usb_interface *intf;
		struct usb_host_interface *alt;
		u8 ifnum;

		cp->interface[i] = intf = new_interfaces[i];
		intfc = cp->intf_cache[i];
		intf->altsetting = intfc->altsetting;
		intf->num_altsetting = intfc->num_altsetting;
		intf->authorized = 1; //FIXME

		alt = usb_altnum_to_altsetting(intf, 0);

		/* No altsetting 0?  We'll assume the first altsetting.
		 * We could use a GetInterface call, but if a device is
		 * so non-compliant that it doesn't have altsetting 0
		 * then I wouldn't trust its reply anyway.
		 */
		if (!alt)
			alt = &intf->altsetting[0];

		ifnum = alt->desc.bInterfaceNumber;
		intf->intf_assoc = find_iad(dev, cp, ifnum);
		intf->cur_altsetting = alt;
		intf->dev.parent = &dev->dev;
		intf->dev.driver = NULL;
		intf->dev.bus = (bus_type*) 0xdeadbeef /*&usb_bus_type*/;
		intf->minor = -1;
		device_initialize(&intf->dev);
		dev_set_name(&intf->dev, "%d-%s:%d.%d", dev->bus->busnum,
					 dev->devpath, configuration, ifnum);
	}
	kfree(new_interfaces);

	ret = usb_control_msg(dev, usb_sndctrlpipe(dev, 0),
						  USB_REQ_SET_CONFIGURATION, 0, configuration, 0,
						  NULL, 0, USB_CTRL_SET_TIMEOUT);
	if (ret < 0 && cp) {
		for (i = 0; i < nintf; ++i) {
			put_device(&cp->interface[i]->dev);
			cp->interface[i] = NULL;
		}
		cp = NULL;
	}

	dev->actconfig = cp;

	if (!cp) {
		dev->state = USB_STATE_ADDRESS;
		return ret;
	}
	dev->state = USB_STATE_CONFIGURED;

	return 0;
}


//----------------------------------------------------------------------------//

/*****************
 ** linux/idr.h **
 *****************/
typedef Genode::addr_t         addr_t;

struct Idr
{

	enum { INVALID_ENTRY = ~0ul, };
	enum { MAX_ENTRIES = 1024, };

	Genode::Bit_array<MAX_ENTRIES>  _barray { };
	addr_t                          _ptr[MAX_ENTRIES] { };
	void                           *_idp { nullptr };

	bool _check(addr_t index) { return index < MAX_ENTRIES ? true : false; }

	Idr(struct idr *idp) : _idp(idp) { }

	virtual ~Idr() { }

	bool handles(void *ptr) { return _idp == ptr; }

	bool set_id(addr_t index, void *ptr)
	{
		if (_barray.get(index, 1)) { return false; }

		_barray.set(index, 1);
		_ptr[index] = (uintptr_t) ptr;
		return true;
	}

	addr_t alloc(addr_t start, void *ptr)
	{
		addr_t index = INVALID_ENTRY;
		for (addr_t i = start; i < MAX_ENTRIES; i++) {
			if (_barray.get(i, 1)) { continue; }
			index = i;
			break;
		}

		if (index == INVALID_ENTRY) { return INVALID_ENTRY; }

		_barray.set(index, 1);
		_ptr[index] = (addr_t) ptr;
		return index;
	}

	void clear(addr_t index)
	{
		if (!_check(index)) { return; }

		_barray.clear(index, 1);
		_ptr[index] = 0;
	}

	addr_t next(addr_t index)
	{
		for (addr_t i = index; i < MAX_ENTRIES; i++) {
			if (_barray.get(i, 1)) { return i; }
		}
		return INVALID_ENTRY;
	}

	void *get_ptr(addr_t index)
	{
		if (!_check(index)) { return NULL; }
		return (void*)_ptr[index];
	}
};


static Genode::Registry<Genode::Registered<Idr>> _idr_registry;


static Idr &idp_to_idr(struct idr *idp)
{
	Idr *idr = nullptr;
	auto lookup = [&](Idr &i) {
		if (i.handles(idp)) { idr = &i; }
	};
	_idr_registry.for_each(lookup);

	if (!idr) {
		Genode::Registered<Idr> *i = new (&Lx_kit::env().heap())
			Genode::Registered<Idr>(_idr_registry, idp);
		idr = &*i;
	}

	return *idr;
}


int idr_alloc(struct idr *idp, void *ptr, int start, int end, gfp_t gfp_mask)
{
	Idr &idr = idp_to_idr(idp);

	if ((end - start) > 1) {
		addr_t const id = idr.alloc(start, ptr);
		return id != Idr::INVALID_ENTRY ? id : -1;
	} else {
		if (idr.set_id(start, ptr)) { return start; }
	}

	return -1;
}


void *idr_find(struct idr *idp, int id)
{
	Idr &idr = idp_to_idr(idp);
	return idr.get_ptr(id);
}


void *idr_get_next(struct idr *idp, int *nextid)
{
	Idr &idr = idp_to_idr(idp);
	addr_t i = idr.next(*nextid);
	if (i == Idr::INVALID_ENTRY) { return NULL; }

	*nextid = i;
	return idr.get_ptr(i);
}

void *idr_remove(struct idr *idp, unsigned long id)
{
	Idr &idr = idp_to_idr(idp);
	idr.clear(id);
	return NULL;
}



struct tty_driver *__tty_alloc_driver(unsigned int lines,
		struct module *owner, unsigned long flags)
{
	struct tty_driver *driver;
	unsigned int cdevs = 1;
	int err;

	driver = (struct tty_driver *) kzalloc(sizeof(struct tty_driver), GFP_LX_DMA);
	if (!driver)
		return (struct tty_driver *) ERR_PTR(-ENOMEM);

	kref_init(&driver->kref);
	driver->magic = TTY_DRIVER_MAGIC;
	driver->num = lines;
	driver->owner = owner;
	driver->flags = flags;

	if (!(flags & TTY_DRIVER_DEVPTS_MEM)) {
		driver->ttys = (struct tty_struct **) kcalloc(lines, sizeof(*driver->ttys),
				GFP_LX_DMA);
		driver->termios = (struct ktermios **) kcalloc(lines, sizeof(*driver->termios),
				GFP_LX_DMA);
		if (!driver->ttys || !driver->termios) {
			err = -ENOMEM;
			goto err_free_all;
		}
	}

	if (!(flags & TTY_DRIVER_DYNAMIC_ALLOC)) {
		driver->ports = (struct tty_port **) kcalloc(lines, sizeof(*driver->ports),
				GFP_LX_DMA);
		if (!driver->ports) {
			err = -ENOMEM;
			goto err_free_all;
		}
		cdevs = lines;
	}

	driver->cdevs = (struct cdev **) kcalloc(cdevs, sizeof(*driver->cdevs), GFP_LX_DMA);
	if (!driver->cdevs) {
		err = -ENOMEM;
		goto err_free_all;
	}

	return driver;
err_free_all:
	kfree(driver->ports);
	kfree(driver->ttys);
	kfree(driver->termios);
	kfree(driver->cdevs);
	kfree(driver);
	return (struct tty_driver *) ERR_PTR(err);

}

struct tty_struct *alloc_tty_struct(struct tty_driver *driver, int idx)
{
	struct tty_struct *tty;

	tty = (struct tty_struct *) kzalloc(sizeof(*tty), GFP_LX_DMA);
	if (!tty)
		return NULL;

	// kref_init(&tty->kref);
	tty->magic = TTY_MAGIC;
	// tty_ldisc_init(tty);
	tty->session = NULL;
	tty->pgrp = NULL;
	// mutex_init(&tty->legacy_mutex);
	// mutex_init(&tty->throttle_mutex);
	// init_rwsem(&tty->termios_rwsem);
	// mutex_init(&tty->winsize_mutex);
	// init_ldsem(&tty->ldisc_sem);
	// init_waitqueue_head(&tty->write_wait);
	// init_waitqueue_head(&tty->read_wait);
	// INIT_WORK(&tty->hangup_work, do_tty_hangup);
	// mutex_init(&tty->atomic_write_lock);
	// spin_lock_init(&tty->ctrl_lock);
	// spin_lock_init(&tty->flow_lock);
	// spin_lock_init(&tty->files_lock);
	// INIT_LIST_HEAD(&tty->tty_files);
	// INIT_WORK(&tty->SAK_work, do_SAK_work);

	tty->driver = driver;
	if (driver->ops == 0)
		Genode::error(__PRETTY_FUNCTION__, ": empty driver ops");
	else
		Genode::warning(__PRETTY_FUNCTION__, ": driver ops");

	tty->ops = driver->ops;
	tty->index = idx;
	// tty_line_name(driver, idx, tty->name);
	// tty->dev = tty_get_device(tty);

	return tty;
}

static int tty_driver_install_tty(struct tty_driver *driver,
						struct tty_struct *tty)
{
	return driver->ops->install ? driver->ops->install(driver, tty) :
		tty_standard_install(driver, tty);
}

struct tty_struct *tty_init_dev(struct tty_driver *driver, int idx)
{
	struct tty_struct *tty;
	int retval;


	Genode::log(__PRETTY_FUNCTION__, ":", __LINE__);
	/*
	 * First time open is complex, especially for PTY devices.
	 * This code guarantees that either everything succeeds and the
	 * TTY is ready for operation, or else the table slots are vacated
	 * and the allocated memory released.  (Except that the termios
	 * may be retained.)
	 */
	tty = alloc_tty_struct(driver, idx);
	if (!tty) {
		retval = -ENOMEM;
		goto err_module_put;
	}
	Genode::log(__PRETTY_FUNCTION__, ":", __LINE__);
	retval = tty_driver_install_tty(driver, tty);
	if (retval < 0)
		goto err_free_tty;

	Genode::log(__PRETTY_FUNCTION__, ":", __LINE__);
	if (!tty->port)
		tty->port = driver->ports[idx];

	WARN(!tty->port,
			"%s: %s driver does not set tty->port. This will crash the kernel later. Fix the driver!\n",
			__func__, tty->driver->name);

	tty->port->itty = tty;
	return tty;

err_free_tty:
	// free_tty_struct(tty);
err_module_put:
	// module_put(driver->owner);
	return (struct tty_struct *) ERR_PTR(retval);
}

void tty_set_operations(struct tty_driver *driver,
			const struct tty_operations *op)
{
	driver->ops = op;
}


extern "C" int tty_register_driver(struct tty_driver *driver)
{

	if (!(driver->flags & TTY_DRIVER_DYNAMIC_DEV)) {
		TRACE_AND_STOP;
	}

	driver->flags |= TTY_DRIVER_INSTALLED;
	return 0;
}


const struct usb_device_id *usb_match_id(struct usb_interface *interface,
					 const struct usb_device_id *id)
{
/* proc_connectinfo in devio.c may call us with id == NULL. */
	if (id == NULL)
		return NULL;

	/* It is important to check that id->driver_info is nonzero,
	   since an entry that is all zeroes except for a nonzero
	   id->driver_info is the way to create an entry that
	   indicates that the driver want to examine every
	   device and interface. */
	for (; id->idVendor || id->idProduct || id->bDeviceClass ||
	       id->bInterfaceClass || id->driver_info; id++) {
		if (usb_match_one_id(interface, id))
			return id;
	}

	return NULL;
}


int usb_match_one_id(struct usb_interface *interface,
			    const struct usb_device_id *id)
{
	struct usb_host_interface *intf;
	struct usb_device *dev;

	/* proc_connectinfo in devio.c may call us with id == NULL. */
	if (id == NULL)
		return 0;

	intf = interface->cur_altsetting;
	dev = interface_to_usbdev(interface);

	if (!usb_match_device(dev, id))
		return 0;

	return usb_match_one_id_intf(dev, intf, id);
}

struct usb_device *usb_get_dev(struct usb_device *dev)
{
	if (dev)
		get_device(&dev->dev);
	return dev;
}

struct usb_interface *usb_get_intf(struct usb_interface *intf)
{
	if (intf)
		get_device(&intf->dev);
	return intf;
}

unsigned long __get_free_pages(gfp_t gfp_mask, unsigned int order)
{
	struct page * pages = alloc_pages(gfp_mask, order);
	if (!pages)
		return 0;

	return (unsigned long)pages->addr;
}

