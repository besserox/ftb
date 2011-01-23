CFLAGS_SHARED = -fPIC -mpowerpc -pthread

noinst_LTLIBRARIES = libserver_bg.la
libserver_bg_la_SOURCES = *.c
libserver_bg_la_CFLAGS = $(CFLAGS_SHARED) -I${top_srcdir}/src/client_lib/platforms/bg/zoid_client -I${ZOID_HOME} -I${top_srcdir}/include 
