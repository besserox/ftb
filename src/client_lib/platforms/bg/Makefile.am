SUBDIRS = zoid_client

if PLATFORM_IS_BGP
PLATFORM = BGP
else
PLATFORM = BGL
endif

noinst_LTLIBRARIES = libftb_zoid_wrapper.lo
libftb_zoid_wrapper_lo_SOURCES = ftb_zoid_wrapper.c
libftb_zoid_wrapper_lo_CFLAGS = -I${top_srcdir}/include -I${ZOID_CLIENT_DIR} -D${PLATFORM} -I${ZOID_HOME}

noinst_LTLIBRARIES = libblrts.la

libblrts_la_LIBADD = ./libftb_zoid_wrapper.la zoid_client/libftb_zoid_client.la
