#include <stdio.h>
#include <stdlib.h>
#include <libusb-1.0/libusb.h>
#include <unistd.h>

#define VENDOR_ID	0x0ffe
#define PRODUCT_ID	0x1008

void printdev(libusb_device *dev);

int main(int argc, char **argv) {

   libusb_device **list;
   libusb_device *accel;
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
         printf("Device Found: idVendor = %xh, idProduct = %xh\n",desc.idVendor, desc.idProduct);
         if (desc.idVendor == VENDOR_ID && desc.idProduct == PRODUCT_ID) {
            printdev(device);
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

   libusb_device_handle *handle;
   r = libusb_open(dev,&handle);
   if (r < 0){
      printf("Error opening device: %i\n",r);
      return;
   }

   if (libusb_kernel_driver_active(handle,0)) {
      printf("Device already has a kernel driver");
      r = libusb_detach_kernel_driver(handle,0);
      if (r < 0) {
         printf("Unable to detach kernel driver: %i\n",r);
         goto ERROR;
      }
      sleep(5);
      r = libusb_attach_kernel_driver(handle,0);
      if (r < 0) {
         printf("Failed to reattach kernel driver: %i\n",r);
         goto ERROR;
      }

ERROR:
      libusb_close(handle);
   }

}
