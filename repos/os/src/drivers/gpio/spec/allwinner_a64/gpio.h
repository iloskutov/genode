/*
 * \brief  Allwinner A64 GPIO definitions
 * \author Ivan Loskutov <ivan.loskutov@ksyslabs.org>
 * \date
 */

/*
 * Copyright (C) 2012-2020 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

#ifndef _DRIVERS__GPIO__SPEC__ALLWINNER_A64__GPIO_H_
#define _DRIVERS__GPIO__SPEC__ALLWINNER_A64__GPIO_H_

/* Genode includes */
#include <util/mmio.h>

struct Gpio_reg : Genode::Mmio
{
	Gpio_reg(Genode::addr_t const mmio_base, unsigned idx)
	: Genode::Mmio(mmio_base + 0x24 * idx) { }

	struct Cfg   : Register_array<0x00, 32, 32, 4> {};
	struct Dat   : Register_array<0x10, 32, 32, 1> {};
	struct Drv   : Register_array<0x14, 32, 32, 2> {};
	struct Pul0  : Register_array<0x1c, 32, 32, 2> {};
};

#endif /* _DRIVERS__GPIO__SPEC__ALLWINNER_A64__GPIO_H_ */
