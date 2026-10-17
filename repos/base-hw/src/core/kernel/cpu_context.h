/*
 * \brief   Class for kernel data that is needed to manage a specific CPU
 * \author  Martin Stein
 * \author  Stefan Kalkowski
 * \date    2014-01-14
 */

/*
 * Copyright (C) 2014-2017 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

#ifndef _CORE__KERNEL__CPU_CONTEXT_H_
#define _CORE__KERNEL__CPU_CONTEXT_H_

/* core includes */
#include <kernel/scheduler.h>
#include <kernel/timer.h>

namespace Kernel {

	class Core_thread;
	class Cpu;
	class Cpu_context;
}


/**
 * Context (thread, vcpu) that shall be executed by a CPU
 */
class Kernel::Cpu_context : private Scheduler::Context
{
	private:

		friend class Cpu;
		friend class Core_thread;

		Cpu *_cpu_ptr;

		/*
		 * Noncopyable
		 */
		Cpu_context(Cpu_context const &);
		Cpu_context &operator = (Cpu_context const &);

	protected:

		Cpu &_cpu() const { return *_cpu_ptr; }

		/**
		 * Handle interrupt exception
		 */
		void _interrupt();

		void _activate();
		void _deactivate();

		/**
		 * Yield the currently scheduled CPU share of this context
		 */
		void _yield();

		/**
		 * Return possibility to help context 'j' scheduling-wise
		 */
		bool _helping_possible(Cpu_context const &j) const {
			return j._cpu_ptr == _cpu_ptr; }

		void _help(Cpu_context &context);

		/**
		 * End the helping of this context and of its helpers
		 *
		 * Helping works only between contexts of the same CPU.
		 *
		 * \param fn  called with each context that stopped helping,
		 *            this one included if it helped another context
		 */
		void _helping_cut(auto const &fn)
		{
			_account_helping_change();
			Context::helping_cut([&] (Context &c) {
				fn(static_cast<Cpu_context &>(c)); });
		}

		/**
		 * Account the time up to now before the helping of this context changes
		 */
		void _account_helping_change();

		/**
		 * End the helping of this context
		 */
		void helping_finished();

		/**
		 * Assign this unready context to CPU 'cpu'
		 */
		void _move_to(Cpu &cpu);

		using Context::ready;

	public:

		using Context  = Scheduler::Context;
		using Group_id = Scheduler::Group_id;

		Cpu_context(Cpu &cpu, Group_id const id);

		virtual ~Cpu_context();

		using Scheduler::Context::execution_time;

		/**
		 * Returns true if this context is assigned to a
		 * different cpu than the currently executing one.
		 */
		bool remotely_assigned();

		/**
		 * Returns true if this context is running currently on a
		 * different cpu than the currently executing one.
		 */
		bool remotely_running();

		/**
		 * Handle exception that occured during execution of this context
		 */
		virtual void exception(Cpu_state&) = 0;

		/**
		 * Continue execution of this context
		 */
		virtual void load() = 0;

		/**
		 * Continue execution of this context using given state
		 */
		virtual void load(Cpu_state&) = 0;

		/**
		 * Have to save cpu state of this context
		 */
		virtual void save(Cpu_state&) = 0;
};

#endif /* _CORE__KERNEL__CPU_CONTEXT_H_ */
