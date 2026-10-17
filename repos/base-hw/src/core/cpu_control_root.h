/*
 * \brief  Core's Cpu_control service
 * \author Ivan Loskutov
 * \date   2026-10-17
 *
 * A session moves threads named by their CPU session's label and their
 * name. Core forwards the move through the thread's trace source to its
 * Cpu_thread_component and Platform_thread::affinity(). The service runs
 * on the entrypoint of the CPU service, so no thread vanishes during a
 * call: a thread's trace source is destructed before its platform thread.
 */

/*
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

#ifndef _CORE__CPU_CONTROL_ROOT_H_
#define _CORE__CPU_CONTROL_ROOT_H_

/* Genode includes */
#include <base/rpc_server.h>
#include <cpu_control_session/cpu_control_session.h>
#include <root/component.h>

/* core includes */
#include <trace/source_registry.h>

namespace Core {

	class Cpu_control_session_component;
	struct Cpu_control_root;
}


class Core::Cpu_control_session_component : public Rpc_object<Cpu_control::Session>
{
	private:

		Trace::Source_registry &_sources;

	public:

		Cpu_control_session_component(Trace::Source_registry &sources)
		: _sources(sources) { }

		Affinity_result affinity(Session_label const &label,
		                         Thread_name const &name,
		                         Affinity::Location location) override
		{
			using Source_result = Trace::Source::Affinity_result;

			Affinity_result result = Affinity_result::INVALID_THREAD;

			_sources.for_each_source([&] (Trace::Source &source) {

				if (result == Affinity_result::INVALID_LOCATION)
					return;

				Trace::Source::Info const info = source.info();
				if (info.label != label || info.name != name)
					return;

				switch (source.affinity(location)) {
				case Source_result::OK:               result = Affinity_result::OK; break;
				case Source_result::INVALID_LOCATION: result = Affinity_result::INVALID_LOCATION; break;
				case Source_result::NOT_MOVABLE:      break;
				}
			});
			return result;
		}
};


struct Core::Cpu_control_root : Root_component<Cpu_control_session_component>
{
	Trace::Source_registry &_sources;

	Create_result _create_session(const char *) override
	{
		return _alloc_obj(_sources);
	}

	Cpu_control_root(Rpc_entrypoint &ep, Allocator &md_alloc,
	                 Trace::Source_registry &sources)
	:
		Root_component<Cpu_control_session_component>(&ep, &md_alloc),
		_sources(sources)
	{ }
};

#endif /* _CORE__CPU_CONTROL_ROOT_H_ */
