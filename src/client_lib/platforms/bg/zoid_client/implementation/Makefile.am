lib_LTLIBRARIES = libftb_zoid_client_impl.la
libftb_zoid_client_impl_la_SOURCES = ftb_zoid_client.c ../../../../ftb_client_lib.c
libftb_zoid_client_impl_la_CFLAGS = -fPIC -mpowerpc -pthread -I${top_srcdir}/include -I${abs_srcdir}/src/include -I${abs_srcdir}/src/manager_lib/include -I${abs_srcdir}/src/client_lib/include
libftb_zoid_client_impl_la_LDFLAGS = -shared -I.. -I${ZOID_HOME}
libftb_zoid_client_impl_la_LIBADD = ../../../../../util/libutils.la ../../../../../manager_lib/libftbm.la
