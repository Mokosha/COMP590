CC = g++ -O3 -g

all: test bench

test : test.cpp GContext0.cpp
	$(CC) test.cpp GContext0.cpp -o test

bench : bench.cpp GTime.cpp GContext0.cpp
	$(CC) bench.cpp GContext0.cpp GTime.cpp -o bench

clean:
	rm -rf test bench *.dSYM

