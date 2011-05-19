INCLUDES = -I/usr/include -I/usr/include/libusb-1.0
LIBRARIES = -L. -L/usr/lib
LINKOPTS = -shared -W1,-soname,libaccel.so.1.0
CC = gcc
LDFLAGS = -Wl -O1 --as-needed -lusb-1.0
CFLAGS = -Wall -ffast-math

all:  libaccel.so accelerometer-info poll

accelerometer-info: accelerometer-info.o names.o hid-descriptor.o
	$(CC) $(CFLAGS) $(LDFLAGS) $(INCLUDES) $(LIBRARIES) $(LDFLAGS) accelerometer-info.o hid-descriptor.o names.o -o $@

poll: poll.o libaccel.so
	$(CC) $(CFLAGS) $(LDFLAGS) $(INCLUDES) $(LIBRARIES) $(LDFLAGS) -laccel poll.o -o poll

libaccel.so: libaccel.o
	$(CC) $(LINKOPTS) libaccel.o -o libaccel.so.1.0
	ln -s libaccel.so.1.0 libaccel.so.1
	ln -s libaccel.so.1.0 libaccel.so

tags:
	$(CSCOPE) -b

clean:
	-rm -f *.o
	-rm -f $(EXENAME)
	-rm -f Makefile.dep

Makefile.dep:
	$(CC) -M $(cFLAGS) $(INCLUDES) $(SOURCES) > Makefile.dep

.SUFFIXES: .o .c

.c.o:
	$(CC) $(CFLAGS) $(DEFINES) $(INCLUDES) -c $<

-include Makefile.dep
