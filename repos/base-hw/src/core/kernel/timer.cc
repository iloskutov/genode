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

/* core includes */
#include <kernel/cpu.h>
#include <kernel/timer.h>
#include <kernel/configuration.h>
#include <kernel/assert.h>

using namespace Kernel;


void Timer::Irq::occurred() { _timer._process_timeouts(); }


Timer::Irq::Irq(unsigned id, Cpu &cpu)
:
	Kernel::Irq { id, cpu.irq_pool(), cpu.pic() },
	_timer { cpu.timer() }
{ }


time_t Timer::timeout_max_us() const
{
	return ticks_to_us(_max_value());
}


void Timer::set_timeout(Timeout &timeout, time_t const duration)
{
	/* timeouts may get overridden as result of an update */
	if (_timeouts.set(timeout, time() + duration))
		_first_timeout_changed();
}


void Timer::_first_timeout_changed()
{
	/* only the CPU of this timer can program its device */
	if (_cpu.id() == Cpu::executing_id()) {
		_schedule_timeout();
		return;
	}

	_device_outdated = true;
	_cpu.trigger_ip_interrupt();
}


void Timer::move_timeout(Timeout &timeout, Timer &to)
{
	if (_timeouts.move(timeout, time(), to._timeouts, to.time()))
		to._first_timeout_changed();
}


void Timer::update_device()
{
	if (!_device_outdated)
		return;

	_device_outdated = false;
	_schedule_timeout();
}


void Timer::_schedule_timeout()
{
	/* get the timeout with the nearest end time */
	Timeout * timeout = _timeouts.first();
	time_t end = timeout ? timeout->_end : _time + _max_value();

	/* install timeout at timer hardware */
	_time += _duration();
	time_t timeout_duration = (end > _time) ? end - _time : 1;
	_start_one_shot(timeout_duration);
}


void Timer::_process_timeouts()
{
	/*
	 * Walk through timeouts until the first whose end time is in the future.
	 */
	time_t t = time();
	while (true) {
		Timeout * const timeout = _timeouts.first();
		if (!timeout)
			break;

		if (timeout->_end > t)
			break;

		_timeouts.remove(*timeout);
		timeout->timeout_triggered();
	}

	if (_timeouts.first())
		_schedule_timeout();
}


Timer::Timer(Cpu &cpu)
:
	_cpu(cpu), _device(cpu), _irq(interrupt_id(), cpu)
{
	/*
	 * The timer frequency should allow a good accuracy on the smallest
	 * timeout syscall value (1 us).
	 */
	ASSERT(ticks_to_us(1) < 1 || ticks_to_us(_max_value()) == _max_value());
}
