INC_DIR += $(BASE_DIR)/../base-hw/src/bootstrap/spec/pinephone

SRC_CC  += bootstrap/spec/arm/gicv2.cc
SRC_CC  += bootstrap/spec/arm_64/cortex_a53_mmu.cc
SRC_CC  += bootstrap/spec/pinephone/platform.cc
SRC_CC  += lib/base/arm_64/kernel/interface.cc
SRC_CC  += spec/64bit/memory_map.cc
SRC_S   += bootstrap/spec/arm_64/crt0.s

vpath spec/64bit/memory_map.cc $(BASE_DIR)/../base-hw/src/lib/hw

NR_OF_CPUS = 1

include $(BASE_DIR)/../base-hw/lib/mk/bootstrap-hw.inc
