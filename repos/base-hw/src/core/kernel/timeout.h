/*
 * \brief   Timeouts of a timer, ordered by their end time
 * \author  Martin Stein
 * \author  Ivan Loskutov
 * \date    2026-10-17
 *
 * The code moved here from 'kernel/timer.h', kept free of the timer
 * device, so that the scheduler unit test can check how a timeout moves
 * between timers.
 */

/*
 * Copyright (C) 2016-2026 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

#ifndef _CORE__KERNEL__TIMEOUT_H_
#define _CORE__KERNEL__TIMEOUT_H_

/* base-hw includes */
#include <kernel/types.h>

/* Genode includes */
#include <util/list.h>

namespace Kernel {
	class Timeout;
	class Timeout_list;
	class Timer;
}


/**
 * A timeout causes a kernel pass and the call of a timeout specific handle
 */
class Kernel::Timeout : Genode::List<Timeout>::Element
{
	friend class Timer;
	friend class Timeout_list;
	friend class Genode::List<Timeout>;

	private:

		bool   _listed     = false;
		time_t _end        = 0;

	public:

		/**
		 * Callback handle
		 */
		virtual void timeout_triggered() { }

		virtual ~Timeout() { }

		bool   listed() const { return _listed; }
		time_t end()    const { return _end; }
};


/**
 * Timeouts of one timer in ascending order of their end time
 */
class Kernel::Timeout_list
{
	private:

		Genode::List<Timeout> _list {};

	public:

		Timeout *first() { return _list.first(); }

		/**
		 * Let 'timeout' end at time 'end', also if it is listed already
		 *
		 * \return true if 'timeout' is the first one now, so the timer
		 *         device has to be programmed for it
		 */
		bool set(Timeout &timeout, time_t const end)
		{
			remove(timeout);

			timeout._listed = true;
			timeout._end    = end;

			Timeout *t1 = nullptr;
			for (Timeout *t2 = _list.first();
			     t2 && t2->_end < timeout._end;
			     t1 = t2, t2 = t2->next()) { }

			_list.insert(&timeout, t1);

			return _list.first() == &timeout;
		}

		void remove(Timeout &timeout)
		{
			if (!timeout._listed)
				return;

			_list.remove(&timeout);
			timeout._listed = false;
		}

		/**
		 * Move a listed 'timeout' with its remaining ticks to list 'to'
		 *
		 * The two lists may count time from different bases.
		 *
		 * \param now     current time of this list's timer
		 * \param to_now  current time of the timer of 'to'
		 *
		 * \return true if 'timeout' is the first one of 'to' now
		 */
		bool move(Timeout &timeout, time_t const now,
		          Timeout_list &to, time_t const to_now)
		{
			if (!timeout._listed)
				return false;

			time_t const left = (timeout._end > now) ? timeout._end - now : 0;

			remove(timeout);
			return to.set(timeout, to_now + left);
		}
};

#endif /* _CORE__KERNEL__TIMEOUT_H_ */
