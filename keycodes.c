
// the mapping from PS/2 keycodes to USB is a PITA, and c++ not supporting the designated array initializers makes me use good ole C for this

#include "keycodes.h"
#include <avr/pgmspace.h>   // tools used to store variables in program memory

//-------------------------------------------------------------------------
// the mapping from PS/2 to USB keycodes
// for PS/2 AT codeset 3 (the easiest to implement)

// see http://www.freebsddiary.org/APC/usb_hid_usages.php for USB key codes
// see 
static const uint8_t PROGMEM simple_ps2_to_usb_map[] = {
    [0x76] = 0x29, // ESC
    [0x05] = 0x3a, // F1
    [0x06] = 0x3b, // F2
    [0x04] = 0x3c, // F3
    [0x0C] = 0x3d, // F4
    [0x03] = 0x3e, // F5
    [0x0B] = 0x3f, // F6
    [0x83] = 0x40, // F7
    [0x0A] = 0x41, // F8
    [0x01] = 0x42, // F9
    [0x09] = 0x43, // F10
    [0x78] = 0x44, // F11
    [0x07] = 0x45, // F12
    [0x7E] = 0x47, // SCROLL LOCK

    [0x0E] = 0x35, // ` and ~
    [0x16] = 0x1e, // 1
    [0x1E] = 0x1f, // 2
    [0x26] = 0x20, // 3
    [0x25] = 0x21, // 4
    [0x2E] = 0x22, // 5
    [0x36] = 0x23, // 6
    [0x3D] = 0x24, // 7
    [0x3E] = 0x25, // 8
    [0x46] = 0x26, // 9
    [0x45] = 0x27, // 0
    [0x4E] = 0x2d, // - and _
    [0x55] = 0x2e, // = and +
    [0x66] = 0x2a, // BACKSPACE
    [0x77] = 0x53, // NUM LOCK
    [0x7C] = 0x55, // K *
    [0x7B] = 0x56, // K -

    [0x0D] = 0x2b, // TAB
    [0x15] = 0x14, // Q
    [0x1D] = 0x1a, // W
    [0x24] = 0x08, // E
    [0x2D] = 0x15, // R
    [0x2C] = 0x17, // T
    [0x35] = 0x1c, // Y
    [0x3C] = 0x18, // U
    [0x43] = 0x0c, // I
    [0x44] = 0x12, // O
    [0x4D] = 0x13, // P
    [0x54] = 0x2f, // [ and {
    [0x5B] = 0x30, // ] and }
    [0x5A] = 0x28, // Return
    [0x6C] = 0x5f, // K 7
    [0x75] = 0x60, // K 8
    [0x7D] = 0x61, // K 9
    [0x79] = 0x57, // K +

    //[0x58] = 0x39, // CAPS LOCK
    [0x14] = 0x39, // CAPS LOCK
    [0x1C] = 0x04, // A
    [0x1B] = 0x16, // S
    [0x23] = 0x07, // D
    [0x2B] = 0x09, // F
    [0x34] = 0x0a, // G
    [0x33] = 0x0b, // H
    [0x3B] = 0x0d, // J
    [0x42] = 0x0e, // K
    [0x4B] = 0x0f, // L
    [0x4C] = 0x33, // ; and :
    [0x52] = 0x34, // ' and "
    [0x5D] = 0x32, // # and ~ (non-US key)
    [0x6B] = 0x5c, // K 4
    [0x73] = 0x5d, // K 5
    [0x74] = 0x5e, // K 6

    [0x12] = 0xe1, // LEFT SHIFT
    [0x61] = 0x31, // \ and |
    [0x1A] = 0x1d, // Z
    [0x22] = 0x1b, // X
    [0x21] = 0x06, // C
    [0x2A] = 0x19, // V
    [0x32] = 0x05, // B
    [0x31] = 0x11, // N
    [0x3A] = 0x10, // M
    [0x41] = 0x36, // , and <
    [0x49] = 0x37, // . and >
    [0x4A] = 0x38, // / and ?
    [0x59] = 0xe5, // RIGHT SHIFT
    [0x69] = 0x59, // K 1
    [0x72] = 0x5a, // K 2
    [0x7A] = 0x5b, // K 3

    [0x14] = 0xe0, // LEFT CTRL
    [0x11] = 0xe2, // LEFT ALT
    [0x29] = 0x2c, // SPACE
    [0x70] = 0x62, // K 0
    [0x71] = 0x63, // K .

    // ----- above this line the mapping is identical in codeset 2 and 3-------
    // ----- below this line is specific to codeset 3
    [0x64] = 0x4c, // DELETE
    [0x65] = 0x4d, // END
    [0x6e] = 0x4a, // HOME
    [0x67] = 0x49, // INSERT
    [0x57] = 0x46, // PRINT SCREEN
    [0x79] = 0x58, // K ENTER
    [0x77] = 0x54, // K /
    [0x6f] = 0x4b, // PAGE UP
    [0x6d] = 0x4e, // PAGE DOWN
    [0x62] = 0x48, // PAUSE/BREAK
    [0x58] = 0xe4, // RIGHT CONTROL
    [0x39] = 0xe6, // RIGHT ALT
    [0x6a] = 0x4f, // RIGHT ARROW
    [0x61] = 0x50 , // LEFT ARROW
    [0x60] = 0x51, // DOWN ARROW
    [0x63] = 0x52, // UP ARROW
    
    // ----- below this line are mappings specific to Northgate keyboards -----
    // (they should be no-ops on regular PS/2 keyboards)
    // the OMNI key (center key of arrow compass) sends 0x60, same as a
    // down arrow key, so we can't map it to something special :-(
};

