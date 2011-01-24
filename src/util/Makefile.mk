noinst_LTLIBRARIES += libutils.la
libutils_la_SOURCES = $(top_srcdir)/src/util/ftb_util.c \
	$(top_srcdir)/src/util/ftb_auxil.c \
	$(top_srcdir)/src/util/ftb_util.h \
	$(top_srcdir)/src/util/ftb_auxil.h
libutils_la_CFLAGS = -I${top_srcdir}/src/util
