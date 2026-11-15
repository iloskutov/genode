/*
 * \brief  Dataspace of the kernel's CPU idle records ('Cpu_control::Cpu_stats')
 * \author Ivan Loskutov
 * \date   2026-11-15
 */

/*
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

/* core includes */
#include <dataspace_component.h>
#include <platform.h>
#include <cpu_control_root.h>

/* base-hw core includes */
#include <kernel/cpu_stats.h>

using namespace Core;


Dataspace_capability Core::cpu_stats_dataspace(Rpc_entrypoint &ep)
{
	if (!Kernel::cpu_stats_counter_hz())
		return { };

	/* the page of the kernel's data, read-only, cached like RAM */
	static Constructible<Dataspace_component> ds { };

	if (!ds.constructed()) {
		addr_t const virt = addr_t(&Kernel::cpu_stats());
		addr_t const phys = Platform::core_phys_addr(virt);

		if (!phys) {
			error("no physical address of the CPU idle records");
			return { };
		}

		ds.construct(size_t(Kernel::CPU_STATS_SIZE), virt, phys, CACHED,
		             false, nullptr);
		ep.manage(&*ds);
	}

	return static_cap_cast<Dataspace>(ds->cap());
}
