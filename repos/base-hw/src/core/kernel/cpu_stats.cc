/*
 * \brief  Idle time of each CPU for user level ('Cpu_control::Cpu_stats')
 * \author Ivan Loskutov
 * \date   2026-11-15
 */

/*
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

#include <kernel/cpu_stats.h>

using namespace Genode;


/*
 * A page of its own: core hands it out as a dataspace, nothing else may
 * share it
 */
static union alignas(Kernel::CPU_STATS_SIZE) Cpu_stats_page
{
	Cpu_control::Cpu_stats stats;
	char             bytes[Kernel::CPU_STATS_SIZE];
} _cpu_stats_page;

static_assert(sizeof(Cpu_stats_page) == Kernel::CPU_STATS_SIZE);


Cpu_control::Cpu_stats &Kernel::cpu_stats() { return _cpu_stats_page.stats; }


/* architectures with a user-readable counter define these */
__attribute__((weak)) uint64_t Kernel::cpu_stats_counter()    { return 0; }
__attribute__((weak)) uint32_t Kernel::cpu_stats_counter_hz() { return 0; }


void Kernel::cpu_stats_switch(unsigned const cpu, bool const idle)
{
	uint64_t const now = cpu_stats_counter();
	if (!now || cpu >= Cpu_control::Cpu_stats::MAX_CPUS)
		return;

	Cpu_control::Cpu_stats &stats = cpu_stats();
	Cpu_control::Cpu_stats::Cpu &c = stats.cpu[cpu];

	if (!stats.counter_hz)
		stats.counter_hz = cpu_stats_counter_hz();

	/* odd sequence number while the record is inconsistent */
	c.seq = c.seq + 1;
	__atomic_thread_fence(__ATOMIC_RELEASE);

	if (c.idle && now > c.since)
		c.idle_ticks = c.idle_ticks + (now - c.since);

	c.since = now;
	c.idle  = idle;

	__atomic_thread_fence(__ATOMIC_RELEASE);
	c.seq = c.seq + 1;
}
