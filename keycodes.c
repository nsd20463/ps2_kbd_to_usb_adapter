
// the mapping from PS/2 keycodes to USB is a PITA, and c++ not supporting the designated array initializers makes me use good ole C for this

#include "keycodes.h"
#include <avr/pgmspace.h>   // tools used to store variables in program memory

//-------------------------------------------------------------------------
// the mapping from PS/2 to USB keycodes
// for PS/2 AT codeset 3 (the easiest to implement)

// see http://www.freebsddiary.org/APC/usb_hid_usages.php for USB key codes
// note that I have order these by the "standard" US keyboard layout which is slightly different from the Northgate layout, but I expect most people are looking at a standard cheap-ass keyboard
static const uint8_t PROGMEM simple_ps2_to_usb_map[] = {
    [0x08] = 0x29, // ESC
    [0x07] = 0x3a, // F1
    [0x0f] = 0x3b, // F2
    [0x17] = 0x3c, // F3
    [0x1f] = 0x3d, // F4
    [0x27] = 0x3e, // F5
    [0x2f] = 0x3f, // F6
    [0x37] = 0x40, // F7
    [0x3f] = 0x41, // F8
    [0x47] = 0x42, // F9
    [0x4f] = 0x43, // F10
    [0x56] = 0x44, // F11
    [0x5e] = 0x45, // F12
    [0x5f] = 0x47, // SCROLL LOCK

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
    [0x61] = 0x31, // \ and | (US keyboard, above RETURN)

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
    [0x5D] = 0x32, // # and ~ (the extra key on a 102 key keyboard)
    [0x5A] = 0x28, // RETURN

    [0x12] = 0xe1, // LEFT SHIFT
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
    [0x5c] = 0x31, // \ and | (non-US keyboard, right of RIGHT SHIFT)

    [0x11] = 0xe0, // LEFT CTRL
    // [0x] = 0xe3, LEFT GUI
    [0x19] = 0xe2, // LEFT ALT
    [0x29] = 0x2c, // SPACE
    [0x39] = 0xe6, // RIGHT ALT
    // [0x] = 0xe7, // RIGHT GUI
    // [0x] = 0x76, // MENU
    [0x58] = 0xe4, // RIGHT CONTROL

    [0x57] = 0x46, // PRINT SCREEN
    [0x62] = 0x48, // PAUSE/BREAK

    [0x67] = 0x49, // INSERT
    [0x6e] = 0x4a, // HOME
    [0x6f] = 0x4b, // PAGE UP

    [0x64] = 0x4c, // DELETE
    [0x65] = 0x4d, // END
    [0x6d] = 0x4e, // PAGE DOWN

    [0x63] = 0x52, // UP ARROW

    [0x61] = 0x50, // LEFT ARROW
    [0x60] = 0x51, // DOWN ARROW
    [0x6a] = 0x4f, // RIGHT ARROW
    
    [0x76] = 0x53, // NUM LOCK
    [0x77] = 0x54, // K /
    [0x7e] = 0x55, // K *
    [0x84] = 0x56, // K -

    [0x6C] = 0x5f, // K 7
    [0x75] = 0x60, // K 8
    [0x7D] = 0x61, // K 9
    [0x7c] = 0x57, // K +

    [0x6B] = 0x5c, // K 4
    [0x73] = 0x5d, // K 5
    [0x74] = 0x5e, // K 6

    [0x69] = 0x59, // K 1
    [0x72] = 0x5a, // K 2
    [0x7A] = 0x5b, // K 3

    [0x70] = 0x62, // K 0
    [0x71] = 0x63, // K .
    [0x79] = 0x58, // K ENTER

    // ----- below this line are mappings specific to Northgate keyboards -----
    // (they should be no-ops on regular PS/2 keyboards)
    // the OMNI key (center key of arrow compass) sends the 2-byte
    // seqence E0+73 even in codeset 3
    // the extra '*' key in the main keyboards sends 7E, same as the
    // '*' in the numeric keypad, so we can't tell them apart
    // the extra '=' key in the numeric keypad sends 0x%%, same as the
    // '=/+' key in the main keyboard (except the Northgate removes and
    // restores any SHIFT modifier so you don't get '+')
};

// PS/2 codeset 3 (not 2) mapping for the byte after the E0 prefix
// a "standard" PS/2 keyboard in codeset 3 does not use the E0 prefix
// however my Northgate OmniKey Ultra uses it for the OMNI key
static const uint8_t PROGMEM e0_ps2_to_usb_map[] = {
    // ---- Northgate specific keys ---------
    [0x73] = 0x51, // OMNI, mapped to DOWN ARROW because I find that the most useful thing to do. Maybe in the future I could use OMNI to reprogram the ATmega32u4. then again print-screen,scroll lock and pause, and even caps lock are equally useless and available
};


// map a PS/2 key code to a USB key code, and the UP (release) flag in bit 8
// this function is where we keep track of the PS/2 state machine
// NOTE the largest keycode value this function returns is E7, since nothing past that is defined for USB. The code and array in main.c assumes this behavior.
uint16_t ps2_to_usb_keycode(uint8_t pc) {
    static uint8_t state; // bit 0 is the E0 flag; bit 7 is the UP flag
    uint16_t uc = 0;
    if (pc == 0xf0) { // UP prefix
        state |= 0x80;
    } else if (pc == 0xe0) { // extended set 0 prefix
        // Note that normal PS/2 always sends the prefix before the F0
        // so we can simply set state rather than carefully manipulate it
        state = 1;
    } else {
        switch (state & 1) {
        case 0: // normal key table
            if (pc < sizeof(simple_ps2_to_usb_map))
                uc = pgm_read_byte(&simple_ps2_to_usb_map[pc]);
            break;
        case 1: // E0 extended table
            if (pc < sizeof(e0_ps2_to_usb_map))
                uc = pgm_read_byte(&e0_ps2_to_usb_map[pc]);
            break;
        }
        if (uc)
            // add the UP flag in bit 8 of the result
            uc |= (uint16_t)(state&0x80)<<1;
        // and reset the state to normal
        state = 0;
    }

    return uc;
}

