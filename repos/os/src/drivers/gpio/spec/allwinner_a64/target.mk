TARGET   = allwinner_a64_gpio_drv
REQUIRES = arm_v8
SRC_CC   = main.cc
LIBS     = base
INC_DIR += $(PRG_DIR)

vpath main.cc $(PRG_DIR)

