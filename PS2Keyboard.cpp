/*
  PS2Keyboard.cpp - PS2Keyboard library
  Copyright (c) 2007 Free Software Foundation.  All right reserved.
  Written by Christian Weichel <info@32leaves.net>

  ** Mostly rewritten Paul Stoffregen <paul@pjrc.com> 2010, 2011
  ** Modified for use beginning with Arduino 13 by L. Abraham Smith, <n3bah@microcompdesign.com> * 
  ** Modified for easy interrup pin assignement on method begin(datapin,irq_pin). Cuningan <cuninganreset@gmail.com> **

  for more information you can read the original wiki in arduino.cc
  at http://www.arduino.cc/playground/Main/PS2Keyboard
  or http://www.pjrc.com/teensy/td_libs_PS2Keyboard.html

  Version 2.4 (March 2013)
  - Support Teensy 3.0, Arduino Due, Arduino Leonardo & other boards
  - French keyboard layout, David Chochoi, tchoyyfr at yahoo dot fr

  Version 2.3 (October 2011)
  - Minor bugs fixed

  Version 2.2 (August 2011)
  - Support non-US keyboards - thanks to Rainer Bruch for a German keyboard :)

  Version 2.1 (May 2011)
  - timeout to recover from misaligned input
  - compatibility with Arduino "new-extension" branch
  - TODO: send function, proposed by Scott Penrose, scooterda at me dot com

  Version 2.0 (June 2010)
  - Buffering added, many scan codes can be captured without data loss
    if your sketch is busy doing other work
  - Shift keys supported, completely rewritten scan code to ascii
  - Slow linear search replaced with fast indexed table lookups
  - Support for Teensy, Arduino Mega, and Sanguino added

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "PS2Keyboard.h"

#define BUFFER_SIZE 45
static volatile uint8_t buffer[BUFFER_SIZE];
static volatile uint8_t head, tail;
static uint8_t CharBuffer=0;
static uint8_t UTF8next=0;
static const PS2Keymap_t *keymap=NULL;

// The ISR for the external interrupt,
// which happens on the falling edge of the PS2 Clock signal
static volatile uint8_t bitcount=0;
static volatile unsigned long prev_ms=0;
ISR(INT0_vect)
{
	static uint8_t incoming=0;
	unsigned long now_ms;
	uint8_t n, bn, val;

    // read the data pin
    val = (PIND >> PS2_DATA_PIN) & 1;

	n = bitcount;

	now_ms = millis();
	if (now_ms - prev_ms > 100) {
        // it's been too long; start over
		n = 0;
		incoming = 0;
	}
	prev_ms = now_ms;

    // NOTE: the oscilloscope shows me that something is pulling Clk low for 240 usec every 250 msec, then releasing Clk, 
    // without doing anything else (Data stays high). It turns out it is the computer. Maybe it is the protocol for some
    // other device (PS/2 mouse?) Anyway I need to ignore these pulses.
    // Aha, since data is high during these pulses I know this isn't a proper start bit and I can ignore it.
    if (n == 0 && val) {
        // not a proper start bit; ignore it
        return;
    }

    bn = n-1;
	if (bn < 8) { // note bn is unsigned, so this skips both the start bit and the parity and stop bits
		incoming |= val << bn;
	} else if (bn == 8) {
        // check the parity bit for correctness?
    }
    n++;
	if (n == 11) {
		uint8_t i = head + 1;
		if (i >= BUFFER_SIZE) i = 0;
		if (i != tail) {
			buffer[i] = incoming;
			head = i;
		}
		n = 0;
		incoming = 0;
	}
    bitcount = n;
}

static uint8_t get_scan_code(void)
{
	uint8_t c, i;

	i = tail;
	if (i == head) return 0;
	i++;
	if (i >= BUFFER_SIZE) i = 0;
	c = buffer[i];
	tail = i;
	return c;
}

// http://www.quadibloc.com/comp/scan.htm
// http://www.computer-engineering.org/ps2keyboard/scancodes2.html

// These arrays provide a simple key map, to turn scan codes into ISO8859
// output.  If a non-US keyboard is used, these may need to be modified
// for the desired output.
//

const PROGMEM PS2Keymap_t PS2Keymap_US = {
  // without shift
	{0, PS2_F9, 0, PS2_F5, PS2_F3, PS2_F1, PS2_F2, PS2_F12,
	0, PS2_F10, PS2_F8, PS2_F6, PS2_F4, PS2_TAB, '`', 0,
	0, 0 /*Lalt*/, 0 /*Lshift*/, 0, 0 /*Lctrl*/, 'q', '1', 0,
	0, 0, 'z', 's', 'a', 'w', '2', 0,
	0, 'c', 'x', 'd', 'e', '4', '3', 0,
	0, ' ', 'v', 'f', 't', 'r', '5', 0,
	0, 'n', 'b', 'h', 'g', 'y', '6', 0,
	0, 0, 'm', 'j', 'u', '7', '8', 0,
	0, ',', 'k', 'i', 'o', '0', '9', 0,
	0, '.', '/', 'l', ';', 'p', '-', 0,
	0, 0, '\'', 0, '[', '=', 0, 0,
	0 /*CapsLock*/, 0 /*Rshift*/, PS2_ENTER /*Enter*/, ']', 0, '\\', 0, 0,
	0, 0, 0, 0, 0, 0, PS2_BACKSPACE, 0,
	0, '1', 0, '4', '7', 0, 0, 0,
	'0', '.', '2', '5', '6', '8', PS2_ESC, 0 /*NumLock*/,
	PS2_F11, '+', '3', '-', '*', '9', PS2_SCROLL, 0,
	0, 0, 0, PS2_F7 },
  // with shift
	{0, PS2_F9, 0, PS2_F5, PS2_F3, PS2_F1, PS2_F2, PS2_F12,
	0, PS2_F10, PS2_F8, PS2_F6, PS2_F4, PS2_TAB, '~', 0,
	0, 0 /*Lalt*/, 0 /*Lshift*/, 0, 0 /*Lctrl*/, 'Q', '!', 0,
	0, 0, 'Z', 'S', 'A', 'W', '@', 0,
	0, 'C', 'X', 'D', 'E', '$', '#', 0,
	0, ' ', 'V', 'F', 'T', 'R', '%', 0,
	0, 'N', 'B', 'H', 'G', 'Y', '^', 0,
	0, 0, 'M', 'J', 'U', '&', '*', 0,
	0, '<', 'K', 'I', 'O', ')', '(', 0,
	0, '>', '?', 'L', ':', 'P', '_', 0,
	0, 0, '"', 0, '{', '+', 0, 0,
	0 /*CapsLock*/, 0 /*Rshift*/, PS2_ENTER /*Enter*/, '}', 0, '|', 0, 0,
	0, 0, 0, 0, 0, 0, PS2_BACKSPACE, 0,
	0, '1', 0, '4', '7', 0, 0, 0,
	'0', '.', '2', '5', '6', '8', PS2_ESC, 0 /*NumLock*/,
	PS2_F11, '+', '3', '-', '*', '9', PS2_SCROLL, 0,
	0, 0, 0, PS2_F7 },
	0
};





