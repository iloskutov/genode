/*
 * \brief  Intel XE brightness support
 * \author Alexander Boettcher
 * \date   2022-03-08
 */

/*
 * Copyright (C) 2026 Genode Labs GmbH
 *
 * This file is distributed under the terms of the GNU General Public License
 * version 2.
 */

#define KBUILD_MODNAME "genode_xe_user_driver"

#include <drm/drm_connector.h>

#include "xe_assert.h"

#include "lx_brightness.h"


void lx_xe_set_brightness(unsigned brightness, struct drm_connector * connector)
{
	lx_set_brightness(brightness, connector);
}


unsigned lx_xe_get_brightness(struct drm_connector * const connector,
                              unsigned const brightness_error)
{
	return lx_get_brightness(connector, brightness_error);
}
