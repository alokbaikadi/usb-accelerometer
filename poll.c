#include "libaccel.h"
#include <stdio.h>
#include <signal.h>
#include <unistd.h>

void trap();
static unsigned int quit = 0;

int main(int argc, char **argv) {

   signal(SIGINT,trap);

   libaccel_init();
   printf("Started accelerometer\n");

   while(!quit) {
      unsigned int x,y,z;
      libaccel_direction(&x,&y,&z);
      printf("Orientation: (%i,%i,%i)\n",x,y,z);
      sleep(1);
   }
   libaccel_exit();
}

void trap() {
   quit = 1;
}
