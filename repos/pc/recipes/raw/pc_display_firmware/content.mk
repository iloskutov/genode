PORT_DIR := $(call port_dir,$(GENODE_DIR)/repos/dde_linux/ports/linux-firmware)

content: ucode_files pc_display_firmware.tar


.PHONY: ucode_files
ucode_files:
	cp -R $(PORT_DIR)/firmware/i915 . && \
	cp -R $(PORT_DIR)/firmware/xe . && \
	cp $(PORT_DIR)/firmware/LICENSE.xe . && \
	cp $(PORT_DIR)/firmware/LICENSE.i915 .

include $(GENODE_DIR)/repos/base/recipes/content.inc

pc_display_firmware.tar: ucode_files
	$(TAR) --remove-files -cf $@ -C . *.* i915/*.* xe/*.* && \
		rmdir i915 xe

