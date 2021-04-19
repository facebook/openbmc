all: psb-util

psb-util: psb-util.o 
	$(CC) $(CFLAGS) -std=c99 -o $@ $^ $(LDFLAGS)

.PHONY: clean

clean:
	rm -rf *.o psb-util