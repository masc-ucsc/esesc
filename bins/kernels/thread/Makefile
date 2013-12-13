
all: ThreadSafeFIFO forktest

ThreadSafeFIFO: ThreadSafeFIFO.cpp
	g++ -static -O2 -o $@ $^ -lpthread

forktest: forktest.c
	gcc -static -O2 -o $@ $^

clean:
		-@rm -f ThreadSafeFIFO forktest

