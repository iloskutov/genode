/*
 * \brief  Unit-test scheduler implementation of the kernel
 * \author Stefan Kalkowski
 * \date   2014-09-30
 */

/*
 * Copyright (C) 2014-2025 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

/* Genode includes */
#include <base/component.h>

/* core includes */
#include <kernel/scheduler.h>

using namespace Genode;

namespace Scheduler_test { struct Main; }


struct Scheduler_test::Main
{
	using time_t = Kernel::time_t;
	using Scheduler = Kernel::Scheduler;
	using vtime_t = Scheduler::vtime_t;

	struct Context : Scheduler::Context
	{
		using Label = String<32>;

		Label const _label;

		Context(Kernel::Scheduler::Group_id const id,
		        Label const &label)
		:
			Kernel::Scheduler::Context { id },
			_label { label }
		{ }

		Label const &label() const { return _label; }
	};

	Env &env;

	enum Id {
		IDLE,
		DRV1, DRV2, DRV3,
		MUL1, MUL2, MUL3,
		APP1, APP2, APP3,
		BCK1, BCK2, BCK3,
		MAX = BCK3
	};

	using Gids = Scheduler::Group_id::Ids;

	Context contexts[MAX+1] {
		[IDLE] = { Gids::INVALID,    "idle"        },
		[DRV1] = { Gids::DRIVER,     "driver1"     },
		[DRV2] = { Gids::DRIVER,     "driver2"     },
		[DRV3] = { Gids::DRIVER,     "driver3"     },
		[MUL1] = { Gids::MULTIMEDIA, "multimedia1" },
		[MUL2] = { Gids::MULTIMEDIA, "multimedia2" },
		[MUL3] = { Gids::MULTIMEDIA, "multimedia3" },
		[APP1] = { Gids::APP,        "app1"        },
		[APP2] = { Gids::APP,        "app2"        },
		[APP3] = { Gids::APP,        "app3"        },
		[BCK1] = { Gids::BACKGROUND, "background1" },
		[BCK2] = { Gids::BACKGROUND, "background2" },
		[BCK3] = { Gids::BACKGROUND, "background3" }
	};

	Kernel::Timer timer {};
	Scheduler scheduler { timer, contexts[IDLE] };

	Context& current() {
		return static_cast<Context&>(scheduler.current()); }

	Context::Label const &label(Kernel::Scheduler::Context &c) const {
		return static_cast<Context&>(c).label(); }

	void dump()
	{
		log("");
		log("Scheduler state: (time=", timer.time(),
		    " min_vtime=", scheduler._min_vtime, " ",
		    " timeout=", timer._next_timeout, ")");
		unsigned i = 0;
		scheduler._for_each_group([&] (Scheduler::Group &group) {
			log("Group ", i++, " (weight=", group._weight,
			    ", warp=", group._warp,
				", limit=", group._warp_limit,
			    ") has vtime: ", group._vtime,
			    " and min_vtime: ", group._min_vtime);

			using List_element = Genode::List_element<Scheduler::Context>;

			if (group._contexts.first()) log("  Contexts:");
			for (List_element * le = group._contexts.first(); le;
			     le = le->next()) {
			Scheduler::Context &c = *le->object();
				log("    ", label(c), " has vtime: ", c._vtime,
				    " and real execution time: ", c.execution_time());
			}
		});

		log("Current context: ", current().label(),
		    " (group=", current()._id.value,
		    ") has vtime: ", current()._vtime,
			" ready execution time: ", current()._ready_execution_time);
		log("                 and real execution time: ",
		    current().execution_time());
	}

	void update_and_check(time_t   const consumed_abs_time,
	                      Id       const expected_current,
	                      time_t   const expected_abs_timeout,
	                      unsigned const line_nr)
	{
		timer.set_time(consumed_abs_time);
		scheduler.update();

		if (&current() != &contexts[expected_current]) {
			error("wrong current context ", current().label(),
			      " in line ", line_nr);
			dump();
			env.parent().exit(-1);
		}

		if (timer._next_timeout != expected_abs_timeout) {
			error("expected timeout ", expected_abs_timeout,
			      " in line ", line_nr);
			error("But actual timeout is: ", timer._next_timeout);
			dump();
			env.parent().exit(-1);
		}
	}

	void test_background_idle();
	void test_one_per_group();
	void test_io_signal();
	void test_all_and_yield();
	void test_destroy_helped();
	void test_migrate();
	void test_timeout_move();
	void test_helping_time();

