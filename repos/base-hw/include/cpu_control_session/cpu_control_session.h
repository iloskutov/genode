/*
 * \brief  Control of the CPUs by a user-level balancer
 * \author Ivan Loskutov
 * \date   2026-10-17
 *
 * A CPU balancer that interposes the CPU sessions of its clients cannot
 * keep their priorities: each init puts the session requests of a child
 * into the child's priority band, so all interposed threads end up in the
 * kernel scheduling group of the balancer. Core's Cpu_control service
 * moves a thread of any component instead, named as the TRACE session
 * names it: by the label of its CPU session and its thread name.
 *
 * It also hands out the kernel's records of the idle time of each CPU
 * ('cpu_stats.h'), for a controller of the CPU clocks that samples often.
 *
 * The service gives control over every thread of the system, and its
 * idle records show when other components run. Who may use
 * it is decided by the routes of the scenario, there is no further check.
 */

/*
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

#ifndef _INCLUDE__CPU_CONTROL_SESSION__CPU_CONTROL_SESSION_H_
#define _INCLUDE__CPU_CONTROL_SESSION__CPU_CONTROL_SESSION_H_

#include <base/affinity.h>
#include <base/rpc.h>
#include <base/trace/types.h>
#include <dataspace/capability.h>
#include <session/session.h>

namespace Genode { namespace Cpu_control { struct Session; } }


struct Genode::Cpu_control::Session : Genode::Session
{
	static const char *service_name() { return "Cpu_control"; }

	/*
	 * A session consumes a dataspace capability for the server's
	 * session-object allocation and its session capability
	 */
	static constexpr unsigned CAP_QUOTA = 2;
	static constexpr size_t   RAM_QUOTA = 8*1024;

	using Thread_name = Trace::Thread_name;

	enum class Affinity_result { OK, INVALID_THREAD, INVALID_LOCATION };

	virtual ~Session() { }

	/**
	 * Move the threads of the CPU session 'label' named 'name' to a
	 * physical location, the coordinates the TRACE session reports
	 *
	 * \return  'INVALID_THREAD' if there is no such thread that can be
	 *          moved (core's and the kernel's threads cannot),
	 *          'INVALID_LOCATION' if the location lies outside the
	 *          platform's affinity space
	 */
	virtual Affinity_result affinity(Session_label const &label,
	                                 Thread_name const &name,
	                                 Affinity::Location location) = 0;

	/**
	 * Read-only dataspace with the idle times of the CPUs as written by the
	 * kernel, layout 'Cpu_control::Cpu_stats', invalid on kernels without
	 */
	virtual Dataspace_capability cpu_stats() = 0;


	/*******************
	 ** RPC interface **
	 *******************/

	GENODE_RPC(Rpc_affinity, Affinity_result, affinity, Session_label const &,
	           Thread_name const &, Affinity::Location);

	GENODE_RPC(Rpc_cpu_stats, Dataspace_capability, cpu_stats);

	GENODE_RPC_INTERFACE(Rpc_affinity, Rpc_cpu_stats);
};

#endif /* _INCLUDE__CPU_CONTROL_SESSION__CPU_CONTROL_SESSION_H_ */
