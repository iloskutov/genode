/*
 * \brief  Lx_emul support for firmware loading
 * \author Josef Soentgen
 * \date   2026-09-21
 */

/*
 * Copyright (C) 2026 Genode Labs GmbH
 *
 * This file is distributed under the terms of the GNU General Public License
 * version 2 or later.
 */

#ifndef _LX_EMUL__FIRMWARE_H_
#define _LX_EMUL__FIRMWARE_H_

#ifdef __cplusplus
extern "C" {
#endif

int  lx_emul_request_firmware_nowait(const char *name, void *dest, size_t *result, bool warn);
void lx_emul_release_firmware(void const *data, size_t size);

#ifdef __cplusplus
}
#endif

#endif /* _LX_EMUL__FIRMWARE_H_ */