	/*
	 * Set fixed values for min timeout and group weights and warp
	 * values here, because they may change within the kernel, but
	 * the algorithm logic gets tested here instead
	 */
	static void configure(Scheduler &s)
	{
		s._min_timeout = 500;
		construct_at<Scheduler::Group>(&s._groups[Gids::DRIVER], 2, 400, 2900);
		construct_at<Scheduler::Group>(&s._groups[Gids::MULTIMEDIA], 3, 200, 2900);
		construct_at<Scheduler::Group>(&s._groups[Gids::APP], 2, 100, 2900);
		construct_at<Scheduler::Group>(&s._groups[Gids::BACKGROUND], 1, 0, 0);
	}

	/**
	 * Return whether scheduler 's' holds context 'c' in any of its lists
	 */
	static bool lists(Scheduler &s, Scheduler::Context &c)
	{
		using List_element = Genode::List_element<Scheduler::Context>;

		bool found = (s._current == &c);
		for (List_element *le = s._ready_contexts.first(); le; le = le->next())
			if (le->object() == &c) found = true;
		s._for_each_group([&] (Scheduler::Group &group) {
			for (List_element *le = group._contexts.first(); le; le = le->next())
				if (le->object() == &c) found = true; });
		return found;
	}

	void check(bool const condition, char const *what, unsigned const line_nr)
	{
		if (condition)
			return;

		error(what, " in line ", line_nr);
		dump();
		env.parent().exit(-1);
	}

	Main(Env &env) : env(env) { configure(scheduler); }
};


void Scheduler_test::Main::test_background_idle()
{
	time_t MAX_TIME = scheduler._max_timeout;

	/* params:       time, curr, timeout, line */
	update_and_check(   0, IDLE,       0, __LINE__);
	scheduler.ready(contexts[BCK1]);
	update_and_check(   0, BCK1, MAX_TIME, __LINE__);
	update_and_check(  10, BCK1, MAX_TIME, __LINE__);
	update_and_check(   0, BCK1, MAX_TIME, __LINE__);
	scheduler.ready(contexts[BCK2]);
	update_and_check(  10, BCK2,      510, __LINE__);
	update_and_check( 510, BCK1,     1510, __LINE__);
	update_and_check(1530, BCK2,     2550, __LINE__);
	scheduler.ready(contexts[BCK3]);
	update_and_check(2000, BCK3,     2500, __LINE__);
	update_and_check(2500, BCK2,     3050, __LINE__);
	update_and_check(3050, BCK1,     4000, __LINE__);
	scheduler.unready(contexts[BCK1]);
	update_and_check(3100, BCK3,     3650, __LINE__);
	scheduler.unready(contexts[BCK3]);
	update_and_check(3040, BCK2, MAX_TIME
	                               + 3040, __LINE__);
	update_and_check(4000, BCK2, MAX_TIME
	                               + 4000, __LINE__);
}


void Scheduler_test::Main::test_one_per_group()
{
	scheduler.ready(contexts[BCK1]);
	scheduler.ready(contexts[APP1]);
	scheduler.ready(contexts[DRV1]);
	scheduler.ready(contexts[MUL1]);

	/* params:       time, curr, timeout, line */
	update_and_check(    0, DRV1,     900, __LINE__);
	update_and_check(  900, MUL1,    1700, __LINE__);
	update_and_check( 1700, APP1,    2400, __LINE__);
	update_and_check( 2400, BCK1,    2950, __LINE__);
	update_and_check( 3000, DRV1,    3532, __LINE__);
	update_and_check( 3600, MUL1,    4652, __LINE__);
	update_and_check( 4700, APP1,    5400, __LINE__);
	update_and_check( 5500, DRV1,    6164, __LINE__);
	update_and_check( 6200, MUL1,    7204, __LINE__);
	/* MUL1 warp limit reached */
	update_and_check( 7300, BCK1,    7850, __LINE__);
	update_and_check( 7900, APP1,    8500, __LINE__);
	update_and_check( 8500, DRV1,    9500, __LINE__);
	/* DRV1 warp limit reached */
	update_and_check( 9500, APP1,   10096, __LINE__);
	update_and_check(10100, MUL1,   11206, __LINE__);
	update_and_check(11300, BCK1,   11850, __LINE__);
	update_and_check(11900, APP1,   12696, __LINE__);
	/* APP1 warp limit reached */
	update_and_check(12700, MUL1,   13806, __LINE__);
	update_and_check(13900, DRV1,   14700, __LINE__);
}


