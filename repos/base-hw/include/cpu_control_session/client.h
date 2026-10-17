/*
 * \brief  Client-side interface of the Cpu_control session
 * \author Ivan Loskutov
 * \date   2026-10-17
 */

/*
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

#ifndef _INCLUDE__CPU_CONTROL_SESSION__CLIENT_H_
#define _INCLUDE__CPU_CONTROL_SESSION__CLIENT_H_

#include <cpu_control_session/cpu_control_session.h>
#include <base/rpc_client.h>

namespace Genode { namespace Cpu_control { struct Session_client; } }


struct Genode::Cpu_control::Session_client : Rpc_client<Session>
{
	explicit Session_client(Capability<Session> session)
	: Rpc_client<Session>(session) { }

	Affinity_result affinity(Session_label const &label, Thread_name const &name,
	                         Affinity::Location location) override {
		return call<Rpc_affinity>(label, name, location); }
};

#endif /* _INCLUDE__CPU_CONTROL_SESSION__CLIENT_H_ */
