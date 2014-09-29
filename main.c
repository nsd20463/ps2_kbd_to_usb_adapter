/*
 * code to use a ATmega32u4 as a PS/2 keyboard to USB keyboard converter, to allow using a good ole IBM Model M (or Northgate Omnikey Ultra) AT or PS/2 keyboard with modern computers which now (2014) lack any PS/2 ports at all.
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
 *          AB C D    E F G H    S T U VW XY   Z
 *         ____        ___        ___   _________
 *   data      \______/   \___.../   \_/
 *                ____   _          _           _
 *   clk   _/\___/    \_/ \_/....\_/ \_/\_/\___/
 *
 *          AB                19 usec clock high time before computer pulls clock low to request attention
 *           B-C              93 usec clock setup time before computer pulls data low too
 *             C-D            86 usec data low hold time before clock is released
 *               D----E       346 usec wait until keyboard starts driving clock
 *                    E       note data and clock move quasi-simultaneously, with data being delayed by 1 or 2 usec
 *                    E-F     52 usec data setup time
 *                      F-G   52 usec data hold time
 *                        G-H 52 usec data setup time
 *                    E---G   104 usec pkt period
 *   and at and after the stop bit, the handshake looks like
 *                               S-T             46 usec low Clk pulse to clock in the stop bit
 *                                 T-U           46 usec high Clk
 *                                   U-V         46 usec low Clk and Data as the keyboard sends the Ack
 *                                     VW        16 usec blip on Clk
 *                                      W-X      136 usec Clk held low
 *                                        XY     16 usec blip on Clk, again
 *                                         Y---Z 188 usec Clk held low
 *          A----------------------------------Z 1.4 msec
 *  I have no idea what the blips on Clk are. Maybe the handoff of the Clk streching ack isn't perfect.
 *  I hope I don't need to debounce :-)
 *
 *  And on this [long,hacked up] cable and keyboard and computer rising edges have a slow rise time of ~9 usec
 *
 *  Lastly the Northgate keyboard draws ~195 mA of current when idle, and more when the LEDs are lit. How much
 *  more I cannot tell right now because my meter only goes to 200 mA. But a 1980's era green LED usually took
 *  10 to 20 mA, so 3 of these would give a total current of ~260 mA.
 *
 *  Note that these timing numbers are different than what other people report seeing from
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
 */

#include <LUFA/Drivers/USB/USB.h>
#include "ps2.h"
#include "keycodes.h"

// blink the byte c on the LED slow and noticeably enough that a human can write it down
static void blink_byte(uint8_t c) {
    // display c on the LED
    // a 1 bit will be shown as a steady lit LED
    // a 0 bit as a dim LED
    for (uint8_t i=0; i<8; i++) {
      uint8_t bit = (c>>7);
      c <<= 1;
      PORTE |= 1<<6;
      for (uint8_t j=0; j<80; j++) {
          _delay_us(100);
          if (!bit)
              PORTE ^= 1<<6;
          _delay_us(10000);
          if (!bit)
              PORTE ^= 1<<6;
      }
      PORTE &= ~(1<<6);
      // pause between bits
      for (uint8_t j=0; j<250; j++)
        _delay_us(500);
    }

    // pause between bytes
    for (uint8_t j=0; j<200; j++)
      _delay_us(6000);
}

// die, blinking out the debug byte every 4 seconds, and blinking rapidly the rest of the time
void die_blinking(uint8_t c) {
    DDRE |= (1<<6);
    while (1) {
        for (uint16_t i=0; i<4*40; i++) {
          _delay_us(25000);
          PORTE ^= 1<<6;
        }
        blink_byte(c);
    }
}

//-------------------------------------------------------------------------
// hack so I can send debug messages as ascii text over USB
// (no more debugging with only a single red LED to blink out bytes on :-)
#if 1
#include <stdio.h>
#include <stdarg.h>

char debug_buf[500];
char* debug_tail; // once it's a 16 bit index it might as well be a pointer
char* debug_head; // technically these shoud be volatile, but given the structure of the current code it isn't necessary

void debug(const char* fmt, ...) {
    // if we've caught up; reset to the start of the buffer
    // (this is easier than a real circular buffer, and for debug
    // purposes as useful)
    if (debug_head == debug_tail)
        debug_head = debug_tail = debug_buf;
    va_list va;
    va_start(va,fmt);
    char* buf = debug_head;
    size_t left = debug_buf + sizeof(debug_buf) - debug_head;
    vsnprintf(buf, left, fmt, va);
    debug_head += strlen(debug_head);
    va_end(va);
}

