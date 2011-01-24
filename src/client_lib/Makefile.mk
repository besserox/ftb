lib_LTLIBRARIES += libftb.la

libftb_la_SOURCES = $(top_srcdir)/src/client_lib/ftb_client_lib.c \
	$(top_srcdir)/src/client_lib/ftb_client_lib.h
libftb_la_CFLAGS = -I${top_srcdir}/src/util -I$(top_srcdir)/src/client_lib
libftb_la_LIBADD = libutils.la libftbm.la
libftb_la_LDFLAGS = -version-info ${libftb_so_version}

include src/client_lib/platforms/Makefile.mk

if PLATFORM_IS_BGP
endif
