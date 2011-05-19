#include <stdio.h>
#include <stdlib.h>
#include <libusb.h>
#include <unistd.h>

#define VENDOR_ID	0x0ffe
#define PRODUCT_ID	0x1008

void libaccel_init();
void libaccel_exit();
void libaccel_direction_x(unsigned int *x);
void libaccel_direction_y(unsigned int *y);
void libaccel_direction_z(unsigned int *z);
void libaccel_direction(unsigned int *x, unsigned int *y, unsigned int *z);
