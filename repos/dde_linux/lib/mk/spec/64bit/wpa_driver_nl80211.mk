LIB_DIR     := $(REP_DIR)/src/lib/wpa_driver_nl80211
LIB_INC_DIR := $(LIB_DIR)/include
INC_DIR     += $(LIB_INC_DIR)

LIBS += libc libnl libnl_include

SHARED_LIB = yes
LD_OPT += --version-script=$(LIB_DIR)/symbol.map

SRC_CC += dummies.cc ioctl.cc
SRC_CC += rfkill_genode.cc

WS_CONTRIB_DIR := $(call select_from_ports,wpa_supplicant)/src/app/wpa_supplicant

# l2_packet
SRC_C   += src/l2_packet/l2_packet_linux.c
INC_DIR += $(WS_CONTRIB_DIR)/src/l2_packet

# nl80211 driver
SRC_C_drivers = drivers.c        \
                driver_nl80211.c \
                driver_nl80211_capa.c \
                driver_nl80211_event.c \
                driver_nl80211_monitor.c \
                driver_nl80211_scan.c \
                netlink.c

SRC_C += $(addprefix src/drivers/, $(SRC_C_drivers))
INC_DIR += $(WS_CONTRIB_DIR)/src/drivers \
           $(WS_CONTRIB_DIR)/src/utils \
           $(WS_CONTRIB_DIR)/src

CC_OPT += -DCONFIG_DRIVER_NL80211
CC_OPT += -DCONFIG_LIBNL20
CC_OPT += -D_LINUX_SOCKET_H

#
# "import" locally implemented socket functions
#
CC_C_OPT += -Dbind=libnl_bind
CC_C_OPT += -Dclose=libnl_close
CC_C_OPT += -Dfcntl=libnl_fcntl
CC_C_OPT += -Dgetsockname=libnl_getsockname
CC_C_OPT += -Dpoll=libnl_poll
CC_C_OPT += -Drecvfrom=libnl_recvfrom
CC_C_OPT += -Drecvmsg=libnl_recvmsg
CC_C_OPT += -Dsend=libnl_send
CC_C_OPT += -Dsendmsg=libnl_sendmsg
CC_C_OPT += -Dsendto=libnl_sendto
CC_C_OPT += -Dsetsockopt=libnl_setsockopt
CC_C_OPT += -Dsocket=libnl_socket

vpath %.c  $(WS_CONTRIB_DIR)
vpath %.cc $(LIB_DIR)

CC_CXX_WARN_STRICT =
