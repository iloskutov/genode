/*
 * \brief  Move a thread blocked on its kernel timeout to another CPU
 * \author Ivan Loskutov
 * \date   2026-10-16
 *
 * A sleeper thread waits on a kernel timeout of its own. While the
 * timeout is pending, the sleeper is moved to another CPU, mostly an idle
 * one. The timeout must still end in time, the new CPU has to program its
 * timer for it.
 */

/*
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

/* Genode includes */
#include <base/component.h>
#include <base/log.h>
#include <base/signal.h>
#include <base/thread.h>
#include <cpu_thread/client.h>

/* base-hw includes */
#include <kernel/interface.h>

namespace Test {

	using namespace Genode;

	struct Kernel_timeout;
	struct Sleeper;
	struct Main;
}


/**
 * Kernel timeout of the calling thread with a signal receiver of its own
 */
struct Test::Kernel_timeout
{
	Signal_receiver receiver { };
	Signal_context  context  { };

	Signal_context_capability const cap = receiver.manage(context);

	Kernel::capid_t const sel = Kernel::capid_t(addr_t(cap.data()) & 0xffffu);

	~Kernel_timeout() { receiver.dissolve(context); }

	void sleep(uint64_t const us)
	{
		Kernel::timeout(Kernel::timeout_t(us), sel);
		receiver.wait_for_signal();
	}
};


struct Test::Sleeper : Thread
{
	enum { SLEEP_US = 20*1000 };

	Blockade go   { };
	Blockade done { };

	uint64_t volatile start_us { 0 };
	uint64_t volatile took_us  { 0 };

	Sleeper(Env &env) : Thread(env, "sleeper", Stack_size { 16*1024 }) { }

	void entry() override
	{
		Kernel_timeout timeout { };

		while (true) {
			go.block();

			start_us = Kernel::time();
			timeout.sleep(SLEEP_US);
			took_us = Kernel::time() - start_us;

			done.wakeup();
		}
	}
};


struct Test::Main
{
	enum { MOVE_AFTER_US = 5*1000, MAX_LATE_US = 2*1000 };

	Env &env;

	Sleeper sleeper { env };

	Kernel_timeout delay { };

	struct Move { unsigned from, to; };

	void move(unsigned const cpu)
	{
		Cpu_thread_client(sleeper.cap())
			.affinity(env.cpu().affinity_space().location_of_index(cpu));
	}

	bool test(Move const m)
	{
		move(m.from);

		sleeper.go.wakeup();
		delay.sleep(MOVE_AFTER_US);

		uint64_t const move_us = Kernel::time();
		move(m.to);

		sleeper.done.block();

		uint64_t const pending_us = move_us - sleeper.start_us;
		uint64_t const late_us    = sleeper.took_us > Sleeper::SLEEP_US
		                          ? sleeper.took_us - Sleeper::SLEEP_US : 0;

		if (pending_us >= Sleeper::SLEEP_US) {
			error("move ", m.from, "->", m.to, " came after the timeout");
			return false;
		}

		if (late_us > MAX_LATE_US) {
			error("move ", m.from, "->", m.to, " ", pending_us, " us after "
			      "the start: timeout ended ", late_us, " us late");
			return false;
		}

		log("move ", m.from, "->", m.to, ": timeout in time");
		return true;
	}

	Main(Env &env) : env(env)
	{
		sleeper.start();

		/* CPU 0 runs this test's entrypoint and core's entrypoint */
		Move const moves[] = { { 1, 2 }, { 2, 3 }, { 3, 1 },
		                       { 1, 0 }, { 0, 2 } };

		bool ok = true;
		for (unsigned round = 0; round < 3; round++)
			for (Move const m : moves)
				ok = test(m) && ok;

		log(ok ? "--- test succeeded ---" : "--- test failed ---");
	}
};


void Component::construct(Genode::Env &env) { static Test::Main main(env); }
