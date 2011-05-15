#include <stdio.h>
#include <stdlib.h>
#include <libusb-1.0/libusb.h>
#include <unistd.h>
#include <signal.h>
#include "hid-descriptor.h"
#include "names.h"

#define VENDOR_ID	0x0ffe
#define PRODUCT_ID	0x1008

void printdev(libusb_device *dev);
void dump_hid(libusb_device *dev, libusb_device_handle *handle);
void dump_report_desc(unsigned char *b, int l);
void dump_unit(unsigned int data, unsigned int len);
void clean_up(); // signal handler for SIGINT
void test_report(libusb_device_handle *handle);

static unsigned int cleanup = 0; // don't cleanup
static unsigned int test = 0; // don't test

int main(int argc, char **argv) {

   printf("Arguments: %i\n",argc);
   int arg;
   for (arg = 0 ; arg < argc ; arg++)
      printf("  %i: %s;\n", arg,argv[arg]);
   if ((argc > 1) && argv[1][0] == '1') {
      test = 1;
      printf("Set test\n");
   }

   libusb_device **list;
   //libusb_device *accel;
   libusb_context *ctx = NULL;


   libusb_init(&ctx);
   libusb_set_debug(ctx,3);
   names_init("./usb.ids");

   ssize_t cnt = libusb_get_device_list(ctx,&list);

   ssize_t i = 0;
   int err = 0;

   if (cnt < 0) {
      printf("Error getting usb device list\n");
      exit(1);
   }

   for (i = 0 ; i < cnt ; i++) {
      libusb_device *device = list[i];
      struct libusb_device_descriptor desc;
      err = libusb_get_device_descriptor(device,&desc);
      if (err < 0)
         printf("Error getting device descriptor: %i\n",err);
      else {
         if (desc.idVendor == VENDOR_ID && desc.idProduct == PRODUCT_ID) {
            printf("Device Found: idVendor = %xh, idProduct = %xh\n",desc.idVendor, desc.idProduct);
            printdev(device);
            int reattach = 0;
            libusb_device_handle *handle;
            int r = libusb_open(device,&handle);
            if (r < 0){
               printf("Error opening device: %i\n",r);
               exit(1);
            }

            if (libusb_kernel_driver_active(handle,0)) {
               printf("Device already has a kernel driver\n");
               r = libusb_detach_kernel_driver(handle,0);
               if (r < 0) {
                  printf("Unable to detach kernel driver: %i\n",r);
                  goto CLOSE;
               }
               reattach = 1;
            }

            if (libusb_claim_interface(handle,0) == 0) {
               printf("Claimed Device\n");
               dump_hid(device,handle);

               if (test) {
                  printf("Beginning report testing\n");
                  test_report(handle);
               }
            }
CLOSE:

            libusb_release_interface(handle, 0);
            if (reattach) {
               r = libusb_attach_kernel_driver(handle,0);
               if (r < 0) {
                  printf("Failed to reattach kernel driver: %i\n",r);
                  switch (-1) {
                     case LIBUSB_ERROR_IO: 
                        printf("I/O error\n");
                        break;
                     case LIBUSB_ERROR_ACCESS:
                        printf("Access Denied\n");
                        break;
                     case LIBUSB_ERROR_NOT_FOUND:
                        printf("Kernel driver wasn't active\n");
                        break;
                     case LIBUSB_ERROR_INVALID_PARAM:
                        printf("Interface doesn't exist\n");
                        break;
                     case LIBUSB_ERROR_NO_DEVICE:
                        printf("Device was disconnected\n");
                        break;
                     case LIBUSB_ERROR_BUSY:
                        printf("Interface is still claimed\n");
                        break;
                     case LIBUSB_ERROR_TIMEOUT:
                        printf("Operation timed out\n");
                        break;
                     case LIBUSB_ERROR_OVERFLOW:
                        printf("Overflow\n");
                        break;
                     case LIBUSB_ERROR_PIPE:
                        printf("Pipe Error\n");
                        break;
                     case LIBUSB_ERROR_INTERRUPTED:
                        printf("System call interrupted\n");
                        break;
                     case LIBUSB_ERROR_NO_MEM:
                        printf("Insufficient Memory\n");
                        break;
                     case LIBUSB_ERROR_NOT_SUPPORTED:
                        printf("Operation not supported\n");
                        break;
                     default:
                        printf("Some other error occured\n");
                        break;
                  }
               }
            }
            libusb_close(handle);
         }
      }
   }
   libusb_free_device_list(list,1);
   libusb_exit(ctx);
   exit(err);
}

