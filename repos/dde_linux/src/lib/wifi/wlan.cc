/*
 * \brief  Wireless network driver Linux port
 * \author Josef Soentgen
 * \date   2022-02-10
 */

/*
 * Copyright (C) 2022 Genode Labs GmbH
 *
 * This file is distributed under the terms of the GNU General Public License
 * version 2 or later.
 */

/* Genode includes */
#include <base/attached_rom_dataspace.h>
#include <base/component.h>
#include <base/env.h>
#include <net/mac_address.h>
#include <genode_c_api/uplink.h>
#include <genode_c_api/mac_address_reporter.h>


/* DDE Linux includes */
#include <lx_emul/init.h>
#include <lx_emul/page_virt.h>
#include <lx_emul/task.h>
#include <lx_kit/env.h>
#include <lx_kit/init.h>
#include <lx_user/io.h>

/* wifi includes */
#include <wifi/rfkill.h>

/* local includes */
#include "lx_user.h"
#include "dtb_helper.h"

using namespace Genode;

/* RFKILL handling */

extern "C" int  lx_emul_rfkill_get_any(void);
extern "C" void lx_emul_rfkill_switch_all(int blocked);


struct Rfkill_helper
{
	Wifi::Rfkill_notification_handler &_handler;

	Rfkill_helper(Wifi::Rfkill_notification_handler &handler)
	:
		_handler { handler }
	{ }

	void submit_notification()
	{
		_handler.rfkill_notify();
	}
};


bool Wifi::rfkill_blocked(void)
{
	/*
	 * It is safe to call this from non EP threads as we
	 * only query a variable.
	 */
	return lx_emul_rfkill_get_any();
}


extern "C" unsigned int wifi_ifindex(void)
{
	/* TODO replace with actual qyery */
	return 2;
}


extern "C" char const *wifi_ifname(void)
{
	/* TODO replace with actual qyery */
	return "wlan0";
}


/* used from socket_call.cc */
void _wifi_report_mac_address(Net::Mac_address const &mac_address)
{
	struct genode_mac_address address;

	mac_address.copy(&address);
	genode_mac_address_register("wlan0", address);
}


struct Wlan
{
	Env                    &_env;
	Signal_handler<Wlan> _signal_handler { _env.ep(), *this,
	                                       &Wlan::_handle_signal };

	Dtb_helper _dtb_helper { _env };

	void _handle_signal()
	{
		if (uplink_task_struct_ptr)
			lx_emul_task_unblock(uplink_task_struct_ptr);

		Lx_kit::env().scheduler.execute();

		genode_uplink_notify_peers();
	}

	Constructible<Rfkill_helper>   rfkill_helper { };

	Wlan(Env &env) : _env { env }
	{
		Lx_kit::initialize(env, _signal_handler);

		genode_mac_address_reporter_init(env, Lx_kit::env().heap);

		{
			/*
			 * Query the configuration once at start-up to enable
			 * the reporter. The actual reporting will be done
			 * once by 'genode_mac_address_register()'.
			 */
			Attached_rom_dataspace _config_rom { _env, "config" };
			genode_mac_address_reporter_config(_config_rom.node());
		}

		genode_uplink_init(genode_env_ptr(_env),
		                   genode_allocator_ptr(Lx_kit::env().heap),
		                   genode_signal_handler_ptr(_signal_handler));

		lx_emul_start_kernel(_dtb_helper.dtb_ptr());
	}
};


static Blockade *wpa_blockade;


extern "C" void wakeup_wpa()
{
	static bool called_once = false;
	if (called_once)
		return;

	wpa_blockade->wakeup();
	called_once = true;
}


static Wlan *_wlan_ptr;


void wifi_init(Env &env, Blockade &blockade)
{
	wpa_blockade = &blockade;

	static Wlan wlan(env);
	_wlan_ptr = &wlan;
}


/*
 * Rfkill handling
 */

void Wifi::rfkill_establish_handler(Wifi::Rfkill_notification_handler &handler)
{
	_wlan_ptr->rfkill_helper.construct(handler);
}


void Wifi::set_rfkill(bool blocked)
{
	if (!rfkill_task_struct_ptr)
		return;

	lx_emul_rfkill_switch_all(blocked);

	lx_emul_task_unblock(rfkill_task_struct_ptr);
	Lx_kit::env().scheduler.execute();

	/*
	 * We have to open the device again after unblocking
	 * as otherwise we will get ENETDOWN. So unblock the uplink
	 * task _afterwards_ because there we call * 'dev_open()'
	 * unconditionally and that will bring the netdevice UP again.
	 */
	lx_emul_task_unblock(uplink_task_struct_ptr);
	Lx_kit::env().scheduler.execute();
}


void Wifi::rfkill_notify()
{
	if (_wlan_ptr->rfkill_helper.constructed())
		_wlan_ptr->rfkill_helper->submit_notification();
}
