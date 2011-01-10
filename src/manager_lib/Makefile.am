SUBDIRS = network

lib_LTLIBRARIES = libftbm.la
libftbm_la_SOURCES = ftb_manager_lib.c
libftbm_la_CFLAGS = -I${top_srcdir}/include -I${top_srcdir}/src/manager_lib/network/include -I${top_srcdir}/src/util
libftbm_la_LIBADD = ../util/libutils.la network/libnetwork.la