#define BREAK     0x01
#define MODIFIER  0x02
#define SHIFT_L   0x04
#define SHIFT_R   0x08
#define ALTGR     0x10

static char get_iso8859_code(void)
{
	static uint8_t state=0;
	uint8_t s;
	char c;

	while (1) {
		s = get_scan_code();
		if (!s) return 0;
		if (s == 0xF0) {
			state |= BREAK;
		} else if (s == 0xE0) {
			state |= MODIFIER;
		} else {
			if (state & BREAK) {
				if (s == 0x12) {
					state &= ~SHIFT_L;
				} else if (s == 0x59) {
					state &= ~SHIFT_R;
				} else if (s == 0x11 && (state & MODIFIER)) {
					state &= ~ALTGR;
				}
				// CTRL, ALT & WIN keys could be added
				// but is that really worth the overhead?
				state &= ~(BREAK | MODIFIER);
				continue;
			}
			if (s == 0x12) {
				state |= SHIFT_L;
				continue;
			} else if (s == 0x59) {
				state |= SHIFT_R;
				continue;
			} else if (s == 0x11 && (state & MODIFIER)) {
				state |= ALTGR;
			}
			c = 0;
			if (state & MODIFIER) {
				switch (s) {
				  case 0x70: c = PS2_INSERT;      break;
				  case 0x6C: c = PS2_HOME;        break;
				  case 0x7D: c = PS2_PAGEUP;      break;
				  case 0x71: c = PS2_DELETE;      break;
				  case 0x69: c = PS2_END;         break;
				  case 0x7A: c = PS2_PAGEDOWN;    break;
				  case 0x75: c = PS2_UPARROW;     break;
				  case 0x6B: c = PS2_LEFTARROW;   break;
				  case 0x72: c = PS2_DOWNARROW;   break;
				  case 0x74: c = PS2_RIGHTARROW;  break;
				  case 0x4A: c = '/';             break;
				  case 0x5A: c = PS2_ENTER;       break;
				  default: break;
				}
			} else if ((state & ALTGR) && keymap->uses_altgr) {
				if (s < PS2_KEYMAP_SIZE)
					c = pgm_read_byte(keymap->altgr + s);
			} else if (state & (SHIFT_L | SHIFT_R)) {
				if (s < PS2_KEYMAP_SIZE)
					c = pgm_read_byte(keymap->shift + s);
			} else {
				if (s < PS2_KEYMAP_SIZE)
					c = pgm_read_byte(keymap->noshift + s);
			}
			state &= ~(BREAK | MODIFIER);
			if (c) return c;
		}
	}
}

