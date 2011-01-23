CFLAGS_SHARED = -fPIC -mpowerpc -pthread

noinst_LTLIBRARIES = libclient_bg.la
libclient_bg_la_SOURCES = *.c
libclient_bg_la_CFLAGS = $(CC_BLRTS) $(CFLAGS_SHARED) -I${top_srcdir}/src/client_lib/platforms/bg/zoid_client -I${ZOID_HOME} -I${top_srcdir}/include
