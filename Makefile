all: ftb ftb_examples

export prefix       = $(PWD)
export exec_prefix  = $(prefix)
export network      = sock
export FTB_NETWORK_CFLAG = -DFTB_NETWORK_SOCK
export clients      = Linux BGL
#export clients      = Linux
export libdir       = $(exec_prefix)/lib
export bindir       = $(exec_prefix)/bin

export ZOID_LIBC_PATH  = $(HOME)/zeptoos/ZeptoOS-1.5-V1R3M2_03/BGL/build/ZeptoOS-1.5-V1R3M2
export ZOID_HOME       = $(HOME)/zoid
export BLRTS_PATH      = /bgl/BlueLight/ppcfloor/blrts-gnu/powerpc-bgl-blrts-gnu
export BGLSYS_LIB_PATH = /bgl/BlueLight/V1R3M2_140_2007-070424/ppc/bglsys/lib

export MAKE          = make
export CC            = gcc
export AR            = ar
export RANLIB        = ranlib
export INC_DIR       = $(PWD)/include
export CFLAGS_FTB    = -Wall -g -fPIC -I$(INC_DIR)
export LIBS_PTHREAD  = -lpthread

export CC_BLRTS     = $(BLRTS_PATH)/bin/gcc
export AR_BLRTS     = $(BLRTS_PATH)/bin/ar
export RANLIB_BLRTS = $(BLRTS_PATH)/bin/ranlib
export ZOID_CLIENT_DIR = $(PWD)/src/client_lib/platforms/BGL/zoid_client

export FTB_LIB_PATH_SHARED  = $(libdir)/shared
export FTB_LIB_PATH_STATIC  = $(libdir)/static
export FTB_LIB_LINUX_NAME   = ftb
export FTB_LIB_BGL_CN_NAME  = ftb_blrts

clean:
	(cd src && $(MAKE) clean)
	(cd examples && $(MAKE) clean)
	rm -f .event_def .config_def

ftb: .event_def .config_def
	(cd src && $(MAKE))

ftb_examples:
	(cd examples && $(MAKE))

.event_def: 
	touch .event_def

.config_def:
	touch .config_def
