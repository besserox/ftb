sbin_PROGRAMS += ftb_database_server
ftb_database_server_SOURCES = \
	$(top_srcdir)/src/manager_lib/network/network_sock/ftb_database_server_udp.c \
	$(top_srcdir)/src/manager_lib/network/network_sock/ftb_network_sock.h
ftb_database_server_CFLAGS =  -I${top_srcdir}/src/manager_lib/network/include \
	-I${top_srcdir}/src/util -I${top_srcdir}/src/manager_lib/network/network_sock
ftb_database_server_LDADD = libutils.la

libnetwork_la_SOURCES = $(top_srcdir)/src/manager_lib/network/network_sock/ftb_network_tcp.c \
	$(top_srcdir)/src/manager_lib/network/network_sock/ftb_bootstrap_udp.c \
	$(top_srcdir)/src/manager_lib/network/network_sock/ftb_network_sock.h
libnetwork_la_CFLAGS = -I${top_srcdir}/src/manager_lib/network/include \
	-I${top_srcdir}/src/util -I${top_srcdir}/src/manager_lib/network/network_sock
