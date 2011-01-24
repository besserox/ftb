include src/manager_lib/network/Makefile.mk

lib_LTLIBRARIES += libftbm.la

libftbm_la_SOURCES = ${top_srcdir}/src/manager_lib/ftb_manager_lib.c
libftbm_la_CFLAGS = -I${top_srcdir}/src/manager_lib/network/include -I${top_srcdir}/src/util
libftbm_la_LIBADD = libutils.la libnetwork.la
libftbm_la_LDFLAGS = -version-info ${libftbm_so_version}
