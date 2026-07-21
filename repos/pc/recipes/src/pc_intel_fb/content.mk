MIRROR_FROM_REP_DIR := src/driver/framebuffer/intel/pc \
                       src/lib/pc_intel_xe \
                       src/lib/pc/lx_emul \
                       src/include \
                       lib/symbols/pc_intel_xe \
                       lib/mk/spec/x86_64/pc_intel_xe.mk

content: $(MIRROR_FROM_REP_DIR)

PORT_DIR := $(call port_dir,$(GENODE_DIR)/repos/dde_linux/ports/linux)

$(MIRROR_FROM_REP_DIR):
	$(mirror_from_rep_dir)


content: LICENSE
LICENSE:
	cp $(PORT_DIR)/src/linux/COPYING $@
