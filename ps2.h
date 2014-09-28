
#ifndef PS2_H
#define PS2_H

#include <avr/io.h>         // this contains the AVR IO port definitions
#include <avr/interrupt.h>  // interrupt service routines
#include <avr/pgmspace.h>   // tools used to store variables in program memory
#include <avr/sleep.h>      // sleep mode utilities
#include <util/delay.h>     // some convenient delay functions
#include <stdint.h>
#include <stdlib.h>

extern unsigned long millis(void);
extern void die_blinking(uint8_t);

// Note for this app the Clock pin is hardcoded to be INT0/PORTD0
// and Data pin is hardcoded to be PORTD1 (For lack of better imagination)
#define PS2_CLK_PIN  PD0
#define PS2_CLK_INT  INT0 // must match PS2_CLK_PIN
#define PS2_DATA_PIN PD1

void ps2_init(void);
void ps2_tick(void);

uint8_t ps2_available(void); // is there ps2 data available to ps2_read()
uint8_t ps2_read(void);
uint8_t ps2_write(uint8_t v);

void ps2_write2(uint8_t a, uint8_t b); // write a 2-byte command
void ps2_set_leds(uint8_t v);
void ps2_set_scan_set(uint8_t v);

#endif