void printdev(libusb_device *dev) {
   struct libusb_device_descriptor desc;
   int r = libusb_get_device_descriptor(dev,&desc);
   int i;
   int j;
   int k;
   if (r< 0) {
      printf("Failed to get device descriptor: %i\n",r);
      return;
   }

   printf("Number of configurations: %i\n",(int)desc.bNumConfigurations);
   printf("Device Class: %i\n",(int)desc.bDeviceClass);
   struct libusb_config_descriptor *config;
   libusb_get_config_descriptor(dev,0,&config);
   printf("Interfaces: %i\n",(int)config->bNumInterfaces);
   const struct libusb_interface *inter;
   const struct libusb_interface_descriptor *interdesc;
   const struct libusb_endpoint_descriptor *epdesc;
   for (i = 0 ; i < config->bNumInterfaces ; i++) {
      inter = &config->interface[i];
      printf("  Number of alternate settings: %i\n",inter->num_altsetting);
      for (j = 0; j < inter->num_altsetting;j++) {
         interdesc = &inter->altsetting[j];
         printf("   Interface Number: %i\n",(int)interdesc->bInterfaceNumber);
         printf("   Interface Class: %i\n",(int)interdesc->bInterfaceClass);
         printf("   Number of endpoints: %i\n",(int)interdesc->bNumEndpoints);
         for (k = 0 ; k < interdesc->bNumEndpoints ; k++) {
            epdesc = &interdesc->endpoint[k];
            printf("    Descriptor Type: %x\n",(int)epdesc->bDescriptorType);
            printf("    EP Address: %x\n",(int)epdesc->bEndpointAddress);
         }
      }
   }
   libusb_free_config_descriptor(config);
}

void dump_hid(libusb_device *dev, libusb_device_handle *handle) {
   hid_device_descriptor *hiddesc = NULL;
   struct libusb_config_descriptor *config;
   libusb_get_config_descriptor(dev,0,&config);
   const struct libusb_interface *inter;
   const struct libusb_interface_descriptor *interdesc;
   int i,j,k,n, r;
   unsigned char dbuf[8190];

   for (i = 0 ; i < config->bNumInterfaces ; i++) {
      inter = &config->interface[i];
      for (j = 0; j < inter->num_altsetting;j++) {
         interdesc = &inter->altsetting[j];
         printf("Interface Number: %i\n",(int)interdesc->bInterfaceNumber);
         r = get_hid_descriptor(handle,interdesc,&hiddesc);
         if (r < 0) {
            printf("Error getting HID device descriptor: %i\n",r);
            goto CLEANUP;
         }
         printf("HID Descriptor\n"
               " bLength:         %5u\n"
               " bDescriptorType: %u\n"
               " bcdHID:          %2x.%02x\n"
               " bCountryCode:    %5u\n"
               " bNumDescriptors: %5u\n",
               hiddesc->bLength, hiddesc->bDescriptorType,
               hiddesc->bcdHID[0], hiddesc->bcdHID[1],
               hiddesc->bCountryCode, hiddesc->bNumDescriptors);
         for (k = 0 ; k < hiddesc->bNumDescriptors ; k++) {
            printf("  bDescriptorType:   %5u\n"
                  "  wDescriptorLength: %5u\n",
                  hiddesc->reports[k].bDescriptorType,
                  hiddesc->reports[k].wDescriptorLength);
            if (hiddesc->reports[k].bDescriptorType != LIBUSB_DT_REPORT)
               continue;
            int len = hiddesc->reports[k].wDescriptorLength;

            int retries = 4;
            n = 0;
            while (n < len && retries--) 
               n = libusb_control_transfer(handle,
                     LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_STANDARD
                     | LIBUSB_RECIPIENT_INTERFACE,
                     LIBUSB_REQUEST_GET_DESCRIPTOR,
                     (LIBUSB_DT_REPORT << 8),
                     interdesc->bInterfaceNumber,
                     dbuf,len, 5*1000);
            if (n > 0) {
               if (n < len)
                  printf(" Warning: incomplete report descriptor\n");
               dump_report_desc(dbuf,len);
            }
         }
      }
   }

CLEANUP:
   if (hiddesc) {
      free_hid_descriptor(hiddesc);
   }
}

