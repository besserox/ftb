SUBDIRS = network

lib_LTLIBRARIES = libftbm.la
libftbm_la_SOURCES = ftb_manager_lib.c
libftbm_la_LIBADD = network/network_sock/libnetwork_sock.la
libftbm_la_CFLAGS = -I${top_srcdir}/include -I${top_srcdir}/src/include -I${top_srcdir}/src/manager_lib/network/include -I${top_srcdir}/src/client_lib/include
#libftbm_la_CFLAGS = -I${top_srcdir}/include -I${top_srcdir}/src/include ${top_srcdir}/src/manager_lib/include -I${top_srcdir}/src/manager_lib/network/include -I${top_srcdir}/src/manager_lib/network/network_${NETWORK}/include ${FTB_NETWORK_EXTRA_CFLAGS} -I${top_srcdir}/src/client_lib/include

