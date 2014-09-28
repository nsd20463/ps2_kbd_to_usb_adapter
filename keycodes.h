
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

