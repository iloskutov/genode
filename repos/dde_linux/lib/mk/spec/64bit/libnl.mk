SHARED_LIB := yes

LD_OPT += --version-script=$(LIB_DIR)/symbol.map

LIB_DIR     = $(REP_DIR)/src/lib/libnl
LIB_INC_DIR = $(LIB_DIR)/include

INC_DIR += $(LIB_INC_DIR)/spec/64bit

LIBS += libc libnl_include

LIBNL_CONTRIB_DIR := $(call select_from_ports,libnl)/src/lib/libnl

INC_DIR += $(LIB_INC_DIR)
INC_DIR += $(LIBNL_CONTRIB_DIR)/include

SRC_CC += lxcc_emul.cc socket.cc if.cc

#
# "export" locally implemented socket functions
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

# libnl
SRC_C += $(addprefix lib/, attr.c cache.c cache_mngt.c data.c error.c handlers.c \
                           hashtable.c msg.c nl.c object.c socket.c utils.c)

# libnl-genl
SRC_C += $(addprefix lib/genl/, ctrl.c family.c genl.c mngt.c)

CC_OPT   += -DSYSCONFDIR=\"/\"
CC_C_OPT += -include $(LIB_INC_DIR)/libnl_emul.h

CC_OPT += -D_LINUX_SOCKET_H

vpath %.c  $(LIBNL_CONTRIB_DIR)
vpath %.c  $(LIB_DIR)
vpath %.cc $(LIB_DIR)

CC_CXX_WARN_STRICT :=
