/*
 * \brief  Fiasco(.OC) KDB UART driver
 * \author Christian Prochaska
 * \date   2013-03-07
 */

/*
 * Copyright (C) 2013-2017 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */
#pragma once

/* Genode includes */
#include <base/signal.h>
#include <base/sleep.h>
#include <timer_session/connection.h>

#include <os/ring_buffer.h>

#include <lx_kit/scheduler.h>

namespace Uart {
	using namespace Genode;

	class Driver;
	struct Driver_factory;
	struct Char_avail_functor;
};

struct tty_device;

/**
 * Functor, called by 'Driver' when data is ready for reading
 */
struct Uart::Char_avail_functor
{
	Genode::Signal_context_capability sigh;

	void operator ()() {
		if (sigh.valid()) Genode::Signal_transmitter(sigh).submit();
		// else Genode::error("no sigh");
	}

};


class Uart::Driver
{
public:
		typedef Genode::Ring_buffer<char, 4096, Genode::Ring_buffer_unsynchronized> Ring_buffer;

	private:
		struct Xmit_data
		{
			Xmit_data(tty_device *d) : ttydev(d) {}
			Ring_buffer buf;
			tty_device *ttydev;
		};

		Char_avail_functor    &_char_avail;
		Timer::Connection      _timer;
		Signal_handler<Driver> _timer_handler;
		tty_device *_ttydev;

		Xmit_data  _tx_data;
		Ring_buffer _rx_buf;

		void _timeout() { if (char_avail()) _char_avail(); }

		static void _run_put_task(void * args);

		// TODO

		Lx::Task _put_task { _run_put_task, &_tx_data, "put_task",
					Lx::Task::PRIORITY_1, Lx::scheduler() };


	public:

		Driver(Genode::Env &env, unsigned, unsigned,
		       Uart::Char_avail_functor &func,
			   tty_device *dev);

		/***************************
		 ** UART driver interface **
		 ***************************/

		void put_char(char c);

		bool char_avail();

		char get_char();

		void baud_rate(int);
};


/**
 * Factory used by 'Uart::Root' at session creation/destruction time
 */
struct Uart::Driver_factory
{
	struct Not_available {};

    // TODO: ???
	enum { UARTS_NUM = 8 };

	Genode::Env  &env;
	Genode::Heap &heap;
	Driver       *drivers[UARTS_NUM] { nullptr };

	Driver_factory(Genode::Env &env, Genode::Heap &heap)
	: env(env), heap(heap) {
		for (unsigned i = 0; i < UARTS_NUM; i++) drivers[i] = 0;
	}


	Uart::Driver &create(unsigned index, unsigned baudrate,
	                     Uart::Char_avail_functor &callback, tty_device *dev);

	void destroy(Uart::Driver *) { /* TODO */ }

};