void dump_report_desc(unsigned char *b, int l) {
   unsigned int t,j, bsize, btag, btype, data = 0xffff, hut = 0xffff;
   int i;
   char *types[4] = { "Main", "Global", "Local", "reserved" };

   printf("Report Descriptor: (length is %d)\n",l);
   for (i = 0 ; i < l ; ) { 
      t = b[i];
      bsize = b[i] & 0x03;
      if (bsize == 3)
         bsize = 4;
      btype = b[i] & (0x03 << 2);
      btag = b[i] & ~0x03; // 2 LSB bits encode length
      printf(" Item(%-6s): %s, data=", types[btype>>2],
            names_reporttag(btag));
      if (bsize > 0) {
         printf(" [ ");
         data = 0;
         for (j = 0 ; j < bsize ; j++) {
            printf("0x%02x ", b[i+j+1]);
            data += (b[i+j+1] << (8*j));
         }
         printf("] %d", data);
      } else
         printf("none");
      printf("\n");
      switch (btag) {
         case 0x04: // Usage Page
            printf("  %s\n", names_huts(data));
            hut = data;
            break;
         case 0x08: // Usage
         case 0x18: // Usage Min
         case 0x28: // Usage Max
            printf("  %s\n", names_hutus((hut << 16) + data));
            break;
         case 0x54: // Unit Exponent
            printf("  Unit Exponent: %i\n", (signed char)data);
            break;
         case 0x64: // Unit
            dump_unit(data, bsize);
            break;
         case 0xa0: // Collection)
            printf("  ");
            switch (data) {
               case 0x00:
                  printf("Physical\n");
                  break;
               case 0x01:
                  printf("Application\n");
                  break;
               case 0x02:
                  printf("Logical\n");
                  break;
               case 0x03:
                  printf("Report\n");
                  break;
               case 0x04:
                  printf("Named Array\n");
                  break;
               case 0x05:
                  printf("Usage Switch\n");
                  break;
               case 0x06:
                  printf("Usage Modifier\n");
                  break;
               default:
                  if (data & 0x80) 
                     printf("Vendor Defined\n");
                  else
                     printf("Reserved for future use\n");
            }
            break;
         case 0x80: // Input
         case 0x90: // Output
         case 0xb0: // Feature
            printf("  %s %s %s %s %s\n  %s %s %s %s\n",
                  data & 0x01 ? "Constant" : "Data",
                  data & 0x02 ? "Variable" : "Array",
                  data & 0x04 ? "Relative" : "Absolute",
                  data & 0x08 ? "Wrap" : "No Wrap",
                  data & 0x10 ? "Non Linear" : "Linear",
                  data & 0x20 ? "No Preferred State" : "Preferred State",
                  data & 0x40 ? "Null State" : "No Null Position",
                  data & 0x80 ? "Volatile" : "Non Volatile",
                  data & 0x100 ? "Buffered Bytes" : "Bitfield");
            break;
      }
      i += 1 + bsize;
   }
}


