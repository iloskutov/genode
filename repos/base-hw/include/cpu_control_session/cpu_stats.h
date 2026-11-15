/*
 * \brief  Idle time of each CPU, written by the kernel, read without a call
 * \author Ivan Loskutov
 * \date   2026-11-15
 *
 * A Cpu_control session hands out a read-only dataspace with this layout
 * ('Cpu_control::Session::cpu_stats'). The kernel updates the record
 * of a CPU whenever the CPU switches between its idle thread and another
 * thread. A reader takes the idle time up to now from the record and the
 * counter value it reads itself, so an idle CPU need not be woken up for
 * an accurate value.
 */

/*
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

#ifndef _INCLUDE__CPU_CONTROL_SESSION__CPU_STATS_H_
#define _INCLUDE__CPU_CONTROL_SESSION__CPU_STATS_H_

#include <base/stdint.h>

namespace Genode { namespace Cpu_control { struct Cpu_stats; } }


struct Genode::Cpu_control::Cpu_stats
{
	enum { MAX_CPUS = 64 };

	/*
	 * Ticks of the counter the kernel uses, e.g. the physical counter of
	 * the ARMv8 generic timer, which is readable from user level
	 */
	uint32_t volatile counter_hz;
	uint32_t          _reserved0;
	uint64_t          _reserved1[3];

	struct Cpu
	{
		uint32_t volatile seq;         /* odd while the kernel writes */
		uint32_t volatile idle;        /* idle thread runs since 'since' */
		uint64_t volatile since;       /* counter at the last switch */
		uint64_t volatile idle_ticks;  /* idle before 'since' */
		uint64_t          _reserved;
	} cpu[MAX_CPUS];

	/**
	 * Counter ticks a CPU was idle up to 'now'
	 *
	 * \param now  counter value read by the caller
	 *
	 * \return false if no consistent record could be read
	 */
	bool idle_ticks(unsigned const index, uint64_t const now, uint64_t &result) const
	{
		if (index >= MAX_CPUS)
			return false;

		Cpu const &c = cpu[index];

		for (unsigned attempt = 0; attempt < 16; attempt++) {

			uint32_t const seq = c.seq;
			__atomic_thread_fence(__ATOMIC_ACQUIRE);

			uint32_t const idle  = c.idle;
			uint64_t const since = c.since;
			uint64_t const ticks = c.idle_ticks;

			__atomic_thread_fence(__ATOMIC_ACQUIRE);
			if ((seq & 1) || seq != c.seq)
				continue;

			result = ticks + ((idle && now > since) ? now - since : 0);
			return true;
		}
		return false;
	}
};

#endif /* _INCLUDE__CPU_CONTROL_SESSION__CPU_STATS_H_ */
