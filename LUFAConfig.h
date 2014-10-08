/*
 *  This file is part of ps2_kbd_to_usb_adapter,
 *  copyright (c) 2014 Nicolas S. Dade
 *
 *  ps2_kbd_to_usb_adapter, is free software: you can redistribute it 
 *  and/or modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation, either version 3 of the
 *  License, or (at your option) any later version.
 *
 *  ps2_kbd_to_usb_adapter, is distributed in the hope that it will be
 *  useful, but WITHOUT ANY WARRANTY; without even the implied warranty
 *  of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with ps2_kbd_to_usb_adapter.  If not, see http://www.gnu.org/licenses/
 * 
 */

// LUFA library configuration
// I try and strip it down a little, as suggested by the LUFA docs, but really I don't have space issues

#define DISABLE_TERMINAL_CODES // dont need them. I really shouldnt have any debug strings in LUFA, but just in case this would make them smaller

#define HID_HOST_BOOT_PROTOCOL_ONLY // we're just a keyboard. we can stick with the simple protocol. this saves everyone a bunch of code

#define ORDERED_EP_CONFIG // we'll be carefull

#define USB_DEVICE_ONLY // hardcode the args to USB_Init() to just what we need
#define USE_STATIC_OPTIONS (USB_DEVICE_OPT_FULLSPEED | USB_OPT_REG_ENABLED | USB_OPT_AUTO_PLL)

#define NO_LIMITED_CONTROLLER_CONNECT // the adafruit ATmega32u4 breakout board has the VBUS line connected, so no need to emulate it

#define USE_FLASH_DESCRIPTORS // descriptor structs are fixed and in progmen

#define NO_INTERNAL_SERIAL // the ATmega doesn't have any built-in serial or ID number

#define FIXED_CONTROL_ENDPOINT_SIZE 8 // we'll use the usual value for the control endpoint

#define DEVICE_STATE_AS_GPIOR 2 // we don't use GPIOR2 in our code, so LUFA can go ahead and use it

#define FIXED_NUM_CONFIGURATIONS 1 // we only have one USB device configuration

//#define INTERRUPT_CONTROL_ENDPOINT // I used to think this would be required, because in the PS/2 code, when we send commands to the PS/2 keyboard we are stuck in that code until the keyboard has clocked the bits from us, which takes an arbitrary amount of time. However everything seems to work out fine without it, and since it isn't a common LUFA config I'll avoid it.

#define NO_DEVICE_REMOTE_WAKEUP // for now I don't implement making the PS/2 keyboard wake up the PC. It might not be a great idea to ever do it given the power usage of these old keyboards at idle (~200 mA x 5V = 1 Watt)

#define NO_DEVICE_SELF_POWER // we are always powered via the USB bus

