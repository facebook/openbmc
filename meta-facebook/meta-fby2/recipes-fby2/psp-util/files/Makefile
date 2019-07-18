all: psp-util

psp-util: psp-util.o 
	$(CC) $(CFLAGS) -lbic -std=c99 -o $@ $^ $(LDFLAGS)

.PHONY: clean

clean:
	rm -rf *.o psp-util