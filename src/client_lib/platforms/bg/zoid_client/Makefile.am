SUBDIRS = implementation server client

lib_LTLIBRARIES = libftb_zoid_client_preload.la
${SCANNER} ftb_zoid_client_bgp.h
libftb_zoid_client_preload_la_SOURCES =
libftb_zoid_client_preload_la_CFLAGS =  -Wall -g -O2 -fPIC -mpowerpc -pthread 
libftb_zoid_client_preload_la_LDFLAGS = -shared
libftb_zoid_client_preload_la_LD_ADD = server/*.o ftb_zoid_client_bgp_dispatch.la

noinst_LTLIBRARIES = libftb_zoid_client_bgp_dispatch.la libftb_zoid_client_blrts.la
libftb_zoid_client_bgp_dispatch_la_SOURCES = ftb_zoid_client_bgp_dispatch.c
libftb_zoid_client_bgp_dispatch_la_CFLAGS = -Wall -g -O2 -fPIC -mpowerpc -pthread 
libftb_zoid_client_bgp_dispatch_la_LDFLAGS = -I${ZOID_HOME}
libftb_zoid_client_blrts_la_SOURCES = 
libftb_zoid_client_blrts_la_LD_ADD = client/*.o
