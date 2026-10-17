/*
 * \brief  Connection to core's Cpu_control service
 * \author Ivan Loskutov
 * \date   2026-10-17
 */

/*
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

#ifndef _INCLUDE__CPU_CONTROL_SESSION__CONNECTION_H_
#define _INCLUDE__CPU_CONTROL_SESSION__CONNECTION_H_

#include <cpu_control_session/client.h>
#include <base/connection.h>

namespace Genode { namespace Cpu_control { struct Connection; } }


struct Genode::Cpu_control::Connection : Genode::Connection<Session>, Session_client
{
	Connection(Env &env, Session_label const &label = Session_label())
	:
		Genode::Connection<Session>(env, label, Ram_quota { RAM_QUOTA }, Args()),
		Session_client(cap())
	{ }
};

#endif /* _INCLUDE__CPU_CONTROL_SESSION__CONNECTION_H_ */
