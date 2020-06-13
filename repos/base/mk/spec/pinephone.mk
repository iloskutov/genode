#
# Enable peripherals of the platform
#
SPECS += pinephone usb gpio

#
# Pull in CPU specifics
#
SPECS += arm_v8

#
# Add device parameters to include search path
#
REP_INC_DIR += include/spec/pinephone

include $(BASE_DIR)/mk/spec/arm_v8a.mk
