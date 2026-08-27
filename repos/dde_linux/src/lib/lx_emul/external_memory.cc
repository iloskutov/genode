/*
 * \brief  Lx_emul backend for externally managed memory
 * \author Stefan Kalkowski
 * \date   2026-08-27
 */

/*
 * Copyright (C) 2026 Genode Labs GmbH
 *
 * This file is distributed under the terms of the GNU General Public License
 * version 2 or later.
 */

#include <lx_kit/env.h>
#include <lx_emul/page_virt.h>
#include <lx_emul/external_memory.h>


extern "C" void
lx_emul_external_memory_add(void *bus_addr, unsigned long size, void *virt_addr)
{
	Lx_kit::env().external_memory.add(bus_addr, size, virt_addr,
	                                  &lx_emul_add_page_range);
}


extern "C" void
lx_emul_external_memory_remove(void *virt_addr)
{
	Lx_kit::env().external_memory.remove(virt_addr, &lx_emul_remove_page_range);
}