bool PS2Keyboard::raw_available() {
    return head != tail;
}

uint8_t PS2Keyboard::raw_read() {
    return get_scan_code();
}

// return true if PS2 bus is idle
static inline bool idle() {
    return (PIND & (_BV(PS2_CLK_PIN) | _BV(PS2_DATA_PIN))) == (_BV(PS2_CLK_PIN) | _BV(PS2_DATA_PIN));
}

// send a byte to the keyboard
bool PS2Keyboard::raw_write(uint8_t v) {
    // to send a byte to the keyboard over the PS/2 (aka AT) protocol, the host (us)
    // has to twiddle the lines to get the keyboard's attention, and then let the keyboard
    // clock the bits at its own rate
    // first wait for Clk to be high and any in-progress byte from the keyboard to finish arriving.
    // note that since the keyboard sends Ack bytes (0xFA) after most command bytes, there is likely an FA being received
    // at the same time that we are trying to send the 2nd byte of a multi-byte command.
    // We usually are in a  race with the keyboard to see who sends first when it comes time for us to send the 2nd byte. 
    // The keyboard will skip sending the FA if we overwrite the keyboard (say the IBM spec).
    // The 100 msec is a sanity check timeout
wait_for_idle_bus:
	unsigned long start_ms = millis();
	unsigned long now_ms = start_ms;
    while ((!idle() || (bitcount && now_ms - prev_ms <= 100)) && now_ms - start_ms <= 100)
        now_ms = millis(); // spin

    if ((PIND & (_BV(PS2_CLK_PIN)|_BV(PS2_DATA_PIN))) != (_BV(PS2_CLK_PIN)|_BV(PS2_DATA_PIN)))
        // Clk and Data aren't high; something is stuck and we can't send
        return false;

    // emulate the PS motherboard I scoped and wait 19 usec after Clk is high before starting
    // Note that I don't really know if Clk just transitioned low->high, but just in case
    _delay_us(19);
    if ((PIND & (_BV(PS2_CLK_PIN)|_BV(PS2_DATA_PIN))) != (_BV(PS2_CLK_PIN)|_BV(PS2_DATA_PIN)) || (bitcount && (millis() - prev_ms <= 100)))
        goto wait_for_idle_bus;
    // OK at this point we believe the PS/2 bus is idle and we're going to grab it and go

    // we pull Clk low, which inhibits the keyboard from sending
    // while we do this we don't want an interrupt since we'd only read what we are writing, so disable the Clk pin interrupt
    EIMSK = 0; // disable INT0 (and all the other INTns, but we don't use them so that's fine)
    (void)EIMSK; // wait until the change is effective (how very grown up of us :-)
    // switch Clk to be driven low while leaving Data as a pulled-up input
    // Note that we switch by temporarily letting Clk float, which is better than temporarily driving it to high
    PORTD = _BV(PS2_DATA_PIN); // keep pulling Data up, but release Clk
    DDRD = _BV(PS2_CLK_PIN); // drive Clk low
    _delay_us(93); // emulate the PC I scoped and wait 93 usec before pulling data low as well. The IBM spec says Clk should be low for at least 60 usec
    // pull Data low as well
    PORTD = 0;
    DDRD = _BV(PS2_CLK_PIN) | _BV(PS2_DATA_PIN);
    _delay_us(86); // emulate the PC I scoped and wait 86 usec before releasing Clock
    // release Clk (which should float back high), and keep holding Data low (so the bus doesn't look idle)
    // Note that we first stop driving Clk, then enable the pullup
    DDRD = _BV(PS2_DATA_PIN);
    PORTD = _BV(PS2_CLK_PIN);
    // wait for the keyboard to drive Clk low. Every time the keyboard drives Clk low, feed it the next bit
    // Note the Northgate OmniKey Ultra I am using for test takes ~350 usec before it drives Clk low for the first bit
    // The IBM spec says the keyboard should have been checking the bus no more than every 10 msec, so it might take 10 msec for the keyboard to notice
    // (the 0 we are driving now is considered the Start bit)
    uint8_t parity = 1; // while we clock out the data bits, compute the parity bit
	start_ms = millis();
	now_ms = start_ms;
    for (uint8_t n=0; n<10 && now_ms-start_ms < 100; n++) {
        if (n == 8) {
            // we're done with the data; next we send the parity bit and the stop bit
            v = parity | 0x2;
        }
        // wait for Clk to go low
        while (PIND & _BV(PS2_CLK_PIN) && (now_ms=millis()) - start_ms < 100) /* spin */;
        // Clk went low; setup the next data bit
        uint8_t bit = v&1;
        parity ^= bit;
        v >>= 1;
        if (bit) {
            // send a 1 by letting the Data line get pulled-up to high
            DDRD = 0;
            PORTD = _BV(PS2_CLK_PIN) | _BV(PS2_DATA_PIN);
        } else {
            // send a 0 by pulling the Data line low
            PORTD = _BV(PS2_CLK_PIN);
            DDRD = _BV(PS2_DATA_PIN);
        }
        // wait for Clk to go high (kbd samples Data on the Clk's low->high transition)
        while (!(PIND & _BV(PS2_CLK_PIN)) && (now_ms=millis()) - start_ms < 100) /* spin */;
    }
    // release Data (and Clk was and remains released), setting both back to pulled-up inputs
    // note that since Data is released (and thus a 1, since it is the Stop bit) no setup/hold violation occurs at the keyboard side as it clocks in the Stop bit
    DDRD = 0; // all pins are inputs
    PORTD = _BV(PS2_CLK_PIN) | _BV(PS2_DATA_PIN); // pull up both wires
    if (now_ms - start_ms >= 100) {
        // this transaction timed out
fail:
        EIMSK = _BV(PS2_CLK_INT); // re-enable INT0 on the way out
        return false;
    }
    // finally there will be a handshake from the keyboard to acknowlege the reception
    // the keyboard is going to clock a 0 bit to us. Wait for it
    while ((PIND & _BV(PS2_CLK_PIN)) && (now_ms=millis()) - start_ms < 100) /* spin */;
    if (now_ms - start_ms >= 100)
        goto fail;
    // read the handshake Data bit
    v = PIND & _BV(PS2_DATA_PIN);
    if (v) {
        // something didn't go right; the handshake should be a 0 bit
        goto fail;
    }
    // some specs say you should handshake back by stretching the Clk pulse. However the IBM spec does not say this. The IBM spec
    // says the transaction is done as soon as Data and Clk go back to high. I suspect what I see on the scope is the
    // computer inhibiting the keyboard from sending while it processes the keystroke. We don't have that problem, so
    // we're done (and the keyboard will release Clk when it is ready to)
    if (0) {
        // handshake back by holding Clk low for long enough that the keyboard, which should release Clk soon, will notice
        PORTD = _BV(PS2_DATA_PIN);
        DDRD = _BV(PS2_CLK_PIN);
        _delay_us(180);
        // let Clk float back up high and we're done
        DDRD = 0;
        PORTD = _BV(PS2_CLK_PIN) | _BV(PS2_DATA_PIN);
    }

    if (0) {
    // wait for the bus to be back to idle state before returning
    while (!idle() && now_ms - start_ms <= 100)
        now_ms = millis(); // spin

    if (!idle()) {
        // Clk and Data aren't high; something is stuck
        return false;
    }
    }

    EIFR = _BV(PS2_CLK_INT); // ack away the pending INT0 (which happened because we caused falling edges on Clk when sending, so an INT0 is, at this point, pending)
    EIMSK = _BV(PS2_CLK_INT); // re-enable INT0
    return true;
}

bool PS2Keyboard::available() {
	if (CharBuffer || UTF8next) return true;
	CharBuffer = get_iso8859_code();
	if (CharBuffer) return true;
	return false;
}

int PS2Keyboard::read() {
	uint8_t result;

	result = UTF8next;
	if (result) {
		UTF8next = 0;
	} else {
		result = CharBuffer;
		if (result) {
			CharBuffer = 0;
		} else {
			result = get_iso8859_code();
		}
		if (result >= 128) {
			UTF8next = (result & 0x3F) | 0x80;
			result = ((result >> 6) & 0x1F) | 0xC0;
		}
	}
	if (!result) return -1;
	return result;
}

void PS2Keyboard::begin(const PS2Keymap_t &map) {
  keymap = &map;

  // initialize both clk and data to be pulled-up input pins
  DDRD = 0; // not really needed since the reset value is 0
  PORTD = _BV(PS2_CLK_PIN) | _BV(PS2_DATA_PIN); // pull up both wires
 
  // configure PS2_CLK_INT on the falling edge
  EICRA = _BV(ISC01<<(2*PS2_CLK_INT)); // interrupt on a falling edge of INT0
  EIMSK = _BV(PS2_CLK_INT); // enable INT0
}

