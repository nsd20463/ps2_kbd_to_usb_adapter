/*
 * code to use a ATmega32u4 as a PS/2 keyboard to USB keyboard converter, to allow using a good ole IBM Model M PS/2 keyboard with modern computers which now (2014) lack any PS/2 ports at all.
 *
 * PS/2 protocol:
 * see: 
 *   http://www.hth.com/filelibrary/TXTFILES/keyboard.txt
 *   http://ps-2.kev009.com:8081/ohlandl/keyboard/Keyboard.html
 *
 *   6 Pin PS2 connector
 *   ---------
 *   1   DATA
 *   2   No connection
 *   3   GND
 *   4   +5V
 *   5   CLK
 *   6   No connection
 *
 *   The PS2 is numbered as follows looking into the end of the
 *   keyboard's PS/2 cable.
 *
 *           ^
 *         5   6
 *        3     4
 *        > 1 2 <
 *
 *   The <>'s indicate the position of the triangular cutouts.
 *
 *   5V signaling @ ~32.25KHz clock rate. (period ~32usec, clk low time ~20usec, high ~12usec)
 *
 *   What I see on the scope when watching my Northgate Ultra Omnikey keyboard
 *   sending a keystroke to the computer:
 *
 *            A B C D  E F
 *         ___       _______
 *   data     \_____/
 *         _____   ____   ___
 *   clk        \_/    \_/
 *
 *            A-B         19.6 usec data setup time
 *              B-C       41.6 usec clk low time
 *                C-D     27.6 usec data hold time
 *                  D-E   22.0 usec data setup time
 *                    E-F 41.6 usec clk low time
 *                C---E   48.8 usec clk high time
 *              B-----E   90.4 usec clk period (so clk speed is ~11 kHz)
 *
 *  and after the stop bit the computer holds clk low for ~504 usec, so the
 *  whole transaction takes ~1.4 msec
 *
 *  When computer send a byte to the keyboard I see
 *
 *          AB C D    E F G H
 *         ____        ___
 *   data      \______/   \___
 *                ____   _   
 *   clk   _/\___/    \_/ \_/
 *
 *          AB                19 usec clock high time before computer pulls clock low to request attention
 *           B-C              93 usec clock setup time before computer pulls data low too
 *             C-D            86 usec data low hold time  
 *               D----E       346 usec wait until keyboard starts driving clock
 *                    E       note data and clock move quasi-simultaneously, with data being delayed by 1 or 2 usec
 *                    E-F     52 usec data setup time
 *                      F-G   52 usec data hold time
 *                        G-H 52 usec data setup time
 *                    E---G   104 usec pkt period
 *
 *  And on this [long,hacked up] cable and keyboard and computer rising edges have a slow rise time of ~9 usec
 *  Note that these numbers are different than what other people report seeing from
 *  their keyboards. Since the keyboard decides the clock rate it can very easily vary.
 *
 *   1 clock wire, open collector, pull-ups
 *   1 data wire, open collector, pull-ups
 *   when idle both are not driven, and thus high
 *   when clocking data the keyboard always drives clk (for either direction)
 *   computer uses clk only for signaling
 *   start bit is 0 (low)
 *   even parity is used (so if the 8 data bits add up to 0mod2 the parity bit is set)
 *   stop bit is 1 (floating high)
 *
 * To send keyboard -> computer (keystrokes; responses to commands)
 *   keyboard:
 *     wait for both clk and data to float high
 *     pull data low
 *     pull clk low
 *     release clk (computer reads data on rising edge of clk)
 *     <clock out 9 data bits (8 data, 1 parity), pulling and releasing Clk during each data bit>
 *     after the parity bit, the computer should send the Stop bit while the keyboard still gives clk a low pulse
 *     finally the keybaord drives data low, then pulses clk low one last time
 *     then releases both clk and data
 *     the computer should have, by now, driven clk low as a final handshake
 *     the computer keeps clk low until it is ready to receive the next byte from the keyboard
 *
 *
 *
 * To send computer -> keyboard (commands to reset, set/clear LEDs)
 *   computer:
 *     pull Clk low (keep keyboard from sending)
 *     pull Data low (start bit to keyboard)
 *     release Clk (so keyboard can drive it when it is ready for the next bit)
 *     wait for Clk to transition low->high, and clock out the next bit whenever it does
 *   keyboard:
 *     notices data is low
 *     waits for clk high (so the computer has releases clk)
 *     drives clk low
 *     <pause>
 *     sample incoming data bit
 *     release clk
 *     if clk does not float high, abort (computer indicates abort by holding clk low)
 *     <pause>
 *     <repeat for 10 data bits (8 data, 1 parity, 1 stop bit)
 *     keyboard sends one bit back by pulling data low
 *
 * At power-on, the keyboard goes through a self-test (which lasts a good part of a second)
 * then it sends 0xAA indicating it is ready.
 *
 *

#include "PS2Keyboard.h"

PS2Keyboard ps2k;

int main(void) {
    ps2k.begin(2,3,PS2Keymap_US);

    while (1) {
        if (ps2k.available()) {
            int c = ps2k.read();

        }
    }
}

// pull in pieces of arduino standard library that we use
// (nice and quick and a dirty hack :-)
void pinMode(uint8_t pin, uint8_t mode) {
    uint8_t bit = digitalPinToBitMask(pin);
    uint8_t port = digitalPinToPort(pin);
    volatile uint8_t *reg, *out;

	reg = portModeRegister(port);
	out = portOutputRegister(port);

	if (mode == INPUT) { 
		uint8_t oldSREG = SREG;
                cli();
		*reg &= ~bit;
		*out &= ~bit;
		SREG = oldSREG;
	} else if (mode == INPUT_PULLUP) {
		uint8_t oldSREG = SREG;
                cli();
		*reg &= ~bit;
		*out |= bit;
		SREG = oldSREG;
	} else {
		uint8_t oldSREG = SREG;
                cli();
		*reg |= bit;
		SREG = oldSREG;
	}
}

void digitalWrite(uint8_t pin, uint8_t val)
{
	uint8_t timer = digitalPinToTimer(pin);
	uint8_t bit = digitalPinToBitMask(pin);
	uint8_t port = digitalPinToPort(pin);
	volatile uint8_t *out;

	if (port == NOT_A_PIN) return;

	// If the pin that support PWM output, we need to turn it off
	// before doing a digital write.
	if (timer != NOT_ON_TIMER) turnOffPWM(timer);

	out = portOutputRegister(port);

	uint8_t oldSREG = SREG;
	cli();

	if (val == LOW) {
		*out &= ~bit;
	} else {
		*out |= bit;
	}

	SREG = oldSREG;
}

int digitalRead(uint8_t pin)
{
	uint8_t timer = digitalPinToTimer(pin);
	uint8_t bit = digitalPinToBitMask(pin);
	uint8_t port = digitalPinToPort(pin);

	if (port == NOT_A_PIN) return LOW;

	// If the pin that support PWM output, we need to turn it off
	// before getting a digital reading.
	if (timer != NOT_ON_TIMER) turnOffPWM(timer);

	if (*portInputRegister(port) & bit) return HIGH;
	return LOW;
}


volatile unsigned long timer0_millis = 0;
static unsigned char timer0_fract = 0;

unsigned long millis()
{
	unsigned long m;
	uint8_t oldSREG = SREG;

	// disable interrupts while we read timer0_millis or we might get an
	// inconsistent value (e.g. in the middle of a write to timer0_millis)
	cli();
	m = timer0_millis;
	SREG = oldSREG;

	return m;
}

// the prescaler is set so that timer0 ticks every 64 clock cycles, and the
// the overflow handler is called every 256 ticks.
#define MICROSECONDS_PER_TIMER0_OVERFLOW (clockCyclesToMicroseconds(64 * 256))

// the whole number of milliseconds per timer0 overflow
#define MILLIS_INC (MICROSECONDS_PER_TIMER0_OVERFLOW / 1000)

// the fractional number of milliseconds per timer0 overflow. we shift right
// by three to fit these numbers into a byte. (for the clock speeds we care
// about - 8 and 16 MHz - this doesn't lose precision.)
#define FRACT_INC ((MICROSECONDS_PER_TIMER0_OVERFLOW % 1000) >> 3)
#define FRACT_MAX (1000 >> 3)

SIGNAL(TIMER0_OVF_vect) {
	// copy these to local variables so they can be stored in registers
	// (volatile variables must be read from memory on every access)
	unsigned long m = timer0_millis;
	unsigned char f = timer0_fract;

	m += MILLIS_INC;
	f += FRACT_INC;
	if (f >= FRACT_MAX) {
		f -= FRACT_MAX;
		m += 1;
	}

	timer0_fract = f;
	timer0_millis = m;
}

