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

#include "ps2.h"

static volatile uint8_t buffer[42]; // buffer of unread bytes from the ps/2 keyboard
static volatile uint8_t head, tail; // indexes into buffer[]

static volatile uint8_t send_FE; // boolean; when non-zero we should send send_FE (resend) to the keyboard because we received a byte with bad parity

ISR(USART1_RX_vect) {
    // unload the UART receive buffer and stash it in buffer[]
    uint8_t status;
    while ((status = UCSR1A) & (1<<RXC1)) {
        // Note: the error flags in UCSR1A apply to the byte yet to be read from UDR1
        // in other words, once we read UDR1 the fifo advances and the bits in UCSR1A apply to the byte after c, so don't re-read UCSR1A
        uint8_t c = UDR1;
        if (status & ((1<<FE1)|(1<<DOR1)|(1<<UPE1))) {
            debug("UART err 0x%x\n", status);
            // rx has failed in some way
            //  FE1 (framing error) means the Stop bit wasn't a 1, which means we're out of sync somehow
            //  DOR1 (data overrun) means the interrupt didn't happen faster enough
            //  PE1 (parity error) means the 9th bit (odd parity) wasn't right
            // for framing and parity errors we send an FE back to the keyboard, asking it to resend the byte
            // for overrun we don't know how many bytes we lost. hopefully only one.
            send_FE = 1;
            PORTE = 1<<6; // and light the LED until we get the proper code back
            // and we throw away 'c'
        } else {
            // stash c in the buffer
            uint8_t h = head + 1;
            if (h == sizeof(buffer))
                h = 0;
            if (h != tail) {
                buffer[h] = c;
                head = h;
            } else {
                // else we've overflowing buffer. buffer[] is large and this shouldn't happen
                debug("buffer[] full\n");
            }
        }
    }
}


void ps2_tick(void) {

    // if the ISR needs a byte resent, send FE to the keyboard
    if (send_FE) {
        _delay_us(1000); // give the keyboard a little time before we write to it
        if (ps2_write(0xfe)) { // send an FE (resend command)
            send_FE = 0;
            // and clear the LED
            PORTE = 0;
        }
        // else leave send_FE set and we'll retry the send at the next call to ps2_tick()
    }
}

// are there scancodes available?
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
    switch (c) {
        // show the non-keystroke bytes
        case 0xfe: case 0xfa: case 0xaa: case 0x00: case 0xff:
            debug("ps2_read() = 0x%x\n", c);
            break;
    }
    return c;
}

// return true if PS2 bus is idle and nothing is pending in the UART rx buffer
static inline uint8_t idle(void) {
    return (PIND & (_BV(PS2_CLK_PIN) | _BV(PS2_DATA_PIN))) == (_BV(PS2_CLK_PIN) | _BV(PS2_DATA_PIN))
           && !(UCSR1A & (1<<RXC1));
}

// send a byte to the keyboard. returns true if the write suceeded, else false
uint8_t _ps2_write(uint8_t v) {
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
    while (!idle() && now_ms - start_ms <= 100)
        now_ms = millis(); // spin

    if (!idle())
        // Clk and Data aren't high; something is stuck and we can't send
        return 0;

    // emulate the PS motherboard I scoped and wait 19 usec after Clk is high before starting
    // Note that I don't really know if Clk just transitioned low->high, but just in case
    for (uint8_t i=0; i<19; i++) {
        _delay_us(1); // one PS/2 bit time is roughly 100 usec, so we should see any bit starting pretty easily
        if (!idle())
            goto wait_for_idle_bus;
    }
    // OK at this point we believe the PS/2 bus is idle and we're going to grab it and go

    // disable UART rx, which returns the PS/2 Clk and Data pins to being regular GPIO pins we can drive
    // Note that there is a race here if the keyboard starting sending between the last idle() check and now.
    // If that happens we'll just collide and the keyboard will have to back off as per the ps/2 protocol.
    UCSR1B &= ~(1<<RXEN1);

    // pull Clk low, which inhibits the keyboard from sending
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
        UCSR1B |= (1<<RXEN1); // re-enable UART RX on the way out
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

    // waiting for the bus to go back to idle.
    // we don't want the XCLK1 to be low when we set RXEN1 just in case that confuses the UART
    if (1) {
        // wait for the bus to be back to idle state before returning
        while (!idle() && now_ms - start_ms <= 100)
            now_ms = millis(); // spin

        if (!idle()) {
            // Clk and Data aren't high; something is stuck
            PORTE = 1<<6; // light the LED until we get this fixed
            UCSR1B |= (1<<RXEN1); // we'll re-enable UART RX, though who knows what is going on
            return 0;
        }
    }

    // re-enable UART rx
    UCSR1B |= (1<<RXEN1);

    return 1;
}

