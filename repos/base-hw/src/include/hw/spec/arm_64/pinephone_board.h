/*
 * \brief  Board definitions for Pinephone
 * \author Stefan Kalkowski
 * \date   2019-06-12
 */

/*
 * Copyright (C) 2019 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

#ifndef _SRC__INCLUDE__HW__SPEC__ARM_64__PINEPHONE__BOARD_H_
#define _SRC__INCLUDE__HW__SPEC__ARM_64__PINEPHONE__BOARD_H_

#include <drivers/uart/dw_apb.h>
#include <hw/spec/arm/boot_info.h>

namespace Hw::Pinephone_board {
	using Serial = Genode::Dw_apb_uart;

	enum {
		// atf PLAT_SUNXI_NS_IMAGE_OFFSET ???
		RAM_BASE   = 0x4a000000,
		RAM_SIZE   = 0x76000000,

		UART_BASE  = 0x01c28000,
		UART_SIZE  = 0x400,
		UART_CLOCK = 25000000,

		CACHE_LINE_SIZE_LOG2 = 6,
	};

	namespace Cpu_mmio {
		enum {
			IRQ_CONTROLLER_DISTR_BASE  = 0x01c81000,
			IRQ_CONTROLLER_DISTR_SIZE  = 0x1000,
			IRQ_CONTROLLER_CPU_BASE    = 0x01c82000,
			IRQ_CONTROLLER_CPU_SIZE    = 0x2000,
			IRQ_CONTROLLER_VT_CTRL_BASE = 0x01c84000,
			IRQ_CONTROLLER_VT_CPU_BASE = 0x01c86000,
			IRQ_CONTROLLER_VT_CPU_SIZE = 0x2000,
		};
	};
};

#endif /* _SRC__INCLUDE__HW__SPEC__ARM_64__PINEPHONE__BOARD_H_ */