// PS/2 codeset 2 mapping for the byte after the E0 prefix
static const uint8_t PROGMEM e0_ps2_to_usb_map[] = {
    [0x1f] = 0xe3, // LEFT GUI
    [0x11] = 0xe6, // RIGHT ALT
    [0x27] = 0xe7, // RIGHT GUI
    [0x2f] = 0x76, // MENU
    [0x14] = 0xe4, // RIGHT CONTROL
    [0x75] = 0x52, // UP ARROW
    [0x72] = 0x51, // DOWN ARROW
    [0x6b] = 0x50, // LEFT ARROW
    [0x74] = 0x51, // RIGHT ARROW
    [0x5a] = 0x58, // K ENTER
    [0x4a] = 0x54, // K /
    [0x71] = 0x4c, // DELETE
    [0x69] = 0x4d, // END
    [0x7a] = 0x4e, // PAGE DOWN
    // keys which usually come as a 2-key sequence, the first key
    // being E0,12. We ignore the 12 and only look at the 2nd key
    // since all the 2nd keys are unique. if that someday is no
    // longer the case then this needs to get more complicated.
    [0x12] = 0, // ignore E0,12
    [0x70] = 0x49, // INSERT
    [0x6c] = 0x4a, // HOME
    [0x7d] = 0x4b, // PAGE UP
    [0x7c] = 0x46, // PRINT SCREEN
};


// map a PS/2 key code to a USB key code, and the UP (release) flag in bit 15
// this function is where we keep track of the PS/2 state machine
uint16_t ps2_to_usb_keycode(uint8_t pc) {
    static uint8_t state; // lower nibble holds the lookup table, bit 7 is the UP flag, but 6 is the PAUSE/BREAK prefix (E1+14) flag
    uint16_t uc = 0;
    if (pc == 0xf0) { // UP prefix
      state |= 0x80;
    } else if (pc == 0xe0) { // extended set 0 prefix
        // Note that normal PS/2 always sends the prefix before the F0
        // so we can simply set state rather than carefully manipulate it
        state = 1;
    } else if (pc == 0xe1) { // extended set 1 prefix
        state = 2;
    } else {
        switch (state & 3) {
        case 0: // normal key table
            if (pc == 0x77 && (state & 0x40)) {
                uc = 0x48; // PAUSE/BREAK
            } else if (pc < sizeof(simple_ps2_to_usb_map))
                uc = pgm_read_byte(&simple_ps2_to_usb_map[pc]);
            break;
        case 1: // E0 extended table
            break;
            if (pc < sizeof(e0_ps2_to_usb_map))
                uc = pgm_read_byte(&e0_ps2_to_usb_map[pc]);
            break;
        case 2: // E1 extended table
            break;
            // actually here is only one E1 keystroke, the
            // PAUSE/BREAK key, which is a two key sequence
            //  E1+14 followed by 77
            // Normally 77 is NUM LOCK, so what we do is set a
            // flag and decode 77 differently in case 0
            if (pc == 0x14)
                state |= 0x40;
            break;
        }
        if (uc)
            // add the UP flag in bit 15
            uc |= (uint16_t)(state&0x80)<<8;
        // and in any case reset the state to normal
        state = 0;
    }

    return uc;
}

