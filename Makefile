SOURCES = $(wildcard *.c)

OBJECTS = $(SOURCES:.c=.o)

EXENAME = accelerometer-info
LIBNAME = libaccelerometer.so.1

DEFINES = -DDEBUG
INCLUDES = -I/usr/include -I/usr/include/libusb-1.0
LIBRARIES =
LINKOPTS = -shared -W1,-soname,$(LIBNAME).0

c = gcc
LDFLAGS = -Wl -O1 --as-needed -lusb-1.0
cFLAGS = -g -Wall -ffast-math

CSCOPE = cscope

all: $(EXENAME) $(LIBNAME)
$(EXENAME): $(OBJECTS)
	$(c) $(cFLAGS) $(LDFLAGS) $(DEFINES) $(INCLUDES) $(OBJECTS) $(LIBRARIES) -o $@

$(LIBNAME): $(OBJECTS)
	$(c) $(cFLAGS) $(LDFLAGS) $(DEFINES) $(INCLUDES) $(LINKOPTS) -o $(@)

tags:
	$(CSCOPE) -b

clean:
	-rm -f *.o
	-rm -f $(EXENAME)
	-rm -f Makefile.dep

Makefile.dep:
	$(c) -M $(cFLAGS) $(INCLUDES) $(SOURCES) > Makefile.dep

.SUFFIXES: .o .c

.c.o:
	$(c) $(cFLAGS) $(DEFINES) $(INCLUDES) -c $<

-include Makefile.dep
