# Choose to use RISC-V or ARM MCU
RISC-V = yes

ifeq ($(strip $(RISC-V)), yes)
    MCU                =   GD32VF103
    BOARD              =   SIPEED_LONGAN_NANO
    LTO_ENABLE         =   yes
    EXTRAFLAGS         += -Ofast  -gdwarf-4 -Wextra
    # Default clock is 96MHz, 120MHz is out of spec but possible.
    OPT_DEFS           += -DOVERCLOCK_120MHZ
else
    MCU        =   STM32F303
    BOARD      =   GENERIC_STM32_F303XC
    LTO_ENABLE =   yes
    EXTRAFLAGS += -Ofast
endif

# DZZY source files
SRC     += matrix.c
LAYOUTS =  dzzy_5x6

# Debugging
EXTRAFLAGS     += -g
ALLOW_WARNINGS =   yes

# QMK features 
CONSOLE_ENABLE                     = no
CUSTOM_MATRIX                      = lite
EEPROM_DRIVER                      = spi
ENCODER_ENABLE                     = no
EXTRAKEY_ENABLE                    = no
KEYBOARD_SHARED_EP                 = yes
MOUSEKEY_ENABLE                    = no
NKRO_ENABLE                        = yes
REGISTER_MULTIPLE_KEYEVENTS_ENABLE = yes
VIA_ENABLE                         = yes
WPM_ENABLE                         = no