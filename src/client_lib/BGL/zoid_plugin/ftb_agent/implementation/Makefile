#***************************************************************************
# ZEPTOOS:zepto-info
#      This file is part of ZeptoOS: The Small Linux for Big Computers.
#      See www.mcs.anl.gov/zeptoos for more information.
# ZEPTOOS:zepto-info
#
# ZEPTOOS:zepto-fillin
#      $Id: Makefile,v 1.5 2007/07/20 10:10:45 qgao Exp $
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

PREFIX = ftb_agent
CC = gcc
CFLAGS = -Wall -g -fPIC -O2 -I.. -I../../include -I$(FTB_INC_DIR) -DFTB_BGL 
CFLAGS_SHARED = -fPIC -mpowerpc -pthread
LDFLAGS = -shared

all: $(PREFIX)_impl.so 

$(PREFIX)_impl.so: $(FTB_SRC_DIR)/ftb_util.o ftb_zoid_plugin.o 
	$(CC) $(CFLAGS_SHARED) $(LDFLAGS) -o $@ $^

/ftb_util.o:
	echo "must run it by makefile in ftb"
	exit 1

install:
	cp $(PREFIX)_impl.so $(ZEPTO_INSTALLDIR)

clean:
	rm -f $(PREFIX)_impl.so *.o *~

distclean: clean
