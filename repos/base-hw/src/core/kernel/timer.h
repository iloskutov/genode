/*
 * \brief   A timer manages a continuous time and timeouts on it
 * \author  Martin Stein
 * \date    2016-03-23
 */

/*
 * Copyright (C) 2016-2017 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

#ifndef _CORE__KERNEL__TIMER_H_
#define _CORE__KERNEL__TIMER_H_

/* base-hw includes */
#include <kernel/types.h>
#include <kernel/irq.h>
#include <kernel/timeout.h>

#include <board.h>

namespace Kernel {
	class Cpu;
	class Timer;
}


/**
 * A timer manages a continuous time and timeouts on it
 */
class Kernel::Timer
{
	private:

		class Irq : private Kernel::Irq
		{
			private:

				Timer &_timer;

			public:

				Irq(unsigned id, Cpu &cpu);

				void occurred() override;
		};

		Cpu                  &_cpu;
		Board::Timer          _device;
		Irq                   _irq;
		time_t                _time = 0;
		Timeout_list          _timeouts {};

		/* a timeout was set from another CPU, the device is outdated */
		bool                  _device_outdated { false };

		void _start_one_shot(time_t const ticks);

		time_t _value();

		time_t _max_value() const;

		time_t _duration() const;

		void _process_timeouts();

		void _schedule_timeout();

		/**
		 * Program the device for a new first timeout
		 */
		void _first_timeout_changed();

	public:

		Timer(Cpu &cpu);

		void set_timeout(Timeout &timeout, time_t const duration);

		/**
		 * Move 'timeout' with its remaining ticks to timer 'to'
		 *
		 * The time of all timers must be the same, which holds for
		 * a counter shared by all CPUs.
		 */
		void move_timeout(Timeout &timeout, Timer &to);

		/**
		 * Program the device if a timeout was set from another CPU
		 *
		 * Must be called by the CPU of this timer.
		 */
		void update_device();

		time_t ticks_left(Timeout const &timeout) const {
			return (timeout._end > time()) ? timeout._end - time() : 0; }

		time_t us_to_ticks(time_t const us) const;

		time_t ticks_to_us(time_t const ticks) const;

		time_t timeout_max_us() const;

		unsigned interrupt_id() const;

		time_t time() const { return _time + _duration(); }

		void init() { _device.init(); }

};

#endif /* _CORE__KERNEL__TIMER_H_ */
