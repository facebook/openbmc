
MAINTAINERCLEANFILES	= Makefile.in

AM_CPPFLAGS		= -I$(top_srcdir)/include

EXTRA_LTLIBRARIES	= libintf_obmc.la
noinst_LTLIBRARIES	= @INTF_OBMC_LIB@
libintf_obmc_la_LIBADD	= $(top_builddir)/lib/libipmitool.la
libintf_obmc_la_SOURCES	= obmc.c
