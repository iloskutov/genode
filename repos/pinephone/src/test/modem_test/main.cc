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
#include <irq_session/client.h>
#include <timer_session/connection.h>
#include <uart_session/connection.h>

using namespace Genode;



struct Main
{
	Env                    &env;
	Timer::Connection       timer    { env };

	Main(Env &env);
};


Main::Main(Env &env) : env(env)
{
	log("--- Modem test ---");

	{
		Uart::Connection  uart(env);

		const char send[] = "AT\r\n";
		char recv[100] = { 0 };
		int pos = 0;

		size_t cnt = 0;
		for (;;) {
			uart.write(send, sizeof(send) - 1);
			warning("send: ", send);

			cnt = 100000;
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