void dump_unit(unsigned int data, unsigned int len)
{
   char *systems[5] = { "None", "SI Linear", "SI Rotation",
      "English Linear", "English Rotation" };

   char *units[5][8] = {
      { "None", "None", "None", "None", "None",
         "None", "None", "None" },
      { "None", "Centimeter", "Gram", "Seconds", "Kelvin",
         "Ampere", "Candela", "None" },
      { "None", "Radians",    "Gram", "Seconds", "Kelvin",
         "Ampere", "Candela", "None" },
      { "None", "Inch",       "Slug", "Seconds", "Fahrenheit",
         "Ampere", "Candela", "None" },
      { "None", "Degrees",    "Slug", "Seconds", "Fahrenheit",
         "Ampere", "Candela", "None" },
   };

   unsigned int i;
   unsigned int sys;
   int earlier_unit = 0;

   /* First nibble tells us which system we're in. */
   sys = data & 0xf;
   data >>= 4;

   if (sys > 4) {
      if (sys == 0xf)
         printf("System: Vendor defined, Unit: (unknown)\n");
      else
         printf("System: Reserved, Unit: (unknown)\n");
      return;
   } else {
      printf("System: %s, Unit: ", systems[sys]);
   }
   for (i = 1 ; i < len * 2 ; i++) {
      char nibble = data & 0xf;
      data >>= 4;
      if (nibble != 0) {
         if (earlier_unit++ > 0)
            printf("*");
         printf("%s", units[sys][i]);
         if (nibble != 1) {
            /* This is a _signed_ nibble(!) */

            int val = nibble & 0x7;
            if (nibble & 0x08)
               val = -((0x7 & ~val) + 1);
            printf("^%d", val);
         }
      }
   }
   if (earlier_unit == 0)
      printf("(None)");
   printf("\n");
}

void clean_up() {
   cleanup = 1;
   printf("Loop handled ; Cleaning up\n");
}

#define REPORT_ID(byte) byte
// byte1 1-8, byte2 1-2
#define DIRECTION_X(byte1,byte2) ((byte1) | (((byte2) & 0x03) >> 8))
// byte1 3-8, byte2 1-4
#define DIRECTION_Y(byte1,byte2) (((byte1) & 0xfc) | (((byte2) & 0x07) >> 8))
// byte1 5-8, byte2 1-6
#define DIRECTION_Z(byte1,byte2) (((byte1) & 0xf8) | (((byte2) & 0x3f) >> 8))
#define BUTTON_1(byte) ((byte) & ~0x40)
#define BUTTON_2(byte) ((byte) & ~0x80)

#define REPORT_REQUEST_TYPE 0xa1
#define GET_REPORT 0x01
#define REPORT_TYPE_ID(id) ((id) | 0x01 >> 8)
#define EP_ADDRESS 0x81
#define INTERFACE 0

void test_report(libusb_device_handle *handle) {
   signal(SIGINT,clean_up);// handle sigint (^C) differently
   printf("\n\nTesting Reports, press CTRL-C to stop\n");
   while (!cleanup) {
      // STUB to check error handling
      unsigned int x=0,y=0,z=0;
      unsigned char report[10]; // 5 should be sufficient, testing anyway
      int err = 0;
      int r;
      /* Report Format:
       *  Length: 40 bits
       *  8 bits - Report ID (0) // may not be present
       *  10 bits - Direction-X
       *  10 bits - Direction-Y
       *  10 bits - Direction-Z
       *   2 bits - Buttons
       */
//      err = libusb_control_transfer(handle, //handle
//            REPORT_REQUEST_TYPE, // bmRequestType
//            GET_REPORT, // bRequest
//            REPORT_TYPE_ID(0x00), // wValue
//            INTERFACE, // interface
//            report,// data
//            5,// wLength
//            5*1000); // infinite timeout
      r = libusb_interrupt_transfer(handle, // dev_handle
               EP_ADDRESS, // endpoint
               report, // data
               10, // length
               &err, // transferred
               5*1000);// timeout

      if (r == 0 && err == 4) {
         printf("Successfully retrieved report\n");
         x = DIRECTION_X(report[0],report[1]);
         y = DIRECTION_Y(report[1],report[2]);
         z = DIRECTION_Z(report[2],report[3]);
      } else {
         printf("Only transferred %d bytes\n", err);
      }

      printf("Device is located at: (%d,%d,%d)\n",x,y,z);
      sleep(1);// wait a second before trying again
   }
   printf("Loop exited\n");
}
