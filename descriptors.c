// the LUFA USB descriptor macros only work in C, so I have to split them out from the C++ code

#include <LUFA/Drivers/USB/USB.h>

static const USB_Descriptor_Device_t PROGMEM usb_device_desc = {
	.Header                 = {.Size = sizeof(USB_Descriptor_Device_t), .Type = DTYPE_Device},

	.USBSpecification       = VERSION_BCD(1,1,0), // we implement USB 1.10 spec (no need to go newer than that and loose compatability with old hosts)
	.Class                  = USB_CSCP_NoDeviceClass, // our classes are at the interface level, like commercial keyboards do
	.SubClass               = USB_CSCP_NoDeviceSubclass,
	.Protocol               = USB_CSCP_NoDeviceProtocol,

	.Endpoint0Size          = FIXED_CONTROL_ENDPOINT_SIZE,

	.VendorID               = 0x03EB,
	.ProductID              = 0x2042, // VID/PID are those donated by ATmel for the LUFA keyboard projects
	.ReleaseNumber          = VERSION_BCD(6,0,4), // ReleaseNumber can be anything you want. LUFA suggests using this field to destinguish different projects. I use the revision number from my Northgate just for yucks

	.ManufacturerStrIndex   = 1,
	.ProductStrIndex        = 2,
	.SerialNumStrIndex      = 3, //NO_DESCRIPTOR,

	.NumberOfConfigurations = FIXED_NUM_CONFIGURATIONS,
};

static const USB_Descriptor_HIDReport_Datatype_t PROGMEM usb_report_desc[] = {
    HID_RI_USAGE_PAGE(8,1), // generic desktop controls
    HID_RI_USAGE(8, 6), // keyboard
    HID_RI_COLLECTION(8, 1), // application

        // the modifier keys
        HID_RI_USAGE_PAGE(8, 7), // key codes
        HID_RI_USAGE_MINIMUM(8, 0xe0), // minimum modified code (Control Left)
        HID_RI_USAGE_MAXIMUM(8, 0xe7), // maximum modified code (GUI Right)
        HID_RI_LOGICAL_MINIMUM(8, 0),
        HID_RI_LOGICAL_MAXIMUM(8, 1),
        // note the commercial keyboard has SIZE before COUNT for the modifier keys. I presume this work around something somewhere and will do the same
        // note that the example keyboard report descriptor in the USB HID spec has this oddity as well. perhaps it all goes back to that
        HID_RI_REPORT_SIZE(8, 1), // each key uses 1 bit
        HID_RI_REPORT_COUNT(8, 8), // there are 8 modifier keys
        HID_RI_INPUT(8, HID_IOF_DATA | HID_IOF_VARIABLE | HID_IOF_ABSOLUTE),

        // no idea what this byte is yet. might be padding to pad out the modifier to 16 bits
        // the commercial keyboard does it too, so it must be important somewhere, though really it seems a waste of a perfectly good byte
        // Aha, it is because the USB Boot Keyboard device spec defines a fixed 8-byte report, and our actual report must match this
        HID_RI_REPORT_COUNT(8, 1),
        HID_RI_REPORT_SIZE(8, 8),
        HID_RI_INPUT(8, HID_IOF_CONSTANT),

        // the LEDs
        HID_RI_REPORT_COUNT(8, 3), // we have 3 LEDs; note commercial keyboards report 5 (Kana) even when they don't have more than the usual 3 LEDs.
        HID_RI_REPORT_SIZE(8, 1), // each LED needs 1 bit
        HID_RI_USAGE_PAGE(8, 8), // LEDs
        HID_RI_USAGE_MINIMUM(8, 1), // Num-Lock
        HID_RI_USAGE_MAXIMUM(8, 3), // Scroll-Lock; note commercial keyboards report 5 (Kana) even when they don't have more than the usual 3 LEDs.
        HID_RI_OUTPUT(8, HID_IOF_DATA | HID_IOF_VARIABLE | HID_IOF_ABSOLUTE),

        // padding to pad out the LED byte
        HID_RI_REPORT_COUNT(8, 1),
        HID_RI_REPORT_SIZE(8, 8-3),
        HID_RI_OUTPUT(8, HID_IOF_CONSTANT),

        // normal keys
        HID_RI_REPORT_COUNT(8, 6), // up to 6 normal keys at a time
        HID_RI_REPORT_SIZE(8, 8), // 8 bits/key
        HID_RI_LOGICAL_MINIMUM(8, 0),
        HID_RI_LOGICAL_MAXIMUM(8, 0xff), // commercial keyboard reports 255, as does the USB HIB Boot Keyboard spec, so we do too
        HID_RI_USAGE_PAGE(8, 7), // key codes
        HID_RI_USAGE_MINIMUM(8, 0),
        HID_RI_USAGE_MAXIMUM(8, 0xff), // commercial keyboard reports 255, as does the USB HIB Boot Keyboard spec, so we do too
        HID_RI_INPUT(8, HID_IOF_DATA | HID_IOF_ARRAY | HID_IOF_ABSOLUTE), // keys are reported as an array of keys which are held down; 0x00 indicates the array entry is not used

	HID_RI_END_COLLECTION(0),
};

