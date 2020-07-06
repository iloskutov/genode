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
#include <uart_session/connection.h>

using namespace Genode;

#define PORT(l, p) (((l) - 'A') * 32 + (p))


struct Main
{
	Env                    &env;
	// Gpio::Connection        pwr_key     { env, PORT('B', 3) };
	// Gpio::Connection        pwr     { env, PORT('L', 7) };
	// Gpio::Connection        reset     { env, PORT('C', 4) };
	// Gpio::Connection        disable     { env, PORT('H', 8) };
	// Gpio::Connection        wakeup     { env, PORT('H', 7) };
	Timer::Connection       timer    { env };

	Main(Env &env);
};


Main::Main(Env &env) : env(env)
{
	log("--- Modem test ---");
#if 0
	disable.write(false);
	wakeup.write(false);
	timer.msleep(200);

	reset.write(false);
	timer.msleep(500);

	// pwr.write(true);
	// timer.msleep(200);
	pwr_key.write(true);
	timer.msleep(200);
	pwr_key.write(false);
#endif

	timer.msleep(1000);

	// connect to modem
	{
		Uart::Connection  uart(env);

		const char send[] = "AT\r\n";
		char recv[100] = { 0 };
		int pos = 0;

		size_t cnt = 0;
		for (;;) {
			uart.write(send, sizeof(send) - 1);
			warning("send: ", send);

			cnt = 1000000;
			while (cnt != 0) {
				if (uart.avail()) {
					int r = uart.read(recv + pos, sizeof(recv) - pos);
					for (int k = 0; k < r; k++)
						warning("> ", Hex(recv[pos + k]));
					pos += r;

					if (recv[pos - 1] == '\n')
						break;
				}
				cnt--;
			}
			if (cnt == 0)
				error("Timeout");

			recv[pos + 1] = '\0';

			warning("recv: ", (const char*) recv);
			pos = 0;
			memset(recv, 0, sizeof(recv));
			timer.msleep(1000);
		}
	}

	while(1)
		;
}


void Component::construct(Env &env) { static Main main(env); }
