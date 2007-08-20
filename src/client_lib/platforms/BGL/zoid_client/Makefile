#***************************************************************************
# ZEPTOOS:zepto-info
#      This file is part of ZeptoOS: The Small Linux for Big Computers.
#      See www.mcs.anl.gov/zeptoos for more information.
# ZEPTOOS:zepto-info
#
# ZEPTOOS:zepto-fillin
#      $Id: Makefile,v 1.7 2007/06/27 15:10:45 iskra Exp $
#      ZeptoOS_Version: 1.2
#      ZeptoOS_Heredity: FOSS_ORIG
#      ZeptoOS_License: GPL
# ZEPTOOS:zepto-fillin
#
# ZEPTOOS:zepto-gpl
#       Copyright: Argonne National Laboratory, Department of Energy,
#                  and UChicago Argonne, LLC.  2004, 2005, 2006, 2007
#       ZeptoOS License: GPL
#  
#       This software is free.  See the file ZeptoOS/misc/license.GPL
#       for complete details on your rights to copy, modify, and use this
#       software.
# ZEPTOOS:zepto-gpl
#***************************************************************************

PREFIX = ftb_zoid_client
SCANNER = $(ZOID_HOME)/scanner.pl
CFLAGS = -Wall -g -O2  
CFLAGS_SHARED = -fPIC -mpowerpc -pthread
LDFLAGS_SHARED = -shared

all: lib$(PREFIX)_blrts.a $(PREFIX)_preload.so implementation $(FTB_LIB_PATH_SHARED)/$(PREFIX)_preload.so

$(FTB_LIB_PATH_SHARED)/$(PREFIX)_preload.so: $(PREFIX)_preload.so
	cp -p $(PREFIX)_preload.so $@

lib$(PREFIX)_blrts.a: client/.object_files_built
	rm -f $@
	$(AR_BLRTS) -rc $@ client/*.o && $(RANLIB_BLRTS) $@

client/.object_files_built: $(PREFIX)_defs.h
	(cd client && \
	$(CC_BLRTS) -c $(CFLAGS) -I.. -I$(ZOID_HOME)/include -I$(INC_DIR) *.c \
	&& touch .object_files_built)

$(PREFIX)_preload.so: server/.object_files_built $(PREFIX)_dispatch.o
	$(CC) $(CFLAGS) $(CFLAGS_SHARED) $(LDFLAGS_SHARED) -o $@ \
	server/*.o $(PREFIX)_dispatch.o

server/.object_files_built: $(PREFIX)_defs.h
	(cd server && \
	$(CC) -c $(CFLAGS) $(CFLAGS_SHARED) -I.. -I$(ZOID_HOME)/include -I$(INC_DIR) *.c \
	&& touch .object_files_built)

$(PREFIX)_dispatch.o: $(PREFIX)_defs.h
	$(CC) -c $(CFLAGS) -I$(ZOID_HOME)/include $(CFLAGS_SHARED) $(PREFIX)_dispatch.c

$(PREFIX)_defs.h: $(PREFIX).h $(SCANNER)
	$(SCANNER) $<

.PHONY: implementation
implementation:
	$(MAKE) -C implementation

clean:
	(cd implementation && $(MAKE) clean)
	rm -f $(FTB_LIB_PATH_SHARED)/$(PREFIX)_preload.so \
	rm -f lib$(PREFIX)_blrts.a $(PREFIX)_preload.so \
	client/*.o client/.object_files_built client/*.c \
	server/*.o server/.object_files_built server/*.c \
	$(PREFIX)_defs.h $(PREFIX)_dispatch.c $(PREFIX)_dispatch.o *~

distclean: clean
