ifdef INST
ifeq ("$(origin INST)", "command line")
INSTALL_DIR := $(INST)
endif
endif

CC		= $(CROSS_COMPILE)gcc
INCLUDE		= .
OBJS		= 
LDLIBS		= -lc -lrt -lgcc
EXTRA_FLAGS	= -lpthread
EXEC_NAME	= kcs-test

all:		$(EXEC_NAME)

$(EXEC_NAME):	$(EXEC_NAME).o
	$(CC) $(MYLDFLAGS) $(OBJS) $(EXEC_NAME).o -o $@ $(LDLIBS) $(EXTRA_FLAGS)

$(EXEC_NAME).o:	$(OBJS) $(EXEC_NAME).c
	$(CC) $(MYCFLAGS) -l$(INCLUDE) -c $(EXEC_NAME).c

romfs:
	$(ROMFSINST) /usr/local/bin/$(EXEC_NAME)

clean:
	rm -f $(EXEC_NAME).elf $(EXEC_NAME).gdb $(EXEC_NAME).o $(EXEC_NAME)

install:
	cp $(EXEC_NAME) $(INSTALL_DIR)
