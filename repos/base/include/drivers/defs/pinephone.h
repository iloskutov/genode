/*
 * \brief  MMIO and IRQ definitions for the Pinephone
 * \author
 * \date
 */

#ifndef _INCLUDE__DRIVERS__DEFS__PINEPHONE_H_
#define _INCLUDE__DRIVERS__DEFS__PINEPHONE_H_

namespace Pinephone {
	enum {
		/* PIO */
		PIO_MMIO_BASE = 0x01c20800,
		PIO_MMIO_SIZE = 0x400,
		/* PIO L */
		PIO_L_MMIO_BASE = 0x01f02c00,
		PIO_L_MMIO_SIZE = 0x400,
	};
};

#endif /* _INCLUDE__DRIVERS__DEFS__PINEPHONE_H_ */
