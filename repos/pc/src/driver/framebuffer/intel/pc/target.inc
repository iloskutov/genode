REQUIRES := x86

REL_PRG_DIR := $(PRG_DIR)/../..

TARGET  := pc_intel_fb
LIBS    := base blit jitterentropy vfs
LIBS    += pc_lx_emul pc_intel_xe

INC_DIR += $(REL_PRG_DIR) $(REL_PRG_DIR)/shadow

SRC_CC  += main.cc
SRC_CC  += emul.cc
SRC_CC  += opregion_io_mem.cc
SRC_CC  += lx_emul/shared_dma_buffer.cc
SRC_CC  += lx_kit/firmware.cc
SRC_C   += dummies.c
SRC_C   += dummies_xe.c
SRC_C   += generated_dummies_xe.c
SRC_C   += pci.c
SRC_C   += lx_emul.c
SRC_C   += $(notdir $(wildcard $(REL_PRG_DIR)/generated_dummies.c))
SRC_C   += lx_user.c
SRC_C   += gem.c
SRC_C   += xe_migrate.c
SRC_C   += intel_gt.c
SRC_C   += lx_emul_firmware.c
SRC_C   += lx_emul/common_dummies.c
SRC_C   += lx_emul/shadow/mm/page_alloc.c
SRC_C   += lx_emul/shadow/drivers/char/random.c

vpath %.c  $(REL_PRG_DIR)
vpath %.cc $(REL_PRG_DIR)

vpath %.c  $(REP_DIR)/src/lib/pc
vpath %.cc $(REP_DIR)/src/lib/pc

#
# Defined by contrib/linux/src/linux/drivers/gpu/drm/i915/Makefile via
#   subdir-ccflags-y += -DI915
#
# Switch to compile/reuse files for i915 and xe
CC_C_OPT += -DI915

INC_DIR    += $(LX_SRC_DIR)/drivers/gpu/drm/i915
INC_DIR    += $(LX_SRC_DIR)/drivers/gpu/drm

#
# contrib/linux/src/linux/drivers/gpu/drm/xe/Makefile
#
CC_OPT_drivers/gpu/drm/xe/xe_module += -I$(LX_SRC_DIR)/drivers/gpu/drm/xe
CC_OPT_xe_migrate                   += -I$(LX_SRC_DIR)/drivers/gpu/drm/xe

#
# The generated dummies file pulls in functions without header
# includes so we force the inclusion of i915_drv explicitly for
# this compilation unit.
# 
CC_OPT_generated_dummies += -include $(LX_SRC_DIR)/drivers/gpu/drm/i915/i915_drv.h

CC_C_OPT += -Wno-unused-label

#
# Export dynamic symbols in binary
#
LD_OPT += --export-dynamic

#
# Genode C-API backends
#