#endif


//-------------------------------------------------------------------------
// the keyboard array, as reported over USB
// USB HID sees a keyboard as a large bit array, each bit representing a single key, where 1=key is pressed, and 0=key is released
// USB HIB reports (the packets sent back to the host) contain an slice of the bitmap (range 0xE0-E7) where the modifier (shift/ctrl/alt) keys
// are found, and an array of up to 6 bit numbers to represent up to 6 down keys.

uint8_t matrix[0xE8/8]; // 29 bytes, the last of which is the modifier keys

// build a USB keyboard report in the given 8-byte buffer
static void make_usb_report(uint8_t* report) {
    report[0] = matrix[0xE0/8];
    report[1] = 0; // always 
    uint8_t j = 2; // our index into the report[] 
    // if we have debug stuff buffered up, send the next char
    if (debug_tail != debug_head) {
        uint8_t c = *debug_tail++;
        uint8_t m = 0;
        // convert ascii c into usb keycode
        if ('a' <= c && c <= 'z')
            c = c - 'a' + 4;
        else if ('A' <= c && c <= 'Z') {
            c = c - 'A' + 4;
            m = 1<<1; // left shift key
        } else if ('1' <= c && c <= '9')
            c = c - '1' + 0x1e;
        else switch (c) {
          // decode a smattering of other chars
          case '(': c = 0x26; m = 2; break;
          case ')': m = 2; // fall through
          case '0': c = 0x27; break;
          case ' ': c = 0x2c; break;
          case '_': m = 2; // fall through
          case '-': c = 0x2d; break;
          case '+': m = 2; // fall through
          case '=': c = 0x2e; break;
          case '[': c = 0x2f; break;
          case ']': c = 0x30; break;
          case ';': c = 0x33; break;
          case ',': c = 0x36; break;
          case '.': c = 0x37; break;
          case '/': c = 0x38; break;
          case '\n': c = 0x28; break;
          default: c = 0x55; // keypad '*' for non-decoding chars
        }
        report[0] |= m;
        report[j++] = c;
    }
    for (uint8_t i=0; i<sizeof(matrix)-1; i++) {
        uint8_t m = matrix[i];
        if (m) {
            // one or more bits are set; decode which they are and encode those keys
            uint8_t k = i<<3;
            do {
                if (m & 1) {
                    // key k is pressed
                    if (j < 8)
                        report[j++] = k;
                    else {
                        // overflow; sent a report array filled with 0x01
                        report[2] = report[3] = report[4] = report[5] = report[6] = report[7] = 0x01;
                        return;
                    }
                }
                m >>= 1;
                k++;
            } while (m);
        } // else skip the whole byte m and move on
    }
    // zero out the rest of the report
    while (j < 8)
        report[j++] = 0;
}

//-------------------------------------------------------------------------
// LUFA USB processing and callbacks

/** Buffer to hold the previously generated Keyboard HID report, for comparison purposes inside the HID class driver. */
static uint8_t prev_report[8];

static USB_ClassInfo_HID_Device_t usb_hid_keyboard = {
    .Config = {
        .InterfaceNumber = 0,
        .ReportINEndpoint = {
            .Address = ENDPOINT_DIR_IN | 1,
            .Size = 8,
            .Banks = 1,
        },
        .PrevReportINBuffer         = prev_report,
        .PrevReportINBufferSize     = sizeof(prev_report),
    },
    // and the rest is init'ed to 0 and maintained by the HID class driver
};


static uint8_t usb_report_proto; // we don't use this, but we need to keep track for the host of whether we are in the normal or boot report mode. 1=normal, 0=boot (matches what USB HID sends)

// USB bus is connected and USB host is enumerating us
void EVENT_USB_Device_Connect(void) {
    //PORTE = 1<<6; // light LED for debug
    // just in case we happened to have power before this (if we were not USB bus powered) reset our USB state
    usb_report_proto = 1; // go back to normal report protocol (not that we do anything with it)
}

// USB host is sending a SetConfig packet. it's time to setup the endpoints
void EVENT_USB_Device_ConfigurationChanged(void) {
    // setup endpoint 1 to hold 8-byte messages
    //PORTE = 1<<6; // light LED for debug
    //Endpoint_ConfigureEndpoint(ENDPOINT_DIR_IN|1, EP_TYPE_INTERRUPT, 8, 1);
    HID_Device_ConfigureEndpoints(&usb_hid_keyboard);
    USB_Device_EnableSOFEvents(); // enable EVENT_USB_Device_StartOfFrame() callback
}

