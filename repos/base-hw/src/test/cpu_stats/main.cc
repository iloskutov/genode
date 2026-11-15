/*
 * \brief  Test of the CPU idle records ('Cpu_control::Cpu_stats')
 * \author Ivan Loskutov
 * \date   2026-11-15
 *
 * The test takes the dataspace of its Cpu_control session. It spins a
 * thread on CPU 1 for a second and requires CPU 1 to be idle for at most
 * 10 % of that second and CPU 3, which runs nothing of the test, for at
 * least 80 %.
 */

/*
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

#include <base/attached_dataspace.h>
#include <base/component.h>
#include <base/log.h>
#include <base/thread.h>
#include <timer_session/connection.h>
#include <cpu_control_session/connection.h>
#include <cpu_control_session/cpu_stats.h>

namespace Test {
	using namespace Genode;
	struct Spinner;
	struct Main;

	static uint64_t counter()
	{
		uint64_t v;
		asm volatile ("isb; mrs %0, cntpct_el0" : "=r" (v));
		return v;
	}
}


struct Test::Spinner : Thread
{
	bool volatile stop = false;

	Spinner(Env &env, Affinity::Location loc)
	: Thread(env, "spinner", Stack_size { 8*1024 }, loc) { }

	void entry() override { while (!stop) { } }
};


struct Test::Main
{
	Env &_env;

	Timer::Connection       _timer       { _env };
	Cpu_control::Connection _cpu_control { _env };

	void _fail(char const *what)
	{
		error(what);
		log("--- test failed ---");
		_env.parent().exit(1);
	}

	Main(Env &env) : _env(env)
	{
		Dataspace_capability const cap = _cpu_control.cpu_stats();

		if (!cap.valid()) { _fail("no records"); return; }

		Attached_dataspace ds { _env.rm(), cap };
		Cpu_control::Cpu_stats const &stats = *ds.local_addr<Cpu_control::Cpu_stats const>();

		if (!stats.counter_hz) { _fail("counter frequency is 0"); return; }
		log("counter at ", stats.counter_hz, " Hz");

		Spinner spinner { _env, Affinity::Location(1, 0) };

		auto idle = [&] (unsigned cpu) {
			uint64_t ticks = 0;
			if (!stats.idle_ticks(cpu, counter(), ticks))
				_fail("no consistent record");
			return ticks;
		};

		uint64_t const t0 = counter();
		uint64_t const busy_0 = idle(1), quiet_0 = idle(3);

		spinner.start();
		_timer.msleep(1000);

		uint64_t const t1 = counter();
		uint64_t const busy_1 = idle(1), quiet_1 = idle(3);

		spinner.stop = true;
		spinner.join();

		uint64_t const period = t1 - t0;
		unsigned const busy_idle  = unsigned((busy_1  - busy_0 )*100/period);
		unsigned const quiet_idle = unsigned((quiet_1 - quiet_0)*100/period);

		log("idle during the second: CPU 1 (spinning) ", busy_idle,
		    " %, CPU 3 ", quiet_idle, " %");

		if (busy_idle > 10)  { _fail("spinning CPU 1 counted as idle"); return; }
		if (quiet_idle < 80) { _fail("quiet CPU 3 not counted as idle"); return; }

		log("--- test succeeded ---");
	}
};


void Component::construct(Genode::Env &env) { static Test::Main main(env); }
