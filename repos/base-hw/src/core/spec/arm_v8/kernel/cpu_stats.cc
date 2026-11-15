/*
 * \brief  Counter of the CPU idle records on ARMv8: the generic timer's
 *         physical counter, which user level may read (CNTKCTL_EL1.EL0PCTEN)
 * \author Ivan Loskutov
 * \date   2026-11-15
 */

/*
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

#include <kernel/cpu_stats.h>
#include <hw/spec/arm_64/cpu.h>

Genode::uint64_t Kernel::cpu_stats_counter()    { return Hw::Arm_64_cpu::Cntpct::read(); }
Genode::uint32_t Kernel::cpu_stats_counter_hz() { return (Genode::uint32_t)Hw::Arm_64_cpu::Cntfrq::read(); }
