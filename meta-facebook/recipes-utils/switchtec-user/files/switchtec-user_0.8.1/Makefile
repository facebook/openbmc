########################################################################
##
## Microsemi Switchtec(tm) PCIe Management Library
## Copyright (c) 2017, Microsemi Corporation
##
## Permission is hereby granted, free of charge, to any person obtaining a
## copy of this software and associated documentation files (the "Software"),
## to deal in the Software without restriction, including without limitation
## the rights to use, copy, modify, merge, publish, distribute, sublicense,
## and/or sell copies of the Software, and to permit persons to whom the
## Software is furnished to do so, subject to the following conditions:
##
## The above copyright notice and this permission notice shall be included
## in all copies or substantial portions of the Software.
##
## THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
## OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
## FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
## THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
## OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
## ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
## OTHER DEALINGS IN THE SOFTWARE.
##
########################################################################

OBJDIR=build

DESTDIR ?=
PREFIX ?= /usr/local

BINDIR ?= $(DESTDIR)$(PREFIX)/bin
LIBDIR ?= $(DESTDIR)$(PREFIX)/lib
SYSCONFDIR ?= $(DESTDIR)/etc

CPPFLAGS=-Iinc -Ibuild -DCOMPLETE_ENV=\"SWITCHTEC_COMPLETE\"
CFLAGS=-g -O2 -fPIC -Wall
DEPFLAGS= -MT $@ -MMD -MP -MF $(OBJDIR)/$*.d
LDLIBS=-lcurses -ltinfo

LIB_SRCS=$(wildcard lib/*.c)
CLI_SRCS=$(wildcard cli/*.c)

LIB_OBJS=$(addprefix $(OBJDIR)/, $(patsubst %.c,%.o, $(LIB_SRCS)))
CLI_OBJS=$(addprefix $(OBJDIR)/, $(patsubst %.c,%.o, $(CLI_SRCS)))


ifneq ($(V), 1)
Q=@
else
NQ=:
endif

ifeq ($(W), 1)
CFLAGS += -Werror
endif

compile: libswitchtec.a libswitchtec.so switchtec

clean:
	$(Q)rm -rf libswitchtec.a libswitchtec.so switchtec build

$(OBJDIR)/version.h $(OBJDIR)/version.mk: FORCE $(OBJDIR)
	@$(SHELL_PATH) ./VERSION-GEN
$(OBJDIR)/cli/main.o: $(OBJDIR)/version.h
-include $(OBJDIR)/version.mk

$(OBJDIR):
	$(Q)mkdir -p $(OBJDIR)/cli $(OBJDIR)/lib

$(OBJDIR)/%.o: %.c | $(OBJDIR)
	@$(NQ) echo "  CC    $<"
	$(Q)$(COMPILE.c) $(DEPFLAGS) $< -o $@

libswitchtec.a: $(LIB_OBJS)
	@$(NQ) echo "  AR    $@"
	$(Q)$(AR) rDsc $@ $^

libswitchtec.so: $(LIB_OBJS)
	@$(NQ) echo "  LD    $@"
	$(Q)$(LINK.o) -shared $^ -o $@

switchtec: $(CLI_OBJS) libswitchtec.a
	@$(NQ) echo "  LD    $@"
	$(Q)$(LINK.o) $^ $(LDLIBS) -o $@

install-bash-completion:
	@$(NQ) echo "  INSTALL  $(SYSCONFDIR)/bash_completion.d/bash-switchtec-completion.sh"
	$(Q)install -d $(SYSCONFDIR)/bash_completion.d
	$(Q)install -m 644 -T ./completions/bash-switchtec-completion.sh \
		$(SYSCONFDIR)/bash_completion.d/switchtec

install-bin: compile
	$(Q)install -d $(BINDIR) $(LIBDIR)

	@$(NQ) echo "  INSTALL  $(BINDIR)/switchtec"
	$(Q)install -s switchtec $(BINDIR)
	@$(NQ) echo "  INSTALL  $(LIBDIR)/libswitchtec.a"
	$(Q)install -m 0664 libswitchtec.a $(LIBDIR)
	@$(NQ) echo "  INSTALL  $(LIBDIR)/libswitchtec.so.$(VERSION)"
	$(Q)install libswitchtec.so $(LIBDIR)/libswitchtec.so.$(VERSION)
	@$(NQ) echo "  INSTALL  $(LIBDIR)/libswitchtec.so"
	$(Q)ln -fs $(LIBDIR)/libswitchtec.so.$(VERSION) \
           $(LIBDIR)/libswitchtec.so

	@$(NQ) echo "  LDCONFIG"
	$(Q)ldconfig

install: install-bin install-bash-completion

uninstall:
	@$(NQ) echo "  UNINSTALL  $(BINDIR)/switchtec"
	$(Q)rm -f $(BINDIR)/switchtec
	@$(NQ) echo "  UNINSTALL  $(LIBDIR)/libswitchtec.a"
	$(Q)rm -f $(LIBDIR)/libswitchtec.a
	@$(NQ) echo "  UNINSTALL  $(LIBDIR)/libswitchtec.so"
	$(Q)rm -f $(LIBDIR)/libswitchtec.so*
	@$(NQ) echo "  LDCONFIG"
	$(Q)ldconfig

PKG=switchtec-$(FULL_VERSION)
dist:
	git archive --format=tar --prefix=$(PKG)/ HEAD > $(PKG).tar
	@echo $(FULL_VERSION) > version
	tar -rf $(PKG).tar --xform="s%^%$(PKG)/%" version
	xz -f $(PKG).tar
	rm -f version

.PHONY: clean compile install unintsall install-bin install-bash-completion
.PHONY: FORCE


-include $(patsubst %.o,%.d,$(LIB_OBJS))
