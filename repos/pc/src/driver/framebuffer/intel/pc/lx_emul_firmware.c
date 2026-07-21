/*
 * \brief  Linux emulation environment firmware handling
 * \author Alexander Boettcher
 * \date   2026-08-05
 */

/*
 * Copyright (C) 2026 Genode Labs GmbH
 *
 * This file is distributed under the terms of the GNU General Public License
 * version 2 or later.
 */

/* lx emul/kit includes */
#include <lx_emul.h>
#include <lx_emul/firmware.h>

/* Linux includes */
#include <linux/version.h>
#include <linux/firmware.h>


int request_firmware_direct(const struct firmware ** firmware_p,
                            const char * name,
                            struct device * device)
{
	lx_emul_trace_and_stop(__func__);
	return -1;
}


int request_firmware_common(const struct firmware **firmware_p,
                            const char *name, struct device *device,
                            bool warn)
{
	struct firmware *fw;

	if (!firmware_p)
		return -1;

	fw = kzalloc(sizeof(struct firmware), GFP_KERNEL);

	if (lx_emul_request_firmware_nowait(name, &fw->data, &fw->size, warn)) {
		kfree(fw);
		return -1;
	}

	*firmware_p = fw;
	return 0;
}


int request_firmware(const struct firmware **firmware_p,
                     const char *name, struct device *device)
{
	return request_firmware_common(firmware_p, name, device, true);
}


void release_firmware(const struct firmware * fw)
{
	if (!fw)
		return;

	lx_emul_release_firmware(fw->data, fw->size);
	kfree(fw);
}


int firmware_request_nowarn(const struct firmware **firmware,
                            const char *name, struct device *device)
{
	return request_firmware_common(firmware, name, device, false);
}
