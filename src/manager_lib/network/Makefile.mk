noinst_LTLIBRARIES += libnetwork.la

include src/manager_lib/network/include/Makefile.mk

if FTB_NETWORK_TYPE_SOCK
include src/manager_lib/network/network_sock/Makefile.mk
endif
