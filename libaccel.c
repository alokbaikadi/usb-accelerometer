#include "libaccel.h"

// macros for pulling info out of the hid interrupt
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

static libusb_context *ctx = NULL;
static libusb_device *device = NULL;
static libusb_device_handle *handle = NULL;
static unsigned int reattach = 0;
static libusb_device **list = NULL;

void libaccel_init() {
   libusb_init(&ctx);
   libusb_set_debug(ctx,3);

   ssize_t cnt = libusb_get_device_list(ctx,&list);
   ssize_t i = 0;
   int err = 0;
   if (cnt < 0) {
      printf("Error getting usb device list: %i\n",err);
      return;
   }

   for (i = 0 ; i < cnt ; i++) {
      libusb_device *dev = list[i];
      struct libusb_device_descriptor desc;
      err = libusb_get_device_descriptor(dev,&desc);
      if (err < 0)
         printf("Error getting device descriptor: %i\n",err);
      else {
         if (desc.idVendor == VENDOR_ID && desc.idProduct == PRODUCT_ID) {
            printf("Device Found: idVendor = 0x%x, idProduct = 0x%x\n", desc.idVendor, desc.idProduct);
            device = dev;
            err = libusb_open(device, &handle);
            if (err < 0) {
               printf("Error opening device: %i\n",err);
               return;
            }

            if (libusb_kernel_driver_active(handle,INTERFACE)) {
               printf("Device already has a kernel driver\n");
               err = libusb_detach_kernel_driver(handle,INTERFACE);
               if (err < 0) {
                  printf("Unable to detach kernel driver: %i\n",err);
                  libaccel_exit();
                  return;
               }
               reattach = 1;
            }

            err = libusb_claim_interface(handle,INTERFACE);
            if (err <  0) {
               printf("Error claiming the device: %i\n", err);
               libaccel_exit();
               return;
            }
            break; // only care about finding the first such device
         }
      }
   }
}

void libaccel_exit() {
   int err = 0;
   libusb_release_interface(handle, INTERFACE);
   if (reattach) {
      err = libusb_attach_kernel_driver(handle,INTERFACE);
      if (err < 0) {
         printf("Failed to reattach kernel driver: %i\n",err);
      }
   }
   libusb_close(handle);
   libusb_free_device_list(list,1);
}

void libaccel_direction_x(int *x) {
   int y, z;
   libaccel_direction(x,&y,&z);
}

void libaccel_direction_y(int *y) {
   int x, z;
   libaccel_direction(&x,y,&z);
}

void libaccel_direction_z(int *z) {
   int y, x;
   libaccel_direction(&x,&y,z);
}

void libaccel_direction(int *x, int *y, int *z) {
   unsigned char report[5];
   int err = 0;
   int transferred;

   err = libusb_interrupt_transfer(handle, EP_ADDRESS, report, 5, &transferred, 5*1000);
   if (err == 0 && transferred == 4) {
      *x = DIRECTION_X(report[0],report[1]);
      *y = DIRECTION_Y(report[1],report[2]);
      *z = DIRECTION_Z(report[2],report[3]);
   }
}

