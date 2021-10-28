objects = mprotect_perf.o
bins = mprotect_perf

%.o : %.c
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $< -o $@

$(objects): %.o: %.c

mprotect_perf: mprotect_perf.o

clean:
	rm -f $(objects) mprotect_perf

default: mprotect_perf

benchmark: mprotect_perf
	for number in 1 100 1000 10000 100000 ; do \
		export PAGE_MULTIPLE=$$number; \
		time ./mprotect_perf; \
	done
