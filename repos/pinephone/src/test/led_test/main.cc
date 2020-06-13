/*
 * \brief  Test GPIO driver with Leds
 * \author Reinier Millo Sánchez <rmillo@uclv.cu>
 * \author Martin Stein
 * \date   2015-07-26
 */

/*
 * Copyright (C) 2012-2017 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

/* Genode includes */
#include <base/component.h>
#include <base/attached_rom_dataspace.h>
#include <gpio_session/connection.h>
#include <irq_session/client.h>
#include <timer_session/connection.h>

using namespace Genode;


struct Main
{
	Env                    &env;
	Gpio::Connection        ledG     { env, 114 };
	Gpio::Connection        ledR     { env, 115 };
	Gpio::Connection        ledB     { env, 116 };
	Gpio::Connection        pl7     { env, 359 };
	Timer::Connection       timer    { env };

	Main(Env &env);
};


Main::Main(Env &env) : env(env)
{
	log("--- RGB LED test ---");

	while (1) {
#if 0
		log("R");
		ledR.write(true);
		timer.msleep(500);
		ledR.write(false);
		timer.msleep(500);

		log("G");
		ledG.write(true);
		timer.msleep(500);
		ledG.write(false);
		timer.msleep(500);
#endif

		log("B");
		ledB.write(true);
		// pl7.write(true);
		timer.msleep(500);
		ledB.write(false);
		// pl7.write(false);
		timer.msleep(500);
	}
}


void Component::construct(Env &env) { static Main main(env); }