static const struct {
    // Note the order of the pieces on the config desc match those of a commercial keyboard
    // and is what is specified by the USB HID spec for a Boot Protocol Keyboard's config descriptor
    USB_Descriptor_Configuration_Header_t config;
    USB_Descriptor_Interface_t            interface0;
    USB_HID_Descriptor_HID_t              hid_keyboard;
    USB_Descriptor_Endpoint_t             endpoint1;
} PROGMEM usb_config_desc = {
	.config = {
			.Header                 = { .Size = sizeof(USB_Descriptor_Configuration_Header_t), .Type = DTYPE_Configuration },

			.TotalConfigurationSize = sizeof(usb_config_desc),
			.TotalInterfaces        = 1,

			.ConfigurationNumber    = 1,
			.ConfigurationStrIndex  = NO_DESCRIPTOR, // we only have one configuration, so no point in naming it

			.ConfigAttributes       = USB_CONFIG_ATTR_RESERVED, // bit 7 must be set for backwards compat with USB 1.0

			.MaxPowerConsumption    = USB_CONFIG_POWER_MA(250) // it's a guess since my V-A meter doesn't measure more than 200mA in DC mode, but the Northgate keyboard uses 195 mA idle, and each 1908's-era green LED ought to use 10 to 20 mA. The ATmega32u4 uses nothing in comparison. Anyhow we don't want to guess too low here since a proper host will cut our power if we draw more than we said we would (not that all hosts do that, but some do and many ought to :-).
		},

	.interface0 = {
			.Header                 = { .Size = sizeof(USB_Descriptor_Interface_t), .Type = DTYPE_Interface },

			.InterfaceNumber        = 0, // interface number 0, our only interface
			.AlternateSetting       = 0,

			.TotalEndpoints         = 1, // 1 IN. commercial keyboards don't have an OUT endpoint. they use the control endpoint for commands, and we will too

			.Class                  = HID_CSCP_HIDClass,
			.SubClass               = HID_CSCP_BootSubclass, // we do implement the simplified Boot protocol
			.Protocol               = HID_CSCP_KeyboardBootProtocol, // and we're a keyboard as far as the Boot protocol is concerned

			.InterfaceStrIndex      = NO_DESCRIPTOR // no name string for this interface
		},

	.hid_keyboard =
		{
			.Header                 = { .Size = sizeof(USB_HID_Descriptor_HID_t), .Type = HID_DTYPE_HID },

			.HIDSpec                = VERSION_BCD(1,1,0), // commercial keyboards report 1.10, so we do too
			.CountryCode            = 33, // commercial keyboards have CC = 0 also, so it seems there is no need to fill this in, but the USB HID spec says 33d is "US", and we are assuming a US layout for the PS/2 keyboard, so this seems right.
			.TotalReportDescriptors = 1,
			.HIDReportType          = HID_DTYPE_Report,
			.HIDReportLength        = sizeof(usb_report_desc)
		},

	.endpoint1 = {
			.Header                 = { .Size = sizeof(USB_Descriptor_Endpoint_t), .Type = DTYPE_Endpoint },
			.EndpointAddress        = ENDPOINT_DIR_IN | 1, // endpoint #1 (#0 is always the control endpoint)
			.Attributes             = EP_TYPE_INTERRUPT | ENDPOINT_USAGE_DATA, // we are a plain interrupt endpoint
			.EndpointSize           = 8, // we send 8-byte reports, like commercial keyboards do
			.PollingIntervalMS      = 1, // have the host poll us rapidly for keystrokes and our device has less keystroke latency. commercial keyboards usually have 10 msec polling intervals, but I think that is too much (plus PS/2 takes ~1msec to transfer a byte, and 1+2 bytes for key down+up, so in theory a fast ps/2 keyboard could send us keystrokes faster than USB would notice. not that that really happens (the ps/2 keyboards aren't running at wire rate and take leisurely pauses when sending))
		},
};

// our usb_manufacturer_str and usb_product_str strings are in english (even though they are also in unicode, so I don't really see the need)
static const USB_Descriptor_String_t PROGMEM usb_language_str = USB_STRING_DESCRIPTOR_ARRAY(LANGUAGE_ID_ENG);

// I'll use the name of a long defunct manufacturer and product just for fun. Hopefully if someone from back then sees this it will make them smile
static const USB_Descriptor_String_t PROGMEM usb_manufacturer_str = USB_STRING_DESCRIPTOR(L"Northgate Computer Systems");
static const USB_Descriptor_String_t PROGMEM usb_product_str = USB_STRING_DESCRIPTOR(L"OmniKey Ultra");

// make up a serial number just for fun. It comes right off the back of my Northgate
static const USB_Descriptor_String_t PROGMEM usb_serial_str = USB_STRING_DESCRIPTOR(L"0621683");

static const USB_Descriptor_String_t* const usb_strings[] = {
    &usb_language_str,
    &usb_manufacturer_str,
    &usb_product_str,
    &usb_serial_str,
};

// might as well have the one function which uses these data structures be in the same file
uint16_t CALLBACK_USB_GetDescriptor(const uint16_t val, const uint8_t idx, const void** const desc) {
    // see which descriptor is wanted
    const void* d = 0;
    uint16_t s = NO_DESCRIPTOR;
    switch (val>>8) {
        case DTYPE_Device:
            d = &usb_device_desc;
            s = sizeof(usb_device_desc);
            break;
        case DTYPE_Configuration:
            d = &usb_config_desc;
            s = sizeof(usb_config_desc);
        case DTYPE_String:
            { // see what string they want
                uint8_t id = val & 0xff;
                if (id < sizeof(usb_strings)/sizeof(usb_strings[0])) {
                    d = usb_strings[id];
                    s = pgm_read_byte(&usb_strings[id]->Header.Size);
                } // else no string for you!
            }
            break;
        case HID_DTYPE_HID:
            d = &usb_config_desc.hid_keyboard;
            s = sizeof(usb_config_desc.hid_keyboard);
            break;
        case HID_DTYPE_Report:
            d = &usb_report_desc;
            s = sizeof(usb_report_desc);
            break;
    }
    *desc = d;
    return s;
}

