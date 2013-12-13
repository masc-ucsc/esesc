
BINS:= memcpy_slow thread_ping stream stream_omp thread_migrate
all:$(BINS)

memcpy_slow: memcpy_slow.c
	gcc -O1 -static $? -o $@ -lpthread

thread_ping: thread_ping.c
	gcc -O1 -static $? -o $@ -lpthread

thread_migrate: thread_migrate.c
	gcc -O1 -static $? -o $@ -lpthread

stream: stream.c
	gcc -O2 -static $? -o $@ -lpthread

stream_omp: stream.c
	gcc -O2 -fopenmp -static $? -o $@ -lpthread

clean:
	@rm -f $(BINS)
