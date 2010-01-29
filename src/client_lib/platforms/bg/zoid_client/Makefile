#***************************************************************************
#  This file is part of FTB (Fault Tolerance Backplance) - the core of CIFTS
#  (Co-ordinated Infrastructure for Fault Tolerant Systems)
#  
#  See http://www.mcs.anl.gov/research/cifts for more information.
#  	
#  This software is licensed under BSD. See the file FTB/misc/license.BSD for
#  complete details on your rights to copy, modify, and use this software.
#***************************************************************************

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

PREFIX = ftb_zoid_client
SCANNER = $(ZOID_SRC)/scanner.pl
CFLAGS = -Wall -g -O2  
CFLAGS_SHARED = -fPIC -mpowerpc -pthread
LDFLAGS_SHARED = -shared

SUBDIRS= client server implementation
all: lib$(PREFIX)_blrts.a $(PREFIX)_preload.so implementation $(FTB_LIB_PATH_SHARED)/$(PREFIX)_preload.so

$(FTB_LIB_PATH_SHARED)/$(PREFIX)_preload.so: $(PREFIX)_preload.so
	cp -p $(PREFIX)_preload.so $@

lib$(PREFIX)_blrts.a: client/.object_files_built
	rm -f $@
	$(AR_BLRTS) -rc $@ client/*.o && $(RANLIB_BLRTS) $@

$(PREFIX)_preload.so: server/.object_files_built $(PREFIX)_$(PLATFORM)_dispatch.o
	$(CC) $(CFLAGS) $(CFLAGS_SHARED) $(LDFLAGS_SHARED) -o $@ \
	/server/*.o $(PREFIX)_$(PLATFORM)_dispatch.o

client/.object_files_built: $(PREFIX)_defs.h
	${MAKE} -C client all

server/.object_files_built: $(PREFIX)_defs.h
	${MAKE} -C server all


$(PREFIX)_$(PLATFORM)_dispatch.o: $(PREFIX)_defs.h
	$(CC) -c $(CFLAGS) -I$(ZOID_HOME) $(CFLAGS_SHARED) $(PREFIX)_$(PLATFORM)_dispatch.c

$(PREFIX)_defs.h: $(PREFIX)_$(PLATFORM).h $(SCANNER)
	$(SCANNER) $<

.PHONY: implementation
implementation:
	$(MAKE) -C implementation

clean:
	for i in ${SUBDIRS}; do ${MAKE} -C $$i $@; done
	rm -f $(FTB_LIB_PATH_SHARED)/$(PREFIX)_preload.so 
	rm -f lib$(PREFIX)_blrts.a $(PREFIX)_preload.so 
	rm -f $(PREFIX)_$(PLATFORM)_defs.h $(PREFIX)_$(PLATFORM)_dispatch.c $(PREFIX)_$(PLATFORM)_dispatch.o *~

distclean: clean
