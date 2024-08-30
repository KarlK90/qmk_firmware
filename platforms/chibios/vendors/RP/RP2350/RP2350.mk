PLATFORM_RP_PATH := $(PLATFORM_PATH)/$(PLATFORM_KEY)/vendors/$(MCU_FAMILY)
PLATFORM_RP2350_PATH := $(PLATFORM_RP_PATH)/$(MCU_SERIES)

include $(PLATFORM_RP_PATH)/rp_common.mk

#
# RP2350 specific source and header files needed by QMK and ChibiOS
##############################################################################
RP2350_INC = $(PICOSDKROOT)/src/rp2350/hardware_regs/include \
             $(PICOSDKROOT)/src/rp2350/hardware_structs/include \
             $(PICOSDKROOT)/src/rp2350/pico_platform/include \
             $(PLATFORM_RP2350_PATH)

PLATFORM_SRC += $(RP2350_SRC)
EXTRAINCDIRS += $(RP2350_INC)
