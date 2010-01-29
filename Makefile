#*******************************************************************************
#  This file is part of FTB (Fault Tolerance Backplance) - the core of CIFTS
#  (Co-ordinated Infrastructure for Fault Tolerant Systems)
#  
#  See http://www.mcs.anl.gov/research/cifts for more information.
#  	
#  This software is licensed under BSD. See the file FTB/misc/license.BSD for
#  complete details on your rights to copy, modify, and use this software.
#*******************************************************************************

#Please ignore blank spaces (if any) in the start of this file







sbindir=${exec_prefix}/sbin
libdir=${exec_prefix}/lib
abs_top_builddir=/home/rgupta/work/cifts/ftb/trunk
abs_top_srcdir=/home/rgupta/work/cifts/ftb/trunk
exec_prefix=${prefix}
prefix=/usr/local
clients=linux
PLATFORM=
network=sock
FTB_NETWORK_CFLAG=-DFTB_NETWORK_SOCK
ZOID_CLIENT_DIR=
TEMPSBIN=${abs_top_srcdir}/sbin
TEMPLIB=${abs_top_srcdir}/lib
FTB_LIB_BG_CN_NAME=
FTB_LIB_LINUX_NAME=ftb
#FTB_LIB_PATH_SHARED=${TEMPLIB}
#FTB_LIB_PATH_STATIC=${TEMPLIB}
INC_DIR=${abs_top_srcdir}/include
AR=ar
CFLAGS_FTB=-Wall -g -fPIC -I${INC_DIR}
CFLAGS_SRC=${CFLAGS_FTB} -I${abs_top_srcdir}/src/include -I${abs_top_srcdir}/src/manager_lib/include
UTIL_OBJ=${abs_top_srcdir}/src/util/ftb_util.o
#MANAGER_LIB_PATH_SHARED=${abs_top_srcdir}/src/manager_lib/lib/shared
#MANAGER_LIB_PATH_STATIC=${abs_top_srcdir}/src/manager_lib/lib/static
MANAGER_LIB_NAME=ftbm
CFLAGS_CLIENT=${CFLAGS_SRC} -I${abs_top_srcdir}/src/client_lib/include
CLIENT_LIB_OBJ=${abs_top_srcdir}/src/client_lib/ftb_client_lib.o
CFLAGS_MANAGER=${CFLAGS_SRC} -I${abs_top_srcdir}/src/manager_lib/network/include -I${abs_top_srcdir}/src/manager_lib/network/network_${network}/include ${FTB_NETWORK_CFLAG}  -I${abs_top_srcdir}/src/manager_lib/include
MAKE=make
CC=gcc
RANLIB=ranlib
LIBS_PTHREAD=-lpthread

SUBDIRS = src

all: ftb

clean:
	for i in ${SUBDIRS}; do ${MAKE} -C $$i $@; done
	rm -f .event_def .config_def
	rm -rf ${abs_top_srcdir}/sbin;
	rm -rf ${abs_top_srcdir}/lib;

ftb: 
	if [ ! -e ${abs_top_srcdir}/sbin ]; then mkdir ${abs_top_srcdir}/sbin; fi				
	if [ ! -e ${abs_top_srcdir}/lib ]; then mkdir ${abs_top_srcdir}/lib; fi
	${MAKE} -C src all

ftb_examples:
	${MAKE} -C components/examples all

install:
	install -v -b -d /usr/local; 
	install -v -b -d ${exec_prefix}/sbin;
	install -v -b -d ${prefix}/include;
	install -v -b -d ${exec_prefix}/lib; 
	install -v -b ${abs_top_srcdir}/sbin/* ${exec_prefix}/sbin;
	install -v -b ${abs_top_srcdir}/lib/* ${exec_prefix}/lib/;
	install -v -b /home/rgupta/work/cifts/ftb/trunk/include/* ${prefix}/include;
#	install -v -b /home/rgupta/work/cifts/ftb/trunk/components/examples/*ftb* /usr/local/components/examples;
