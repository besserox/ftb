sbin_PROGRAMS = ftb_database_server
ftb_database_server_SOURCES = ftb_database_server_udp.c
ftb_database_server_CFLAGS =  -I${top_srcdir}/include -I${top_srcdir}/src/manager_lib/network/include -I${top_srcdir}/src/util -I${top_srcdir}/src/manager_lib/network/network_sock
ftb_database_server_LDADD = ../../../util/libutils.la

noinst_LTLIBRARIES = libnetwork_sock.la
libnetwork_sock_la_SOURCES = ftb_network_tcp.c ftb_bootstrap_udp.c
libnetwork_sock_la_CFLAGS = -I${top_srcdir}/include -I${top_srcdir}/src/manager_lib/network/include -I${top_srcdir}/src/util -I${top_srcdir}/src/manager_lib/network/network_sock

