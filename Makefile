SOURCES = $(wildcard *.c)

OBJECTS = $(SOURCES:.c=.o)

EXENAME = accelerometer-info

DEFINES = -DDEBUG
INCLUDES = -I/usr/include -I/usr/include/libusb-1.0
LIBRARIES =

c = gcc
LDFLAGS = -Wl -O1 --as-needed -lusb-1.0
cFLAGS = -g -Wall -ffast-math

CSCOPE = cscope

all: $(EXENAME)
$(EXENAME): $(OBJECTS)
	$(c) $(cFLAGS) $(LDFLAGS) $(DEFINES) $(INCLUDES) $(OBJECTS) $(LIBRARIES) -o $@

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
