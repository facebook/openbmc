all: i2craw

i2craw:  i2craw.o
	$(CC) -o $@ $^ $(LDFLAGS)

.PHONY: clean

clean:
	rm -rf *.o i2craw
