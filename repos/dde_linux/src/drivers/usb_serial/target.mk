TARGET  := usb_serial_drv
SRC_C   := dummies.c
SRC_CC  := main.cc lx_emul.cc
SRC_CC  += printf.cc timer.cc scheduler.cc malloc.cc env.cc work.cc

LIBS    := base usb_serial_include lx_kit_setjmp

USB_CONTRIB_DIR := $(call select_from_ports,dde_linux)/src/drivers/usb_serial

INC_DIR += $(USB_CONTRIB_DIR)/drivers/usb/core
INC_DIR += $(PRG_DIR)
INC_DIR += $(REP_DIR)/src/include

SRC_C += drivers/usb/core/config.c
SRC_C += drivers/usb/core/generic.c
SRC_C += drivers/usb/serial/bus.c
SRC_C += drivers/usb/serial/generic.c
SRC_C += drivers/usb/serial/usb-serial.c
SRC_C += drivers/usb/serial/ftdi_sio.c
SRC_C += drivers/usb/serial/pl2303.c
SRC_C += drivers/usb/serial/option.c
SRC_C += drivers/usb/serial/usb_wwan.c
SRC_C += lib/kfifo.c

CC_OPT   += -O0 -fno-inline -fno-default-inline -ggdb -g3 -D__KERNEL__
CC_C_OPT += -O0 -fno-inline -fno-default-inline -ggdb -g3 -Wno-unused-but-set-variable -Wno-pointer-sign \
            -Wno-incompatible-pointer-types -Wno-unused-variable \
            -Wno-unused-function -Wno-uninitialized -Wno-maybe-uninitialized

CC_CXX_OPT = -fpermissive

CC_CXX_WARN_STRICT =

vpath %.c  $(USB_CONTRIB_DIR)
vpath %.cc $(REP_DIR)/src/lx_kit
