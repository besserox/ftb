#*******************************************************************************
#  This file is part of FTB (Fault Tolerance Backplance) - the core of CIFTS
#  (Co-ordinated Infrastructure for Fault Tolerant Systems)
#  
#  See http://www.mcs.anl.gov/research/cifts for more information.
#  	
#  This software is licensed under BSD. See the file FTB/misc/license.BSD for
#  complete details on your rights to copy, modify, and use this software.
#*******************************************************************************

sbindir=${exec_prefix}/sbin
libdir=${exec_prefix}/lib
abs_top_builddir=/home/rgupta/work/cifts/ftb/trunk
abs_top_srcdir=/home/rgupta/work/cifts/ftb/trunk
exec_prefix=${prefix}
prefix=/usr/local
clients=linux
PLATFORM=
network=sock
TEMPSBIN=${abs_top_srcdir}/sbin
TEMPLIB=${abs_top_srcdir}/lib
FTB_NETWORK_CFLAG=-DFTB_NETWORK_SOCK
ZOID_CLIENT_DIR=
FTB_LIB_BG_CN_NAME=
FTB_LIB_LINUX_NAME=ftb
FTB_LIB_PATH_SHARED=${TEMPLIB}
FTB_LIB_PATH_STATIC=${TEMPLIB}
INC_DIR=${abs_top_srcdir}/include
AR=ar
CFLAGS_FTB=-Wall -g -fPIC -I${INC_DIR}
CFLAGS_SRC=${CFLAGS_FTB} -I${abs_top_srcdir}/src/include -I${abs_top_srcdir}/src/manager_lib/include
UTIL_OBJ=${abs_top_srcdir}/src/util/ftb_util.o
MANAGER_LIB_PATH_SHARED=${abs_top_srcdir}/src/manager_lib/lib/shared
MANAGER_LIB_PATH_STATIC=${abs_top_srcdir}/src/manager_lib/lib/static
MANAGER_LIB_NAME=ftbm
CFLAGS_CLIENT=${CFLAGS_SRC} -I${abs_top_srcdir}/src/client_lib/include
CLIENT_LIB_OBJ=${abs_top_srcdir}/src/client_lib/ftb_client_lib.o
CFLAGS_MANAGER=${CFLAGS_SRC} -I${abs_top_srcdir}/src/manager_lib/network/include -I${abs_top_srcdir}/src/manager_lib/network/network_${network}/include ${FTB_NETWORK_CFLAG}  -I${abs_top_srcdir}/src/manager_lib/include
MAKE=make
CC=gcc
RANLIB=ranlib
LIBS_PTHREAD=-lpthread

all: ftb_network ftb_manager_lib.o libs 

CFLAGS = $(CFLAGS_MANAGER) 

libs: $(MANAGER_LIB_PATH_STATIC)/lib$(MANAGER_LIB_NAME).a $(MANAGER_LIB_PATH_SHARED)/lib$(MANAGER_LIB_NAME).so

$(MANAGER_LIB_PATH_SHARED)/lib$(MANAGER_LIB_NAME).so: ftb_network ftb_manager_lib.o
	$(CC) -shared -o $@ $(UTIL_OBJ) ftb_manager_lib.o ${abs_top_srcdir}/src/manager_lib/network/network_$(network)/*.o

$(MANAGER_LIB_PATH_STATIC)/lib$(MANAGER_LIB_NAME).a: ftb_network ftb_manager_lib.o
	$(AR) -rc $@ $(UTIL_OBJ) ftb_manager_lib.o ${abs_top_srcdir}/src/manager_lib/network/network_$(network)/*.o
	$(RANLIB) $@

clean:
	rm -f $(MANAGER_LIB_PATH_STATIC)/lib$(MANAGER_LIB_NAME).a $(MANAGER_LIB_PATH_SHARED)/lib$(MANAGER_LIB_NAME).so 
	rm -f ftb_manager_lib.o 
#(cd network/network_$(network) && $(MAKE) clean)
	$(MAKE) -C network/network_$(network) clean

ftb_manager_lib.o: ftb_manager_lib.c
	$(CC) $(CFLAGS) -c $<

ftb_network:
#(cd network/network_$(network) && $(MAKE))
	$(MAKE) -C network/network_$(network) all
