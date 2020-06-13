/*
 * \brief  Gpio driver for the Alwinner A64
 * \author Ivan Loskutov <ivan.loskutov@ksyslabs.org>
 * \date
 */

/*
 * Copyright (C) 2012-2017 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

#ifndef _DRIVERS__GPIO__SPEC__AA64__DRIVER_H_
#define _DRIVERS__GPIO__SPEC__AA64__DRIVER_H_

/* Genode includes */
#include <drivers/defs/pinephone.h>
#include <gpio/driver.h>
#include <base/attached_io_mem_dataspace.h>
#include <irq_session/connection.h>
#include <timer_session/connection.h>

/* local includes */
#include "gpio.h"


class AA64_driver : public Gpio::Driver
{
	private:

		enum {
			PIN_SHIFT = 5,
			MAX_BANKS = 12,
			MAX_PINS  = 32
		};

		class Gpio_bank
		{
			private:
				Gpio_reg                             _reg;

			public:
				Gpio_bank(Genode::addr_t base, unsigned idx)
				: _reg(base, idx)
				{ }

				Gpio_reg* regs() { return &_reg; }
		};

		Genode::Attached_io_mem_dataspace  _mmio;
		Genode::Attached_io_mem_dataspace  _mmioL;

		// A ?
		Gpio_bank _gpio_bank_B;
		Gpio_bank _gpio_bank_C;
		Gpio_bank _gpio_bank_D;
		Gpio_bank _gpio_bank_E;
		Gpio_bank _gpio_bank_F;
		Gpio_bank _gpio_bank_G;
		Gpio_bank _gpio_bank_H;
		// I ?
		// J ?
		// K ?
		Gpio_bank _gpio_bank_L;

		Gpio_bank *_gpio_bank(int gpio)
		{
			switch (gpio >> PIN_SHIFT) {
			case 1:
				return &_gpio_bank_B;
			case 2:
				return &_gpio_bank_C;
			case 3:
				return &_gpio_bank_D;
			case 4:
				return &_gpio_bank_E;
			case 5:
				return &_gpio_bank_F;
			case 6:
				return &_gpio_bank_G;
			case 7:
				return &_gpio_bank_H;
			case 11:
				return &_gpio_bank_L;
			}

			Genode::error("no Gpio_bank for pin ", gpio, " available");
			return 0;
		}

		int _gpio_index(int gpio)       { return gpio & 0x1f; }

		AA64_driver(Genode::Env &env)
		:
			_mmio(env, Pinephone::PIO_MMIO_BASE, Pinephone::PIO_MMIO_SIZE),
			_mmioL(env, Pinephone::PIO_L_MMIO_BASE, Pinephone::PIO_L_MMIO_SIZE),
			_gpio_bank_B((Genode::addr_t)_mmio.local_addr<void>(), 1),
			_gpio_bank_C((Genode::addr_t)_mmio.local_addr<void>(), 2),
			_gpio_bank_D((Genode::addr_t)_mmio.local_addr<void>(), 3),
			_gpio_bank_E((Genode::addr_t)_mmio.local_addr<void>(), 4),
			_gpio_bank_F((Genode::addr_t)_mmio.local_addr<void>(), 5),
			_gpio_bank_G((Genode::addr_t)_mmio.local_addr<void>(), 6),
			_gpio_bank_H((Genode::addr_t)_mmio.local_addr<void>(), 7),
			_gpio_bank_L((Genode::addr_t)_mmioL.local_addr<void>(), 0)
		{ }

	public:

		static AA64_driver& factory(Genode::Env &env);


		/******************************
		 **  Gpio::Driver interface  **
		 ******************************/

		void direction(unsigned gpio, bool input) override
		{
			Gpio_reg *gpio_reg = _gpio_bank(gpio)->regs();
			gpio_reg->write<Gpio_reg::Cfg>(input ? 0 : 1, _gpio_index(gpio));
		}

		void write(unsigned gpio, bool level) override
		{
			Gpio_reg *gpio_reg = _gpio_bank(gpio)->regs();
			gpio_reg->write<Gpio_reg::Dat>(level, _gpio_index(gpio));
		}

		bool read(unsigned gpio) override
		{
			Gpio_reg *gpio_reg = _gpio_bank(gpio)->regs();
			return gpio_reg->read<Gpio_reg::Dat>(_gpio_index(gpio));
		}

		void debounce_enable(unsigned, bool) override
		{
		}

		void debounce_time(unsigned, unsigned long) override
		{
		}
		void falling_detect(unsigned) override
		{
		}

		void rising_detect(unsigned) override
		{
		}

		void high_detect(unsigned) override
		{
		}

		void low_detect(unsigned) override
		{
		}

		void irq_enable(unsigned, bool) override
		{
		}

		void ack_irq(unsigned) override
		{
		}

		void register_signal(unsigned,
		                     Genode::Signal_context_capability) override
		{}

		void unregister_signal(unsigned) override
		{
		}

		bool gpio_valid(unsigned gpio) override { return gpio < (MAX_PINS*MAX_BANKS); }
};

#endif /* _DRIVERS__GPIO__SPEC__AA64__DRIVER_H_ */
