SRCS := mprotect_perf.c
OBJS := $(SRCS:%.c=%.o)

%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS)

all: $(OBJS) mprotect_perf

mprotect_perf: mprotect_perf.o
	$(CC) -o $@ mprotect_perf.o $(CFLAGS)

default: mprotect_perf

benchmark: mprotect_perf
	for number in 1 100 1000 10000 100000 ; do \
		export PAGE_MULTIPLE=$$number; \
		time ./mprotect_perf; \
	done

clean:
	rm -f $(OBJS) mprotect_perf