uint8_t ps2_write(uint8_t v) {
    debug("ps2_write(0x%x) = ", v);
    uint8_t rc = _ps2_write(v);
    debug("%u\n", rc);
    return rc;
}

// write a byte and wait for the 0xFA ack
// handle resending the byte if need be
uint8_t ps2_write_and_ack(uint8_t v) {
    // retry the whole transactions 8 times before giving up
    for (uint8_t try=0; try<8; try++) {
      // try to send the byte
      if (!ps2_write(v))
          // there was a collision or a missing low level ACK; retry
          continue;

      // wait for the response
      unsigned long start_ms = millis();
      while (millis() - start_ms < 250) { // give the keyboard .25 sec to get us a response. normally it takes just a msec or two
        if (ps2_available())
          goto got_reply;
      }
      // timed out waiting for a response
      // should we retry , or give up?
      continue; // let's retry

got_reply:;
      uint8_t r = ps2_read();
      if (r == 0xFA) {
        // yay, an ACK from the keyboard, we are successfull
        return 1;
      } else if (r == 0xFE) {
        // keyboard wants that byte resent, so retry from the top
        continue;
      } else {
        // a strange response from the keyboard
        // should we ignore it? given up? retry?
        continue; // let's retry
      }
    }
    // given up and fail
    return 0;
}

uint8_t ps2_write2(uint8_t a, uint8_t b) {
    uint8_t rc = ps2_write_and_ack(a);
    if (rc) {
        _delay_us(1000);
        rc = ps2_write_and_ack(b);
    }
    return rc;
}

// send { 0xED, v } to the keyboard
uint8_t ps2_set_leds(uint8_t v) {
    return ps2_write2(0xed,v);
}

uint8_t ps2_set_scan_set(uint8_t v) {
    return ps2_write2(0xf0,v);
}

void ps2_init() {
    // initialize both clk and data to be pulled-up input pins
    // (when not using the UART we'll make use of this configuration)
    DDRD = 0; // not really needed since the reset value is 0
    PORTD = _BV(PS2_CLK_PIN) | _BV(PS2_DATA_PIN); // pull up both wires

    // since we will be using the UART for PS/2 receive it doesn't look
    // like I can also use the internal pullups (Since the uart, when enabled,
    // takes over those pins)
    
    // setup UART1 so it can receive PS/2 data. 8 bits + 1 odd parity bit + 1 stop bit
    // and latch the data on the falling edge of the XCLK1 signal
    UCSR1A = 0;
    UCSR1B = (0<<UCSZ12); // 8 data bits
    UCSR1C = (1<<UMSEL10) | // synchronous clock mode
             (1<<UPM11) | (1<<UPM10) | // enable odd parity verification
             (0<<USBS1) | // 1 stop bit (value is 0)
             (3<<UCSZ10) | // 8 data bits
             (0<<UCPOL1); // sample rx data on falling edge of clock
    // the ATmega32u4 UART documentation mentions a UCSR1D register, however the same pdf's register map omits it, and the avr header file doesn't define one,
    // and finally there isn't any pin dedicated to CTS and RTS in the pinout, so I think there isn't such a register. If there is, the reset value is 0
    // and what is what we would want.
    // we use an external clock so there is no point in setting up the internal baud rate

    // finally, enable UART receive and receive interrupt (we leave UART transmit disabled always, the complex PS/2 host transmit protocol is done in software
    UCSR1B |= (1<<RXEN1) | (1<<RXCIE1);
}

