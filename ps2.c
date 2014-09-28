
#include "ps2.h"

static volatile uint8_t buffer[32]; // buffer of unread bytes from the ps2 keyboard
static volatile uint8_t head, tail; // indexes into buffer[]

static volatile uint8_t send_FE; // boolean; when true we should send FE (resend) to the keyboard because we received a byte with bad parity

// The ISR for the external interrupt pin which happens on the falling edge of the PS2 Clock signal
static volatile uint8_t bitcount=0;
static volatile unsigned long prev_ms=0;
ISR(INT0_vect) {
    static uint8_t incoming;
    static uint8_t parity;

    // read the data pin
    const uint8_t val = (PIND >> PS2_DATA_PIN) & 1;

    uint8_t n = bitcount;

    unsigned long now_ms = millis();
    if (now_ms - prev_ms > 100) {
        // it's been too long; start over
        n = 0;
        incoming = 0;
        parity = 0;
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

    const uint8_t bn = n-1;
    if (bn < 8) { // note bn is unsigned, so this skips both the start bit and the parity and stop bits
        incoming |= val << bn;
    }
    if (bn < 9) {
        // XOR the 8 data bits and the parity bit together
        // the result should be a 1
        parity ^= val;
    }
    n++;
    if (n == 11) {
        if (parity) {
            uint8_t h = head + 1;
            if (h == sizeof(buffer))
                h = 0;
            if (h != tail) {
                buffer[h] = incoming;
                head = h;
            }
        } else {
            // the parity was bad and we should ask the keyboard to resend the byte
            if (1) {
                unsigned long start = millis();
                while (millis()-start < 4000) {
                  _delay_us(25000);
                  PORTE ^= 1<<6;
                }
            }
            //send_FE = 1;
        }
        n = 0;
        incoming = 0;
        parity = 0;
    }
    bitcount = n;
}

void ps2_tick(void) {
    if (send_FE) {
        _delay_us(1000); // give the keyboard a little time before we write to it
        if (ps2_write(0xFE)) // send an FE (resend command)
            send_FE = 0;
        // else leave send_FE set and we'll retry the send at the next call to ps2_tick()
    }
}

// are there scancodes availble?
uint8_t ps2_available(void) {
    return head != tail;
}

// read the next scancode
uint8_t ps2_read(void) {
    uint8_t t = tail;
    if (t == head)
        return 0;
    t++;
    if (t >= sizeof(buffer))
        t = 0;
    uint8_t c = buffer[t];
    tail = t;
    return c;
}

// return true if PS2 bus is idle
static inline uint8_t idle(void) {
    return (PIND & (_BV(PS2_CLK_PIN) | _BV(PS2_DATA_PIN))) == (_BV(PS2_CLK_PIN) | _BV(PS2_DATA_PIN));
}

// send a byte to the keyboard. returns true if the write suceeded, else false
uint8_t ps2_write(uint8_t v) {
    // to send a byte to the keyboard over the PS/2 (aka AT) protocol, the host (us)
    // has to twiddle the lines to get the keyboard's attention, and then let the keyboard
    // clock the bits at its own rate
    // first wait for Clk to be high and any in-progress byte from the keyboard to finish arriving.
    // note that since the keyboard sends Ack bytes (0xFA) after most command bytes, there is likely an FA being received
    // at the same time that we are trying to send the 2nd byte of a multi-byte command.
    // We usually are in a  race with the keyboard to see who sends first when it comes time for us to send the 2nd byte. 
    // The keyboard will skip sending the FA if we overwrite the keyboard (say the IBM spec).
    // The 100 msec is a sanity check timeout
wait_for_idle_bus:;
    unsigned long start_ms = millis();
    unsigned long now_ms = start_ms;
    while ((!idle() || (bitcount && now_ms - prev_ms <= 100)) && now_ms - start_ms <= 100)
        now_ms = millis(); // spin

    if ((PIND & (_BV(PS2_CLK_PIN)|_BV(PS2_DATA_PIN))) != (_BV(PS2_CLK_PIN)|_BV(PS2_DATA_PIN)))
        // Clk and Data aren't high; something is stuck and we can't send
        return 0;

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
        return 0;
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
        return 0;
    }
    }

    EIFR = _BV(PS2_CLK_INT); // ack away the pending INT0 (which happened because we caused falling edges on Clk when sending, so an INT0 is, at this point, pending)
    EIMSK = _BV(PS2_CLK_INT); // re-enable INT0
    return 1;
}

// send { 0xED, v } to the keyboard
void ps2_set_leds(uint8_t v) {
    ps2_write2(0xed,v);
}

void ps2_set_scan_set(uint8_t v) {
    ps2_write2(0xf0,v);
}

void ps2_write2(uint8_t a, uint8_t b) {
    if (ps2_write(a)) {
        if (1) {
            // wait for the ACK byte before sending the next byte
            while (!ps2_available());
            uint8_t fa = ps2_read();
            if (fa != 0xFA) {
                // we should have received an ACK byte
                die_blinking(fa);
            }
        }
        _delay_us(1000);
        if (ps2_write(b)) {
            if (0) {
                while (!ps2_available());
                uint8_t fa = ps2_read();
                if (fa != 0xFA) {
                    // we should have received an ACK byte
                    die_blinking(fa);
                }
            }
        }
    }
}

void ps2_init() {
  // initialize both clk and data to be pulled-up input pins
  DDRD = 0; // not really needed since the reset value is 0
  PORTD = _BV(PS2_CLK_PIN) | _BV(PS2_DATA_PIN); // pull up both wires
 
  // configure PS2_CLK_INT on the falling edge
  EICRA = _BV(ISC01<<(2*PS2_CLK_INT)); // interrupt on a falling edge of INT0
  EIMSK = _BV(PS2_CLK_INT); // enable INT0
}

