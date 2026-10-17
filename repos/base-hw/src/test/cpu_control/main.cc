/*
 * \brief  Test of core's Cpu_control service: move a thread
 * \author Ivan Loskutov
 * \date   2026-10-17
 *
 * The thread 'spin' counts without pause. The test finds its CPU session's
 * label and its location through the TRACE session, moves it with the
 * Cpu_control session to the CPU 2, and checks through the TRACE session
 * that it runs there and still counts. Before, it checks the refusals: a
 * location outside of the CPUs, an unknown thread, the kernel's idle
 * thread, and a thread that has been destroyed.
 */

/*
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

/* Genode includes */
#include <base/component.h>
#include <base/thread.h>
#include <cpu_control_session/connection.h>
#include <timer_session/connection.h>
#include <trace_session/connection.h>

using namespace Genode;


/**
 * Thread that never blocks, it counts as fast as it can
 */
struct Spin_thread : Thread
{
	unsigned long volatile counter { 0 };

	Spin_thread(Env &env) : Thread(env, "spin", Stack_size { 0x3000 }) { }

	void entry() override
	{
		while (true)
			counter = counter + 1;
	}
};


/**
 * Thread that ends right away, for a move after its destruction
 */
struct Short_thread : Thread
{
	Short_thread(Env &env) : Thread(env, "short", Stack_size { 0x2000 }) { }

	void entry() override { }
};


struct Main
{
	using Result = Cpu_control::Session::Affinity_result;

	Env &env;

	Timer::Connection       timer       { env };
	Trace::Connection       trace       { env, 15*4096 /* RAM quota */, 11*4096 /* arguments */ };
	Cpu_control::Connection cpu_control { env };

	Spin_thread spin { env };

	Affinity::Location expected { };
	unsigned long      counter  { 0 };

	Signal_handler<Main> check_handler { env.ep(), *this, &Main::check };

	static char const *name(Result r)
	{
		switch (r) {
		case Result::OK:               return "ok";
		case Result::INVALID_THREAD:   return "invalid thread";
		case Result::INVALID_LOCATION: return "invalid location";
		}
		return "?";
	}

	/**
	 * Label and location of the thread 'name' as the TRACE session reports
	 */
	bool lookup(char const *thread, Session_label &label, Affinity::Location &location)
	{
		bool found = false;
		trace.for_each_subject_info([&] (Trace::Subject_id const &,
		                                 Trace::Subject_info const &info) {
			if (info.thread_name() != thread)
				return;
			label    = info.session_label();
			location = info.affinity();
			found    = true; });
		return found;
	}

	Main(Env &env) : env(env)
	{
		log("--- Cpu_control test started ---");

		spin.start();

		Session_label      label { };
		Affinity::Location location { };
		if (!lookup("spin", label, location)) {
			error("thread 'spin' not found via trace");
			return;
		}

		Affinity::Space const cpus = env.cpu().affinity_space();

		log("move of 'spin' outside of the CPUs: ",
		    name(cpu_control.affinity(label, "spin", Affinity::Location(cpus.width(), 0))));

		log("move of an unknown thread: ",
		    name(cpu_control.affinity(label, "unknown", Affinity::Location(0, 0))));

		log("move of an idle thread: ",
		    name(cpu_control.affinity("kernel", "idle", Affinity::Location(0, 0))));

		{
			Short_thread short_thread(env);
			short_thread.start();
			short_thread.join();
		}
		log("move of a destroyed thread: ",
		    name(cpu_control.affinity(label, "short", Affinity::Location(0, 0))));

		expected = cpus.location_of_index(2 % cpus.total());
		counter  = spin.counter;

		log("move of 'spin' to location=", expected.xpos(), "x", expected.ypos(), ": ",
		    name(cpu_control.affinity(label, "spin", expected)));

		/* let 'spin' run at its new location for a while */
		timer.sigh(check_handler);
		timer.trigger_once(500*1000);
	}

	void check()
	{
		Session_label      label { };
		Affinity::Location location { };
		lookup("spin", label, location);

		if (location.xpos() != expected.xpos() || location.ypos() != expected.ypos()) {
			error("thread 'spin' at location=", location.xpos(), "x", location.ypos(),
			      " after the move, expected ", expected.xpos(), "x", expected.ypos());
			return;
		}

		if (spin.counter == counter) {
			error("thread 'spin' stopped counting after the move");
			return;
		}

		log("thread 'spin' runs at location=", location.xpos(), "x", location.ypos(),
		    " after the move, counter advanced");
		log("--- test completed successfully ---");
	}
};


void Component::construct(Env &env) { static Main main(env); }