void Scheduler_test::Main::test_io_signal()
{
	scheduler.ready(contexts[BCK1]);
	scheduler.ready(contexts[BCK2]);
	scheduler.ready(contexts[BCK3]);
	scheduler.ready(contexts[APP1]);

	/* params:       time, curr, timeout, line */
	update_and_check(   0, APP1,     700, __LINE__);
	update_and_check( 700, BCK1,    1200, __LINE__);
	update_and_check(1200, APP1,    2200, __LINE__);
	update_and_check(2200, BCK2,    2700, __LINE__);
	scheduler.ready(contexts[DRV1]); /* irq occurred */
	update_and_check(2500, DRV1,    3700, __LINE__);
	timer.set_time(2700);
	scheduler.ready(contexts[MUL1]); /* signal occurred */
	scheduler.unready(contexts[DRV1]);
	update_and_check(2700, MUL1,    3650, __LINE__);
	timer.set_time(3000);
	scheduler.ready(contexts[APP2]); /* signal occurred */
	scheduler.unready(contexts[MUL1]);
	update_and_check(3500, APP2,    4000, __LINE__);
	timer.set_time(3700);
	scheduler.unready(contexts[APP2]);
	update_and_check(3700, BCK3,    4250, __LINE__);
	update_and_check(4700, APP1,    7100, __LINE__);
}


void Scheduler_test::Main::test_all_and_yield()
{
	scheduler.ready(contexts[BCK1]);
	scheduler.ready(contexts[BCK2]);
	scheduler.ready(contexts[BCK3]);
	scheduler.ready(contexts[APP1]);
	scheduler.ready(contexts[APP2]);
	scheduler.ready(contexts[APP3]);
	scheduler.ready(contexts[MUL1]);
	scheduler.ready(contexts[MUL2]);
	scheduler.ready(contexts[MUL3]);
	scheduler.ready(contexts[DRV1]);
	scheduler.ready(contexts[DRV2]);
	scheduler.ready(contexts[DRV3]);

	/* params:       time, curr, timeout, line */
	update_and_check(   0, DRV1,     500, __LINE__);
	update_and_check( 500, MUL1,    1000, __LINE__);
	update_and_check(1000, DRV2,    1500, __LINE__);
	update_and_check(1500, APP1,    2000, __LINE__);
	update_and_check(2000, MUL2,    2500, __LINE__);
	update_and_check(2500, BCK1,    3000, __LINE__);
	update_and_check(3000, DRV3,    3564, __LINE__);
	update_and_check(3600, MUL3,    4154, __LINE__);
	update_and_check(4200, APP2,    4700, __LINE__);
	update_and_check(4700, MUL2,    5200, __LINE__);
	update_and_check(5200, APP3,    5700, __LINE__);
	update_and_check(5700, DRV2,    6200, __LINE__);
	timer.set_time(5800);
	scheduler.yield();
	update_and_check(5800, DRV1,    6396, __LINE__);
	timer.set_time(5900);
	scheduler.yield();
	update_and_check(5900, MUL1,    6406, __LINE__);
	update_and_check(6500, BCK2,    7000, __LINE__);
	update_and_check(7000, DRV3,    7800, __LINE__);
	update_and_check(7800, APP3,    8300, __LINE__);
	timer.set_time(8000);
	scheduler.yield();
	update_and_check(8000, MUL3,    8656, __LINE__);
	update_and_check(8700, APP2,    9200, __LINE__);
}

void Scheduler_test::Main::test_destroy_helped()
{
	auto check = [&] (bool ok, char const *what, unsigned line_nr) {
		if (ok) return;
		error(what, " in line ", line_nr);
		env.parent().exit(-1);
	};

	/* two contexts help a third one, which is destroyed */
	Context &first = contexts[APP1], &second = contexts[APP2];
	{
		Context destination { Gids::APP, "destination" };

		first .help(destination);
		second.help(destination);

		check(&first.helping_destination()  == &destination &&
		      &second.helping_destination() == &destination,
		      "helpers do not execute their destination", __LINE__);
	}

	/* the pointers themselves: nothing of the destroyed context is read */
	check(first._destination == nullptr,
	      "first helper still helps the destroyed context", __LINE__);
	check(second._destination == nullptr,
	      "second helper still helps the destroyed context", __LINE__);
}


