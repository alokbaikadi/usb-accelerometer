#include <stdio.h>
#include <stdlib.h>
#include <libusb-1.0/libusb.h>
#include <unistd.h>
#include "hid-descriptor.h"

#define VENDOR_ID	0x0ffe
#define PRODUCT_ID	0x1008

void printdev(libusb_device *dev);
void dump_hid(libusb_device *dev);

int main(int argc, char **argv) {

   libusb_device **list;
   //libusb_device *accel;
   libusb_context *ctx = NULL;


   libusb_init(&ctx);
   libusb_set_debug(ctx,3);

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
            dump_hid(device);
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
            printf("    Descriptor Type: %i\n",(int)epdesc->bDescriptorType);
            printf("    EP Address: %i\n",(int)epdesc->bEndpointAddress);
         }
      }
   }
   libusb_free_config_descriptor(config);

}

void dump_hid(libusb_device *dev) {
   int reattach = 0;
   libusb_device_handle *handle;
   int r = libusb_open(dev,&handle);
   if (r < 0){
      printf("Error opening device: %i\n",r);
      return;
   }

   if (libusb_kernel_driver_active(handle,0)) {
      printf("Device already has a kernel driver\n");
      r = libusb_detach_kernel_driver(handle,0);
      if (r < 0) {
         printf("Unable to detach kernel driver: %i\n",r);
         goto CLEANUP;
      }
      reattach = 1;
   }

   hid_device_descriptor *hiddesc = NULL;
   struct libusb_config_descriptor *config;
   libusb_get_config_descriptor(dev,0,&config);
   const struct libusb_interface *inter;
   const struct libusb_interface_descriptor *interdesc;
   int i,j,k,n;
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

            if (libusb_claim_interface(handle,interdesc->bInterfaceNumber) == 0) {
               printf("Claimed Device\n");
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
               }
               libusb_release_interface(handle, interdesc->bInterfaceNumber);
            }
         }

         if (reattach) {
            r = libusb_attach_kernel_driver(handle,0);
            if (r < 0) {
               printf("Failed to reattach kernel driver: %i\n",r);
               goto CLEANUP;
            }
         }
      }
   }

CLEANUP:
         if (hiddesc) {
            free_hid_descriptor(hiddesc);
         }
         libusb_close(handle);
      }

