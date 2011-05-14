#include "hid-descriptor.h"

int get_hid_descriptor(const struct libusb_device_handle *handle,const struct libusb_interface_descriptor *interface, struct _hid_device_descriptor **descriptor) {
   if (!handle || !interface || !descriptor)
      return -1; // null arguments

   const unsigned char *buf = interface->extra;
   //unsigned char dbuf[8192]; // for reports?
   int i;

   if (buf[1] != LIBUSB_DT_HID)
      return -2;// not an HID descriptor
   else if (buf[0] < 6 + 3*buf[5]) 
      return -3; // too short
   *descriptor = (struct _hid_device_descriptor *)malloc(sizeof(struct _hid_device_descriptor));
   (*descriptor)->bLength = buf[0];
   (*descriptor)->bDescriptorType = buf[1];
   (*descriptor)->bcdHID[0] = buf[3];
   (*descriptor)->bcdHID[1] = buf[2];
   (*descriptor)->bCountryCode = buf[4];
   (*descriptor)->bNumDescriptors = buf[5];
   (*descriptor)->reports = malloc(buf[5] * sizeof(hid_report_descriptor));
   for (i = 0 ; i < buf[5] ; i++) {
      (*descriptor)->reports[i].bDescriptorType = buf[6+3*i];
      (*descriptor)->reports[i].wDescriptorLength = buf[7+3*i] | buf[8+3*i] << 8;
   }

   return 0;
}

int free_hid_descriptor(struct _hid_device_descriptor *descriptor) {
   if (!descriptor)
      return 0;

   free(descriptor->reports);
   free(descriptor);
   return 0;
}
