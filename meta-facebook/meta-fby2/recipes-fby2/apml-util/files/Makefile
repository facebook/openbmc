all: apml-util

apml-util: apml-util.o 
	$(CC) $(CFLAGS) -lbic -lfby2_common -std=c99 -o $@ $^ $(LDFLAGS)

.PHONY: clean

clean:
	rm -rf *.o apml-util