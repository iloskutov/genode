/*
 * \brief  Test for UART driver
 * \author Christian Helmuth
 * \date   2011-05-30
 */

/*
 * Copyright (C) 2011-2017 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

/* Genode includes */
#include <base/snprintf.h>
#include <base/component.h>
#include <timer_session/connection.h>
#include <uart_session/connection.h>

using namespace Genode;


struct Main
{
	Timer::Connection timer;
	Uart::Connection  uart;
	char              buf[100];

	Main(Env &env) : timer(env), uart(env)
	{
		log("--- UART test started ---");

#if 0
		int n = snprintf(buf, sizeof(buf), "UART hello\n");
		uart.write(buf, n);

		for (;;) {
			if (uart.avail()) {
				int n = uart.read(buf, 10);
				uart.write(buf, n);
			}
		}
#else
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
#endif
	}
};

void Component::construct(Env &env) { static Main main(env); }
