sbin_PROGRAMS += ftb_bootstrap_server
ftb_bootstrap_server_SOURCES = \
	$(top_srcdir)/src/manager_lib/network/network_sock/bootstrap/ftb_bootstrap_sock_server.c 
ftb_bootstrap_server_CFLAGS =  -I${top_srcdir}/src/manager_lib/network/include \
	-I${top_srcdir}/src/util -I${top_srcdir}/src/manager_lib/network/network_sock/
ftb_bootstrap_server_LDADD = libutils.la
