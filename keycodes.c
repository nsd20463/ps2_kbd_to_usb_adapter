
// the mapping from PS/2 keycodes to USB is a PITA, and c++ not supporting the designated array initializers makes me use good ole C for this

#include "keycodes.h"
#include <avr/pgmspace.h>   // tools used to store variables in program memory

//-------------------------------------------------------------------------
// the mapping from PS/2 to USB keycodes

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
//        PrtSc:  E012E07C E0F07CE0F012
    [0x7E] = 0x47, // SCROLL LOCK
//        Break:  E11477 E1F014F077

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
//        Ins:    E012E070 E0F070E0F012
//        Home:   E012E06C E0F06CE0F012
//        PgUp:   E012E07D E0F07DE0F012
    [0x77] = 0x53, // NUM LOCK
//        K/:     E04A E0F04A
    [0x7C] = 0x55, // K*
    [0x7B] = 0x56, // K-

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
//        Del:    E071 E0F071
//        End:    E069 E0F069
//        PgDn:   E07A E0F07A
    [0x6C] = 0x5f, // K7
    [0x75] = 0x60, // K8
    [0x7D] = 0x61, // K9
    [0x79] = 0x57, // K+

    [0x58] = 0x39, // CAPS LOCK
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
    [0x6B] = 0x5c, // K4
    [0x73] = 0x5d, // K5
    [0x74] = 0x5e, // K6

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
//        Up:     E075 E0F075
    [0x69] = 0x59, // K1
    [0x72] = 0x5a, // K2
    [0x7A] = 0x5b, // K3
//        Enter:  E05A E0F05A

    [0x14] = 0xe0, // LEFT CTRL
//        LSuper: E01F E0F01F
    [0x11] = 0xe2, // LEFT ALT
    [0x29] = 0x2c, // SPACE
//        RAlt:   E011 E0F011
//        RSuper: E027 E0F027
//        Menu:   E02F E0F02F
//        RCtrl:  E014 E0F014
//        Left:   E06B E0F06B
//        Down:   E072 E0F072
//        Right:  E074 E0F074
    [0x70] = 0x62, // K0
    [0x71] = 0x63, // K.
};

// map a PS/2 key code to a USB key code
uint8_t ps2_to_usb_keycode(uint8_t pc) {
    uint8_t uc;
    if (pc < sizeof(simple_ps2_to_usb_map)) {
        uc = simple_ps2_to_usb_map[pc];
        if (uc)
            return uc;
    }
    return 0;
}

