SCANNER = $(ZOID_SRC)/scanner.pl
#all: ftb_zoid_client_$(PLATFORM).h 
$(SCANNER) $<

SUBDIRS = client server implementation

noinst_LTLIBRARIES = libftb_zoid_client_blrts.la
libftb_zoid_client_blrts_la_AR = $(AR_BLRTS)
libftb_zoid_client_blrts_la_RANLIB = $(RANLIB_BLRTS)
libftb_zoid_client_blrts_la_LIBADD = client/libclient_bg.la

lib_LTLIBRARIES = libftb_zoid_client_preload.la
libftb_zoid_client_preload_la_SOURCES = ftb_zoid_client_bgp_dispatch.c
libftb_zoid_client_preload_la_CFLAGS = -I$(ZOID_HOME) 
libftb_zoid_client_preload_la_LIBADD = server/libserver_bg.la

