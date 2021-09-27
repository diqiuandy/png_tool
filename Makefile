# Makefile, ECE252  
# Yiqing Huang

CC = gcc       # compiler
CFLAGS = -Wall -g -std=gnu99 # compilation flags
LD = gcc       # linker
LDFLAGS = -g   # debugging symbols in build
LDLIBS = -lz   # link with libz

# For students 
LIB_UTIL = zutil.o crc.o lab_png.o
SRCS   = pnginfo.c findpng.c catpng.c

TARGETS= pnginfo findpng catpng

all: ${TARGETS}

pnginfo: pnginfo.o $(LIB_UTIL) 
	$(LD) -o $@ $^ $(LDLIBS) $(LDFLAGS) 

findpng: findpng.o $(LIB_UTIL) 
	$(LD) -o $@ $^ $(LDLIBS) $(LDFLAGS) 

catpng: catpng.o $(LIB_UTIL) 
	$(LD) -o $@ $^ $(LDLIBS) $(LDFLAGS) 

%.o: %.c 
	$(CC) $(CFLAGS) -c $< 

%.d: %.c
	gcc -MM -MF $@ $<

-include $(SRCS:.c=.d)

.PHONY: clean
clean:
	rm -f *.d *.o $(TARGETS) 
