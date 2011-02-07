libnetwork_la_SOURCES = $(top_srcdir)/src/manager_lib/network/network_sock/sock_lib/ftb_network_sock.c \
	$(top_srcdir)/src/manager_lib/network/network_sock/sock_lib/ftb_bootstrap_sock_client.c
libnetwork_la_CFLAGS = -I${top_srcdir}/src/manager_lib/network/include \
	-I${top_srcdir}/src/util -I${top_srcdir}/src/manager_lib/network/network_sock
