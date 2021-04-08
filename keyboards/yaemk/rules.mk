# Choose to use RISC-V or ARM MCU
RISC-V = no

ifeq ($(strip $(RISC-V)), yes)
    MCU        =   GD32VF103
    BOARD      =   SIPEED_LONGAN_NANO
    LTO_ENABLE =   yes
    EXTRAFLAGS += -Os
    EXTRAFLAGS += -fno-tree-loop-distribute-patterns -fno-builtin
    # Default clock is 96MHz, 120MHz is out of spec but possible.
    # OPT_DEFS += -DOVERCLOCK_120MHZ 
else
    MCU        =   STM32F303
    BOARD      =   QMK_PROTON_C
    LTO_ENABLE =   yes
    EXTRAFLAGS += -Os
endif

# YAEMK source files
LAYOUTS = split_5x8
SRC     = led_config.c

# Debugging
EXTRAFLAGS     += -g2
ALLOW_WARNINGS =   yes

# QMK features 
CONSOLE_ENABLE     = no
DEBOUNCE_TYPE      = eager_pk
EEPROM_DRIVER      = i2c
ENCODER_ENABLE     = yes
EXTRAKEY_ENABLE    = yes
KEYBOARD_SHARED_EP = no # Enable if you want to use VIA with GD32VF103
MOUSEKEY_ENABLE    = yes
NKRO_ENABLE        = yes
OLED_DRIVER_ENABLE = yes
RGB_MATRIX_DRIVER  = WS2812
RGB_MATRIX_ENABLE  = yes
SERIAL_DRIVER      = usart_duplex
SPLIT_KEYBOARD     = yes
VIA_ENABLE         = no
WPM_ENABLE         = yes
WS2812_DRIVER      = pwm

ifeq ($(strip $(OLED_DRIVER_ENABLE)), yes)
    ifdef OCEAN_DREAM_ENABLE
        ifeq ($(strip $(OCEAN_DREAM_ENABLE)), yes)
            SRC += ocean_dream.c
            OPT_DEFS += -DOCEAN_DREAM_ENABLE
        endif
    endif
    ifndef OCEAN_DREAM_ENABLE
        SRC += ocean_dream.c
        OPT_DEFS += -DOCEAN_DREAM_ENABLE
    endif
endif
