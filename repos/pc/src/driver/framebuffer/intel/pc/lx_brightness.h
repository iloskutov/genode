/**
 * \brief  Brightness interface shared between Intel/i915 and Intel/Xe
 * \author Alexander Boettcher
 * \date   2026-09-03
 */

/*
 * Copyright (C) 2026 Genode Labs GmbH
 *
 * This file is distributed under the terms of the GNU General Public License
 * version 2.
 */

#ifndef _LX_BRIGHTNESS_H_
#define _LX_BRIGHTNESS_H_


#include <linux/backlight.h>

#include "display/intel_backlight.h"
#include "display/intel_display_types.h"


enum { MAX_BRIGHTNESS = 100, INVALID_BRIGHTNESS = MAX_BRIGHTNESS + 1 };


static void lx_set_brightness(unsigned brightness, struct drm_connector * connector)
{
	struct intel_connector * intel_c = to_intel_connector(connector);
	if (intel_c)
		intel_backlight_set_acpi(intel_c->base.state, brightness, MAX_BRIGHTNESS);
}


static unsigned lx_get_brightness(struct drm_connector * const connector,
                                  unsigned const brightness_error)
{
	struct intel_connector * intel_c = NULL;
	struct intel_panel     * panel   = NULL;
	unsigned ret;

	if (!connector)
		return brightness_error;

	intel_c = to_intel_connector(connector);
	if (!intel_c)
		return brightness_error;

	panel = &intel_c->panel;

	if (!panel || !panel->backlight.device || !panel->backlight.device->ops ||
	    !panel->backlight.device->ops->get_brightness)
		return brightness_error;

	ret = panel->backlight.device->ops->get_brightness(panel->backlight.device);

	/* in percentage */
	return ret * MAX_BRIGHTNESS / panel->backlight.device->props.max_brightness;
}


static void lx_i915_set_brightness(unsigned brightness, struct drm_connector * connector)
{
	lx_set_brightness(brightness, connector);
}


static unsigned lx_i915_get_brightness(struct drm_connector * const connector,
                                       unsigned const brightness_error)
{
	return lx_get_brightness(connector, brightness_error);
}


/* provided by lx_xe_brightness.c in pc_intel_xe.so */
void lx_xe_set_brightness(unsigned brightness, struct drm_connector * connector);
unsigned lx_xe_get_brightness(struct drm_connector * const connector, unsigned const brightness_error);


#endif