void Scheduler_test::Main::test_migrate()
{
	/* second CPU with its own timer, idle context, and scheduler */
	Kernel::Timer timer_b {};
	Context       idle_b { Gids::INVALID, "idle_b" };
	Scheduler     scheduler_b { timer_b, idle_b };
	configure(scheduler_b);

	auto update_b = [&] (time_t const time, Id const expected_current,
	                     unsigned const line_nr)
	{
		timer_b.set_time(time);
		scheduler_b.update();
		check(&scheduler_b.current() == &contexts[expected_current],
		      "wrong current context on scheduler B", line_nr);
	};

	/* APP1 and APP2 share scheduler A for a while */
	scheduler.ready(contexts[APP1]);
	scheduler.ready(contexts[APP2]);

	/* params:       time, curr, timeout, line */
	update_and_check(    0, APP1,     500, __LINE__);
	update_and_check(  500, APP2,    1400, __LINE__);
	update_and_check( 1500, APP1,    2500, __LINE__);
	update_and_check( 2500, APP2,    3500, __LINE__);
	update_and_check( 3500, APP1,    4500, __LINE__);

	/* APP3 runs alone on scheduler B, B's virtual times stay small */
	scheduler_b.ready(contexts[APP3]);
	update_b(  0, APP3, __LINE__);
	update_b(100, APP3, __LINE__);

	vtime_t const vtime_a = contexts[APP1]._vtime;

	/* APP1 leaves scheduler A while it is ready and current */
	timer.set_time(3600);
	scheduler.unready(contexts[APP1]);
	scheduler_b.join(contexts[APP1]);

	check(!lists(scheduler, contexts[APP1]), "A still lists APP1", __LINE__);
	check(!contexts[APP1].ready(), "APP1 still ready", __LINE__);
	check(contexts[APP1]._vtime == scheduler_b._groups[Gids::APP]._min_vtime,
	      "APP1 vtime is not B's group minimum", __LINE__);
	check(contexts[APP1]._vtime < vtime_a,
	      "APP1 keeps the vtime of scheduler A", __LINE__);
	check(contexts[APP1].execution_time() == 1600,
	      "APP1 lost its execution time", __LINE__);

	/* joining keeps it unready, B lists it only when it becomes ready */
	check(!lists(scheduler_b, contexts[APP1]), "B lists blocked APP1", __LINE__);
	update_b(150, APP3, __LINE__);

	/* A continues with APP2 alone */
	update_and_check(3600, APP2, scheduler._max_timeout + 3600, __LINE__);

	/* APP1 joins B and shares it with APP3 at once */
	scheduler_b.ready(contexts[APP1]);
	update_b( 200, APP1, __LINE__);
	update_b( 700, APP3, __LINE__);
	update_b(1700, APP1, __LINE__);
	check(!lists(scheduler, contexts[APP1]), "A lists APP1 again", __LINE__);

	/*
	 * A context that never ran (vtime 0, far below B's) joins B: it must
	 * not monopolise B, APP1 and APP3 get their turns as before
	 */
	Context app4 { Gids::APP, "app4" };
	scheduler_b.join(app4);
	check(app4._vtime == scheduler_b._groups[Gids::APP]._min_vtime,
	      "app4 vtime is not B's group minimum", __LINE__);
	check(app4._vtime > 0, "B's group minimum is still 0", __LINE__);
	scheduler_b.ready(app4);

	unsigned turns[3] { };
	for (time_t t = 1800; t <= 9800; t += 100) {
		timer_b.set_time(t);
		scheduler_b.update();
		Scheduler::Context const &c = scheduler_b.current();
		if (&c == &contexts[APP1]) turns[0]++;
		if (&c == &contexts[APP3]) turns[1]++;
		if (&c == &app4)           turns[2]++;
	}
	check(turns[0] && turns[1] && turns[2], "a context starved on B", __LINE__);
	check(turns[2] < 2*turns[0] && turns[2] < 2*turns[1],
	      "app4 monopolised B", __LINE__);

	scheduler_b.unready(app4);
	scheduler_b.unready(contexts[APP1]);
	scheduler_b.unready(contexts[APP3]);
}


