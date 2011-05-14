#include <libusb.h>
#include <stdlib.h>

struct _hid_report_descriptor {
   uint8_t bDescriptorType;
   uint16_t wDescriptorLength;
};

struct _hid_device_descriptor {
   uint8_t bLength;
   uint8_t bDescriptorType;
   uint8_t bcdHID[2];
   uint8_t bCountryCode;
   uint8_t bNumDescriptors;
   struct _hid_report_descriptor *reports;
};

typedef struct _hid_device_descriptor hid_device_descriptor;
typedef struct _hid_report_descriptor hid_report_descriptor;

int get_hid_descriptor(const struct libusb_device_handle *handle, const struct libusb_interface_descriptor *interface, hid_device_descriptor **descriptor);
int free_hid_descriptor(hid_device_descriptor *descriptor);
