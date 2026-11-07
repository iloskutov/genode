/*
 * \brief  Test that the kernel preserves tpidr_el0 per thread
 * \author Ivan Loskutov
 * \date   2026-09-25
 *
 * Android (bionic) code reads its thread pointer from tpidr_el0 in almost
 * every function (stack-protector canary, ELF TLS), so each thread must keep
 * its own value across preemption and kernel calls.
 */

/*
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

#include <base/component.h>
#include <base/log.h>
#include <base/thread.h>

using namespace Genode;


static void write_tpidr(addr_t v) { asm volatile ("msr tpidr_el0, %0" :: "r"(v)); }

static addr_t read_tpidr()
{
	addr_t v;
	asm volatile ("mrs %0, tpidr_el0" : "=r"(v));
	return v;
}


/**
 * Set a unique value, then check it while being preempted and doing
 * kernel calls. Returns the number of mismatches seen.
 */
static unsigned check_tpidr(Env &env, addr_t value, unsigned rounds)
{
	write_tpidr(value);

	unsigned errors = 0;
	for (unsigned i = 0; i < rounds; i++) {

		/* spin long enough to be preempted by the other threads */
		for (unsigned j = 0; j < 200'000; j++) {
			asm volatile ("" ::: "memory");
			if (read_tpidr() != value) { errors++; write_tpidr(value); }
		}

		/* kernel entry/exit via an RPC to core */
		(void)env.pd().ram_size(Ram_dataspace_capability());
		if (read_tpidr() != value) { errors++; write_tpidr(value); }
	}
	return errors;
}


struct Checker : Thread
{
	Env           &_env;
	addr_t   const _value;
	unsigned       errors { 0 };

	Checker(Env &env, char const *name, addr_t value)
	: Thread(env, name, Stack_size { 16*1024 }), _env(env), _value(value) { }

	void entry() override { errors = check_tpidr(_env, _value, 50); }
};


void Component::construct(Env &env)
{
	log("--- tpidr_el0 test ---");

	Checker a(env, "checker_a", 0xaaaa0000aaaa0000);
	Checker b(env, "checker_b", 0xbbbb0000bbbb0000);
	a.start();
	b.start();

	unsigned const main_errors = check_tpidr(env, 0xcccc0000cccc0000, 50);

	a.join();
	b.join();

	log("mismatches: main=", main_errors, " a=", a.errors, " b=", b.errors);

	if (main_errors || a.errors || b.errors)
		error("tpidr_el0 is not preserved per thread");
	else
		log("tpidr_el0 test passed");
}
