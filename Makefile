# makefile for Adafruit ATmega32u4 breakout board
# references:
#    http://ladyada.net/products/atmega32u4breakout/
#  How to get started with the Atmega32u4 Breakout Board+ on Linux
#    https://forums.adafruit.com/viewtopic.php?f=24&t=23266

SRC = ps2_kbd_to_usb_adapter.cpp PS2Keyboard.cpp descriptors.c
TARGET = ps2_kbd_to_usb_adapter

MCU = atmega32u4
F_CPU = 16000000

# more definitions of the CPU and board, required by LUFA build system
ARCH = AVR8
BOARD = ADAFRUITU4
# F_CPU was set above
F_USB = $(F_CPU)
OPTIMIZATION = s
# TARGET was set above
# add the LUFA source files to the build
SRC += $(LUFA_SRC_USB) $(LUFA_SRC_USBCLASS)
# path to LUFA library. Override the default by setting it in your environment or make commandline
LUFA_PATH ?= ../libraries/lufa_LUFA_140302/LUFA
# defines for lufa_avrdude.mk so it can talk to the Adafruit ATmega32u4's bootstrap
AVRDUDE_PROGRAMMER ?= avr109
AVRDUDE_PORT ?= /dev/ttyACM0


# <not needed/done by the LUFA build system for us> DEFINES = F_CPU=$(F_CPU)
DEFINES += USE_LUFA_CONFIG_HEADER # specify the LUFA library settings in LUFAConfig.h rather than as a bunch of defines in this makefile

CC_FLAGS = -Wall $(DEFINES:%=-D%)
LD_FLAGS = 


# typical AVR CC_FLAGS flags for tighter and easier code
CC_FLAGS += -fpack-struct -fshort-enums -funsigned-char -funsigned-bitfields

all:


# program
flash : $(TARGET).hex
	avrdude -p m32u4 -P /dev/ttyACM0 -c avr109 -U flash:w:$<:i

# pull in the LUFA bits we use
include $(LUFA_PATH)/Build/lufa_core.mk
include $(LUFA_PATH)/Build/lufa_sources.mk
include $(LUFA_PATH)/Build/lufa_hid.mk
include $(LUFA_PATH)/Build/lufa_cppcheck.mk
include $(LUFA_PATH)/Build/lufa_build.mk
include $(LUFA_PATH)/Build/lufa_avrdude.mk


.PHONY : all flash 
