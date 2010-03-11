SUBDIRS = zoid_client

lib_LTLIBRARIES = libftb_blrts.la
libftb_blrts_la_SOURCES = ftb_zoid_wrapper.c 
libftb_blrts_la_CFLAGS = -Wall -g -I${abs_srcdir}/include -I${abs_srcdir}/src/include -I{${abs_srcdir}/src/client_lib/platforms/bg/zoid_client} -Dbgp -I${ZOID_HOME}
libftb_blrts_la_LIBADD = zoid_client/ftb_zoid_client_blrts.la
