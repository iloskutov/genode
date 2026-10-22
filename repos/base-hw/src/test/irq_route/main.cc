/*
 * \brief  Route a shared peripheral interrupt to the CPU of its handler
 * \author Ivan Loskutov
 * \date   2026-10-22
 *
 * The alarm of Qemu's PL031 RTC raises SPI 34. For each CPU, an
 * entrypoint on that CPU opens the IRQ session, acknowledges the IRQ and
 * arms the alarm twice. The kernel learns the handler's CPU from the
 * first signal; at the second IRQ, the GICv3 distributor must route it
 * to that CPU (IROUTER, affinity Aff0 on Qemu's virt machine).
 */

/*
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

/* Genode includes */
#include <base/attached_io_mem_dataspace.h>
#include <base/component.h>
#include <base/log.h>
#include <irq_session/connection.h>
#include <util/mmio.h>

namespace Test {

	using namespace Genode;

	struct Rtc;
	struct Gicd;
	struct Handler;
	struct Main;

	enum { RTC_BASE = 0x09010000, GICD_BASE = 0x08000000, IRQ = 34 };
}


/**
 * PL031 real-time clock with its alarm
 */
struct Test::Rtc : Attached_io_mem_dataspace, Mmio<0x20>
{
	struct Dr   : Register<0x00, 32> { };
	struct Mr   : Register<0x04, 32> { };
	struct Imsc : Register<0x10, 32> { };
	struct Icr  : Register<0x1c, 32> { };

	Rtc(Env &env)
	:
		Attached_io_mem_dataspace(env, RTC_BASE, 0x1000),
		Mmio({local_addr<char>(), 0x1000})
	{ }

	void arm()
	{
		write<Icr>(1);
		write<Mr>(read<Dr>() + 1);
		write<Imsc>(1);
	}

	void clear()
	{
		write<Imsc>(0);
		write<Icr>(1);
	}
};


/**
 * Distributor of the GICv3, IROUTER only
 */
struct Test::Gicd : Attached_io_mem_dataspace, Mmio<0x10000>
{
	struct Irouter : Register_array<0x6000, 64, 1020, 64, true> { };

	Gicd(Env &env)
	:
		Attached_io_mem_dataspace(env, GICD_BASE, 0x10000),
		Mmio({local_addr<char>(), 0x10000})
	{ }

	uint64_t route(unsigned irq) { return read<Irouter>(irq); }
};


/**
 * Entrypoint on one CPU that acknowledges the IRQ and waits for it
 */
struct Test::Handler
{
	Env           &_env;
	Main          &_main;
	unsigned const _cpu;
	Entrypoint     _ep;
	unsigned       _irqs = 0;

	Constructible<Irq_connection> _session { };

	Io_signal_handler<Handler> _start { _ep, *this, &Handler::_handle_start };
	Io_signal_handler<Handler> _irq   { _ep, *this, &Handler::_handle_irq };

	void _handle_start();
	void _handle_irq();

	Handler(Env &env, Main &main, unsigned cpu, Affinity::Location location)
	:
		_env(env), _main(main), _cpu(cpu),
		_ep(env, 16*1024, "irq_ep", location)
	{ }

	void start() { Signal_transmitter(_start).submit(); }

	void close() { _session.destruct(); }
};


struct Test::Main
{
	Env &_env;

	Rtc  _rtc  { _env };
	Gicd _gicd { _env };

	Affinity::Space const _space { _env.cpu().affinity_space() };

	unsigned const _cpus { _space.width() };

	/* entrypoints are kept, one per CPU */
	Constructible<Handler> _handler[4] { };

	unsigned _cpu    = 0;
	unsigned _failed = 0;

	Signal_handler<Main> _next { _env.ep(), *this, &Main::_handle_next };

	void _start(unsigned const cpu)
	{
		_handler[cpu].construct(_env, *this, cpu, _space.location_of_index(int(cpu)));
		_handler[cpu]->start();
	}

	void _handle_next()
	{
		_handler[_cpu]->close();

		if (++_cpu < _cpus && _cpu < 4) {
			_start(_cpu);
			return;
		}
		log("--- test ", _failed ? "failed" : "succeeded", " ---");
	}

	void done(unsigned const cpu, uint64_t const route)
	{
		bool const ok = (route == cpu);
		log("IRQ on CPU ", cpu, ": route ", Hex(route), ok ? "" : " (wrong)");
		if (!ok) _failed++;
		Signal_transmitter(_next).submit();
	}

	Main(Env &env) : _env(env)
	{
		log("route at start ", Hex(_gicd.route(IRQ)), ", ", _cpus, " CPUs");
		_start(0);
	}
};


void Test::Handler::_handle_start()
{
	_session.construct(_env, unsigned(IRQ));
	_session->sigh(_irq);
	_session->ack_irq();
	_main._rtc.arm();
}


void Test::Handler::_handle_irq()
{
	_main._rtc.clear();

	if (++_irqs == 1) {
		_session->ack_irq();
		_main._rtc.arm();
		return;
	}
	_main.done(_cpu, _main._gicd.route(IRQ));
}


void Component::construct(Genode::Env &env) { static Test::Main main(env); }
