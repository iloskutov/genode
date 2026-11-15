/*
 * \brief  Idle time of each CPU for user level ('Cpu_control::Cpu_stats')
 * \author Ivan Loskutov
 * \date   2026-11-15
 */

/*
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

#ifndef _CORE__KERNEL__CPU_STATS_H_
#define _CORE__KERNEL__CPU_STATS_H_

#include <cpu_control_session/cpu_stats.h>

namespace Kernel {

	/**
	 * Page of the records, part of the kernel's data
	 */
	Genode::Cpu_control::Cpu_stats &cpu_stats();

	enum { CPU_STATS_SIZE = 4096 };

	/**
	 * Counter of the records, readable from user level, 0 if there is none
	 * (then the records stay empty)
	 */
	Genode::uint64_t cpu_stats_counter();
	Genode::uint32_t cpu_stats_counter_hz();

	/**
	 * Record that CPU 'cpu' switched to ('idle') or from its idle thread
	 *
	 * Called by the CPU itself only, the only writer of its record.
	 */
	void cpu_stats_switch(unsigned cpu, bool idle);
}

#endif /* _CORE__KERNEL__CPU_STATS_H_ */
