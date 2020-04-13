include $(REP_DIR)/src/drivers/usb_host/target.inc

TARGET    = pinephone_usb_host_drv
REQUIRES  = arm_v8

SRC_C   += usb/host/ehci-hcd.c
SRC_C   += usb/host/ohci-hcd.c

INC_DIR += $(REP_DIR)/src/drivers/usb_host/spec/arm
INC_DIR += $(REP_DIR)/src/include/spec/arm_64

SRC_CC  += spec/arm/platform.cc
SRC_CC  += spec/pinephone/platform.cc

SRC_C   += spec/pinephone/ehci-platform.c
SRC_C   += spec/pinephone/ohci-platform.c

CC_OPT  += -DCONFIG_ARM64
CC_OPT  += -DCONFIG_USB_EHCI_HCD
CC_OPT  += -DCONFIG_USB_EHCI_ROOT_HUB_TT
CC_OPT  += -DCONFIG_USB_EHCI_TT_NEWSCHED
CC_OPT  += -DCONFIG_USB_OHCI_HCD

CC_OPT  += -DDEBUG
