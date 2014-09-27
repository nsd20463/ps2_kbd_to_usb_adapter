# makefile for Adafruit ATmega32u4 breakout board
# references:
#    http://ladyada.net/products/atmega32u4breakout/
#  How to get started with the Atmega32u4 Breakout Board+ on Linux
#    https://forums.adafruit.com/viewtopic.php?f=24&t=23266

SRC = ps2_kbd_to_usb_adapter.cpp PS2Keyboard.cpp
OUT = ps2_kbd_to_usb_adapter.elf
MCU = atmega32u4

DEFINES = F_CPU=16000000

CFLAGS = -mmcu=$(MCU) -Os -Wall $(DEFINES:%=-D%) $(INCDIRS:%=-I%)
LDFLAGS = 

HEADERS := $(wildcard *.h) # crude, yet effective enough for now
OBJ = $(filter %.o, $(SRC:%.c=%.o) $(SRC:%.cpp=%.o))
HEX = $(OUT:%.elf=%.hex)

# typical AVR CFLAGS flags for tighter and easier code
CFLAGS += -fpack-struct -fshort-enums -funsigned-char -funsigned-bitfields

all : $(OUT)

# compile c
%.o : %.c $(HEADERS)
	avr-gcc $(CFLAGS) -c $< -o $@

# compile c++
%.o : %.cpp $(HEADERS)
	avr-g++ $(CFLAGS) -c $< -o $@

# link
$(OUT) : $(OBJ)
	avr-gcc $(CFLAGS) $(LDFLAGS) $^ -o $@
	avr-size --format=avr --mcu=$(MCU) $@

# extract main AVR flash area from .elf
%.hex : %.elf
	avr-objcopy -j .text -j .data -O ihex $< $@

# extract eeprom AVR image from .elf
%.eeprom : %.elf
	avr-objcopy -j .eeprom --change-section-lma .eeprom=0 -O ihex $< $@ # --no-change-warnings

# clean outputs
clean :
	-rm $(wildcard $(OBJ) $(OUT) $(HEX))

# program
flash : $(HEX)
	avrdude -p m32u4 -P /dev/ttyACM0 -c avr109 -U flash:w:$<:i

.PHONY : all flash clean
.SECONDARY : $(OUT) $(HEX)
.PRECIOUS : $(OBJ) $(OUT) $(HEX)
