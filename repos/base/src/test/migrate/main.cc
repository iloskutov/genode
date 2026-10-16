/*
 * \brief  Testing thread migration
 * \author Alexander Boettcher
 * \date   2020-08-13
 */

/*
 * Copyright (C) 2020 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

/* Genode includes */
#include <base/component.h>
#include <cpu_session/connection.h>
#include <cpu_thread/client.h>
#include <timer_session/connection.h>
#include <trace_session/connection.h>

using namespace Genode;


/************************************************
 ** Migrate thread over all available CPU test **
 ************************************************/

struct Migrate_thread : Thread
{
	Blockade  blockade { };
	Blockade  ready    { };
	Env      &env;

	Migrate_thread(Env &env)
	: Thread(env, "migrate", Stack_size { 0x3000 }), env(env)
	{ }

	void entry() override
	{
		Cpu_thread_client thread_client(cap());

		/* we are ready */
		ready.wakeup();

		while (true) {
			log("[migrate] going to sleep");

			blockade.block();

			log("[migrate] woke up - got migrated ?");
		}
	}
};

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
 * Thread that wakes up its CPU periodically
 *
 * The kernel accounts the execution time of a CPU's idle thread only when
 * the CPU enters the kernel. The periodic wakeups keep the idle times
 * of all CPUs up to date.
 */
struct Tick_thread : Thread
{
	Timer::Connection timer;

	Tick_thread(Env &env, Affinity::Location location)
	: Thread(env, "tick", Stack_size { 0x2000 }, location), timer(env) { }

	void entry() override
	{
		while (true)
			timer.msleep(10);
	}
};


struct Migrate
{
	Env               &env;
	Timer::Connection  timer  { env };
	Migrate_thread     thread { env };
	Spin_thread        spin   { env };
	Trace::Connection  trace  { env, 15 * 4096 /* RAM quota */,
	                                 11 * 4096 /* ARG_BUFFER RAM quota */ };

	Signal_handler<Migrate> timer_handler { env.ep(), *this,
	                                        &Migrate::check_traces };

	Affinity::Location location { };
	unsigned           loc_same { 0 };
	unsigned           loc_pos  { 0 };

	unsigned           test_rounds { 0 };

	enum State {
		LOOKUP_TRACE_ID, CHECK_AFFINITY, MIGRATE, SPIN_MOVE, SPIN_CHECK, DONE
	} state { LOOKUP_TRACE_ID };

	enum { MAX_CPUS = 16 };

	/* state of the moves of the running thread 'spin' */
	unsigned           spin_moves    { 0 };
	unsigned long      spin_counter  { 0 };
	Affinity::Location spin_expected { };
	Affinity::Location spin_request  { };
	uint64_t           idle_time[MAX_CPUS] { };
	bool               idle_seen[MAX_CPUS] { };

	Constructible<Tick_thread> ticks[MAX_CPUS] { };

	Migrate(Env &env) : env(env)
	{
		Affinity::Space cpus = env.cpu().affinity_space();
		log("Detected ", cpus.width(), "x", cpus.height(), " CPU",
		    cpus.total() > 1 ? "s." : ".");

		timer.sigh(timer_handler);

		thread.start();
		thread.ready.block();

		timer.trigger_periodic( 500 * 1000 /* us */);
	}

	void check_traces()
	{
		switch (state) {
		case LOOKUP_TRACE_ID:
		{
			auto count = trace.for_each_subject_info([&](Trace::Subject_id const &,
			                                             Trace::Subject_info const &info)
			{
				if (info.thread_name() != "migrate")
					return;

				location = info.affinity();
				state    = CHECK_AFFINITY;

				log("[ep] thread '", info.thread_name(), "'  started,",
				    " location=", location.xpos(), "x", location.ypos());
			});

			if (count.count == count.limit && state == LOOKUP_TRACE_ID) {
				error("trace argument buffer too small for the test");
			}
			break;
		}
		case CHECK_AFFINITY:
		{
			trace.for_each_subject_info([&](Trace::Subject_id const &,
			                                Trace::Subject_info const &info)
			{
				if (info.thread_name() != "migrate")
					return;

				loc_same ++;

				if ((location.xpos() == info.affinity().xpos()) &&
				    (location.ypos() == info.affinity().ypos()) &&
				    (location.width() == info.affinity().width()) &&
				    (location.height() == info.affinity().height()))
				{
					if (loc_same >= 1) {
						loc_same = 0;
						state = MIGRATE;
					}
					log ("[ep] .");
					return;
				}

				loc_same = 0;
				location = info.affinity();

				log("[ep] thread '", info.thread_name(), "' migrated,",
				    " location=", location.xpos(), "x", location.ypos());

				test_rounds ++;
				if (test_rounds == 4)
					start_spin();
			});

			break;
		}
		case MIGRATE:
		{
			state = CHECK_AFFINITY;

			loc_pos ++;
			Affinity::Location const loc = env.cpu().affinity_space().location_of_index(loc_pos);

			/* trigger migration */
			Cpu_thread_client client(thread.cap());
			client.affinity(loc);

			log("[ep] thread 'migrate' scheduled to migrate to location=",
			    loc.xpos(), "x", loc.ypos());

			thread.blockade.wakeup();

			break;
		}
		case SPIN_MOVE:
			spin_move();
			break;
		case SPIN_CHECK:
			spin_check();
			break;
		case DONE:
			break;
		}
	}

