/*
 * \brief  UART driver for the DesignWire 8250
 * \author Martin stein
 * \date   2011-10-17
 */

/* TODO: Based on tl16c750.h. Can it be merged? */

/*
 * Copyright (C) 2011-2017 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

#ifndef _INCLUDE__DRIVERS__UART__DW_APB_H_
#define _INCLUDE__DRIVERS__UART__DW_APB_H_

/* Genode includes */
#include <util/mmio.h>

namespace Genode { class Dw_apb_uart; }

class Genode::Dw_apb_uart: public Mmio
{
	protected:
		/**
		* Transmit holding register
		*/
		struct Uart_thr : Register<0x0, 32>
		{
			struct Thr : Bitfield<0, 8> { };
		};

		/**
		* Line status register
		*/
		struct Uart_lsr : Register<0x14, 32>
		{
			struct Rx_fifo_empty : Bitfield<0, 1> { };
			struct Tx_fifo_empty : Bitfield<5, 1> { };
		};

	public:

		Dw_apb_uart(addr_t const base, unsigned /* clock */,
		         unsigned const /* baud_rate */)
		: Mmio(base)
		{
			/* TODO: Initialize serial port. */
		}

		void put_char(char const c)
		{
			/* wait as long as the transmission buffer is full */
			while (!read<Uart_lsr::Tx_fifo_empty>()) ;

			/* transmit character */
			write<Uart_thr::Thr>(c);
		}
};

#endif /* _INCLUDE__DRIVERS__UART__DW_APB_H_ */
