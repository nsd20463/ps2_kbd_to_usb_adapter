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
 *  along with ps2_kbd_to_usb_adapter,.  If not, see http://www.gnu.org/licenses/
 * 
 */

#ifndef KEYCODES_H
#define KEYCODES_H

#include <stdint.h>

#ifdef __cplusplus 
extern "C" {
#endif

// map a PS/2 key code to USB. returns 0 if there is no mapping
uint16_t ps2_to_usb_keycode(uint8_t);

#ifdef __cplusplus 
} // end of extern "C"
#endif

#endif