	void start_spin()
	{
		Affinity::Space const cpus = env.cpu().affinity_space();

		for (unsigned i = 0; i < cpus.total() && i < MAX_CPUS; i++) {
			ticks[i].construct(env, cpus.location_of_index(i));
			ticks[i]->start();
		}

		spin.start();
		state = SPIN_MOVE;
	}

	/**
	 * Remember the execution time of each CPU's idle thread
	 */
	void snapshot_idle_times()
	{
		trace.for_each_subject_info([&](Trace::Subject_id const &,
		                                Trace::Subject_info const &info)
		{
			unsigned const cpu = info.affinity().xpos();
			if (info.thread_name() != "idle" || cpu >= MAX_CPUS)
				return;

			idle_time[cpu] = info.execution_time().thread_context;
			idle_seen[cpu] = true;
		});
	}

	/**
	 * Move the running thread 'spin' to the next CPU
	 *
	 * After one move to each CPU, request a location outside of the
	 * affinity space, which must leave the thread where it is.
	 */
	void spin_move()
	{
		Affinity::Space const cpus = env.cpu().affinity_space();

		spin_moves ++;

		if (spin_moves <= cpus.total()) {
			spin_request  = cpus.location_of_index(spin_moves % cpus.total());
			spin_expected = spin_request;
		} else {
			spin_request  = Affinity::Location(cpus.width(), 0);
			spin_expected = cpus.location_of_index(0);
		}

		spin_counter = spin.counter;

		Cpu_thread_client(spin.cap()).affinity(spin_request);

		log("[ep] thread 'spin' requested to move to location=",
		    spin_request.xpos(), "x", spin_request.ypos());

		snapshot_idle_times();
		state = SPIN_CHECK;
	}

	/**
	 * Check that 'spin' reports the expected location and kept running
	 *
	 * The idle thread of the CPU 'spin' runs on must not have run in the
	 * meantime, as 'spin' never blocks.
	 */
	void spin_check()
	{
		uint64_t idle_before[MAX_CPUS];
		for (unsigned i = 0; i < MAX_CPUS; i++)
			idle_before[i] = idle_time[i];

		snapshot_idle_times();

		bool               found    = false;
		Affinity::Location location { };
		trace.for_each_subject_info([&](Trace::Subject_id const &,
		                                Trace::Subject_info const &info)
		{
			if (info.thread_name() != "spin")
				return;

			found    = true;
			location = info.affinity();
		});

		unsigned long const counter = spin.counter;

		if (!found) {
			error("thread 'spin' not found via trace");
			return;
		}

		if (location.xpos() != spin_expected.xpos() ||
		    location.ypos() != spin_expected.ypos()) {
			error("thread 'spin' at location=", location.xpos(), "x",
			      location.ypos(), ", expected ", spin_expected.xpos(),
			      "x", spin_expected.ypos());
			return;
		}

		if (counter == spin_counter) {
			error("thread 'spin' stopped counting at location=",
			      location.xpos(), "x", location.ypos());
			return;
		}

		unsigned const cpu = location.xpos();
		if (cpu < MAX_CPUS && idle_seen[cpu]) {

			uint64_t max_idle = 0;
			for (unsigned i = 0; i < MAX_CPUS; i++)
				if (idle_seen[i] && idle_time[i] - idle_before[i] > max_idle)
					max_idle = idle_time[i] - idle_before[i];

			uint64_t const idle = idle_time[cpu] - idle_before[cpu];
			if (idle*4 > max_idle) {
				error("CPU ", cpu, " idled while thread 'spin' is said to "
				      "run there (idle ", idle, " of max ", max_idle, ")");
				return;
			}
		}

		bool const moved = (spin_request.xpos() == spin_expected.xpos());

		log("[ep] thread 'spin' ", moved ? "runs" : "stays",
		    " at location=", location.xpos(), "x", location.ypos(),
		    ", counter advanced");

		if (!moved) {
			log("--- test completed successfully ---");
			state = DONE;
			return;
		}

		state = SPIN_MOVE;
	}
};


void Component::construct(Env &env)
{
	log("--- migrate thread test started ---");

	static Migrate migrate_test(env);
}
