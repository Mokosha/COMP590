CC = g++ -O3 -g -DNDEBUG

all: test bench image

test : test.cpp GContext2.cpp
	$(CC) test.cpp GContext2.cpp -o test

bench : bench.cpp GTime.cpp GContext2.cpp
	$(CC) bench.cpp GTime.cpp GContext2.cpp -o bench

image : image.cpp GBitmap.cpp GContext2.cpp
	$(CC) image.cpp GBitmap.cpp GContext2.cpp -lpng -o image

clean:
	rm -rf test bench image *.dSYM

