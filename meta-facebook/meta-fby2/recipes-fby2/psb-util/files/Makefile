all: psb-util

psb-util: psb-util.o 
	$(CC) $(CFLAGS) -lpal -lkv -std=c99 -o $@ $^ $(LDFLAGS)

.PHONY: clean

clean:
	rm -rf *.o psb-util