// called when the SOF packet is seen [once a millisecond). the HID class driver uses these ticks to handle the Idle timeouts
void EVENT_USB_Device_StartOfFrame(void) {
    HID_Device_MillisecondElapsed(&usb_hid_keyboard);
}

// USB host send a control packet
// the lightly decoded packet is stored in the global USB_ControlRequest
void EVENT_USB_Device_ControlRequest(void) {
    HID_Device_ProcessControlRequest(&usb_hid_keyboard);
}

void CALLBACK_HID_Device_ProcessHIDReport(USB_ClassInfo_HID_Device_t* const intf, const uint8_t id, const uint8_t type, const void* data, const uint16_t len) {
    if (len == 1) {
        // set the keyboard LEDs given the lower bits of report[0]
        uint8_t led = *(const uint8_t*)data;
        // conveniently the USB and PS/2 encodings of the LED bits are different :-)
        // USB:
        //  bit 0...NumLock
        //  bit 1...CapsLock
        //  bit 2...ScrollLock
        // [bit 3...Compose]
        // [bit 4...Kana]
        // PS/2:
        //  bit 0...ScrollLock
        //  bit 1...NumLock
        //  bit 2...CapsLock
        led = (led << 1) | ((led >> 2) & 1);
        led &= 7; // remove extra ScrollLock bit as well as any Compose/Kana and other garbage
        ps2_set_leds(led);
    } // else we don't understand what the host just sent, so do nothing
}

bool CALLBACK_HID_Device_CreateHIDReport(USB_ClassInfo_HID_Device_t* const intf, uint8_t* const id, const uint8_t type, void* data, uint16_t* const len) {
    uint8_t* report = (uint8_t*)data;
    *len = 8;
    *id = 0; // we aren't using report IDs since we only have one possible report to send to the host
    make_usb_report(report);
    return false; // let HID class driver decide if this new report should be sent
}


//-------------------------------------------------------------------------

int main(void) {

    // init timer0 sufficiently that TIMER0_OVF_vect() and thus millis() will work
    TCCR0A = 0;
    TCCR0B = _BV(CS00) | _BV(CS01); // /64 prescalar
    TIMSK0 = _BV(TOIE0); // enable overflow interrupt

    // make bit 6 (the LED) an output for testing/status
    DDRE = 1<<6;
    PORTE = 1<<6;

    ps2_init();

    USB_Init();
    
    // now that everything is setup, enable interrupts
    sei();

    // put the keyboard in the easiest scan set for us to deal with
    ps2_set_scan_set(3);
    // set all keys to make/break with no repeat (USB does the repeat at the host side)
    _delay_us(1000);
    ps2_write_and_ack(0xf8);

    // and show a rapid pattern on the keyboard LEDs to indicate
    // we have a succesfull connection over PS/2
    _delay_us(1000);
    for (int8_t i=3*2; i>= 0; i--) {
      ps2_set_leds(1<<((uint8_t)i%3));
      _delay_us(125000);
    }
    // turn off keyboard LEDs
    ps2_set_leds(0);
    // turn off our LED
    PORTE = 0;

    while (1) {
        if (1) {
            // sleep until there's something of interest
            set_sleep_mode(SLEEP_MODE_IDLE);
            sleep_enable();
            sleep_cpu();
            // <sleeping>
            sleep_disable();
        }

        ps2_tick();

        if (ps2_available()) {
            uint8_t c = ps2_read();
            uint16_t mu = ps2_to_usb_keycode(c);
            uint8_t u = (uint8_t)mu;
            uint8_t up = mu>>8;
            if (u && ((matrix[u>>3] >> (u&7)) & 1) == up) {
                matrix[u>>3] ^= 1 << (u&7);
            }

            // for debug, blink out the PS/2 code and the USB code
            //static uint8_t blinkie;
            //if (blinkie) blink_byte(c);
            //if (blinkie && u) blink_byte(u);
            //blinkie ^= (mu == 0x56); // keypad '-' toggles blinkie
        }

        if (1) {
            HID_Device_USBTask(&usb_hid_keyboard);
            USB_USBTask();
        }
    }
}

// pull in pieces of arduino standard library that we need for millis() to work
// (nice and quick and a dirty hack :-)


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

#define clockCyclesPerMicrosecond() ( F_CPU / 1000000L )
#define clockCyclesToMicroseconds(a) ( (a) / clockCyclesPerMicrosecond() )
#define microsecondsToClockCycles(a) ( (a) * clockCyclesPerMicrosecond() )

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

ISR(TIMER0_OVF_vect) {
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

