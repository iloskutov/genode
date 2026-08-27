/*
 * \brief  Lx_emul support to rgister externally managed memory
 * \author Stefan Kalkowski
 * \date   2026-08-27
 */

/*
 * Copyright (C) 2026 Genode Labs GmbH
 *
 * This file is distributed under the terms of the GNU General Public License
 * version 2 or later.
 */

#ifndef _LX_EMUL__EXTERNAL_MEMORY_H_
#define _LX_EMUL__EXTERNAL_MEMORY_H_

#include <genode_c_api/base.h>

#ifdef __cplusplus
extern "C" {
#endif

void lx_emul_external_memory_add(void *bus_addr, unsigned long size,
                                 void *virt_addr);

void lx_emul_external_memory_remove(void *virt_addr);

#ifdef __cplusplus
}
#endif

#endif /* _LX_EMUL__EXTERNAL_MEMORY_H_ */

