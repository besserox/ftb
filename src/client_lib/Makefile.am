SUBDIRS = platforms
lib_LTLIBRARIES = libftb.la
libftb_la_SOURCES = ftb_client_lib.c ftb_client_lib.h
libftb_la_CFLAGS = -I${top_srcdir}/include -I${top_srcdir}/src/util
libftb_la_LIBADD = platforms/linux/libftb_linux.la