void Scheduler_test::Main::test_timeout_move()
{
	using Kernel::Timeout;
	using Kernel::Timeout_list;

	/* timers of two CPUs, their time counts from different bases */
	Timeout_list a { }, b { };
	time_t const a_base = 1000, b_base = 50000;

	Timeout t1 { }, t2 { }, t3 { }, t4 { };

	check(a.set(t1, a_base + 300), "t1 is not first on A", __LINE__);
	check(a.set(t2, a_base + 100), "t2 is not first on A", __LINE__);
	check(b.set(t3, b_base + 200), "t3 is not first on B", __LINE__);

	/* 100 ticks later t1 has 200 left, it ends after t3 on B */
	check(!a.move(t1, a_base + 100, b, b_base + 100),
	      "t1 claims to be first on B", __LINE__);
	check(t1.listed() && t1.end() == b_base + 300,
	      "t1 lost its remaining ticks", __LINE__);

	/* t2 is due, it becomes B's first and B must program its timer */
	check(a.move(t2, a_base + 100, b, b_base + 100),
	      "due t2 is not first on B", __LINE__);
	check(t2.end() == b_base + 100, "due t2 is not due on B", __LINE__);

	check(!a.first(), "A still lists a moved timeout", __LINE__);
	check(b.first() == &t2, "B's first is not t2", __LINE__);
	b.remove(t2);
	check(b.first() == &t3, "B's second is not t3", __LINE__);
	b.remove(t3);
	check(b.first() == &t1, "B's third is not t1", __LINE__);
	b.set(t2, b_base + 100);
	b.set(t3, b_base + 200);

	/* an unlisted timeout stays unlisted */
	check(!a.move(t4, a_base, b, b_base), "unlisted t4 moved", __LINE__);
	check(!t4.listed(), "unlisted t4 got listed", __LINE__);

	/* moving back keeps the remaining ticks too, and A gets a new first */
	check(b.move(t1, b_base + 250, a, a_base + 400),
	      "t1 is not first on empty A", __LINE__);
	check(t1.end() == a_base + 450, "t1 lost its ticks moving back", __LINE__);

	a.remove(t1);
	b.remove(t2);
	b.remove(t3);
}


void Scheduler_test::Main::test_helping_time()
{
	/*
	 * APP1 is a client, APP2 a server waiting for a request. The server's
	 * virtual time is ahead, so the scheduler keeps the client current
	 * and the client executes the server while it helps.
	 */
	Context &client = contexts[APP1], &server = contexts[APP2];
	server._vtime = 10000;

	scheduler.ready(client);
	update_and_check(0, APP1, scheduler._max_timeout, __LINE__);

	/* the client computes for 300, then calls the server and helps it */
	timer.set_time(300);
	scheduler.ready(server);
	scheduler.help(client, server);
	timer.set_time(300);
	scheduler.update();
	check(&current() == &client, "the client is not current", __LINE__);
	check(&scheduler.current().helping_destination() == &server,
	      "the client does not execute the server", __LINE__);

	/* the server works for 200 and replies, the client continues for 100 */
	timer.set_time(500);
	scheduler.helping_finished(client);
	scheduler.unready(server);
	timer.set_time(600);
	scheduler.update();
	scheduler.unready(client);

	log("helping: execution time client ", client.execution_time(),
	    " server ", server.execution_time());

	check(client.execution_time() == 400,
	      "client time is not its own computation (300 + 100)", __LINE__);
	check(server.execution_time() == 200,
	      "server time is not the time it worked (200)", __LINE__);

	/* the scheduling share stays the client's: it donated it */
	check(client._vtime == 600, "client vtime does not cover its share", __LINE__);
	check(server._vtime == 10000, "server vtime changed while helped", __LINE__);
}


void Component::construct(Env &env)
{
	{
		Scheduler_test::Main main { env };
		main.test_background_idle();
	}

	{
		Scheduler_test::Main main { env };
		main.test_one_per_group();
	}

	{
		Scheduler_test::Main main { env };
		main.test_io_signal();
	}

	{
		Scheduler_test::Main main { env };
		main.test_all_and_yield();
	}

	{
		Scheduler_test::Main main { env };
		main.test_destroy_helped();
		main.test_migrate();
	}

	{
		Scheduler_test::Main main { env };
		main.test_timeout_move();
	}

	{
		Scheduler_test::Main main { env };
		main.test_helping_time();
	}


	env.parent().exit(0);
}
