CC = g++

CC_DEBUG = $(CC) -g
CC_RELEASE = $(CC) -O3 -DNDEBUG

# need libpng to build
#
PNG_INC = -I/opt/local/include -L/opt/local/lib

all: test bench image xapp

test : test.cpp GContext2.cpp
	$(CC_DEBUG) test.cpp GContext2.cpp -o test

bench : bench.cpp GTime.cpp GContext2.cpp
	$(CC_RELEASE) bench.cpp GTime.cpp GContext2.cpp -o bench

image : image.cpp GBitmap.cpp GContext2.cpp
	$(CC_DEBUG) $(PNG_INC) image.cpp GBitmap.cpp GContext2.cpp -lpng -o image

# needs xwindows to build
#
X_INC = -I/usr/X11R6/include -I/usr/X11R6/include/X11 -L/usr/X11R6/lib -L/usr/X11R6/lib/X11 -lX11

xapp: xapp.cpp GContext2.cpp GXWindow.cpp GXWindow.h GBitmap.cpp GTime.cpp
	$(CC_RELEASE) $(X_INC) xapp.cpp GContext2.cpp GXWindow.cpp GBitmap.cpp GTime.cpp -lpng -lX11 -o xapp


clean:
	rm -rf test bench image xapp *.dSYM

