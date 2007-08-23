all: $(clients)

Linux: ftb_watchdog ftb_polling_logger ftb_simple ftb_notify_logger

BGL: ftb_simple_bgl

CFLAGS_LINUX  = -Wall -g -I$(INC_DIR)
LDFLAGS_LINUX = -L$(FTB_LIB_PATH_SHARED)
LIBS_LINUX    = -l$(FTB_LIB_LINUX_NAME) $(LIBS_PTHREAD)

CFLAGS_BGL    = -Wall -g -I$(INC_DIR) -I$(ZOID_HOME)/include -I$(ZOID_CLIENT_DIR)
LDFLAGS_CN    = -L$(FTB_LIB_PATH_STATIC) -L$(ZOID_LIBC_PATH) -L$(BGLSYS_LIB_PATH)
LIBS_CN       = -lftb_blrts -lc -lmsglayer.rts -ldevices.rts -lrts.rts -ldevices.rts -lrts.rts

clean:
	rm -f ftb_watchdog ftb_polling_logger ftb_simple ftb_simple_bgl

ftb_simple_bgl: ftb_simple.c
	@if [ -z "$(ZOID_HOME)" ]; then \
        	echo "ZOID_HOME is not set!  Aborting!"; \
		exit 1; \
	fi
	@if [ -z "$(ZOID_LIBC_PATH)" ]; then \
        	echo "ZOID_LIBC_PATH is not set!  Aborting!"; \
		exit 1; \
	fi
	@if [ -z "$(BLRTS_PATH)" ]; then \
        	echo "BLRTS_PATH is not set!  Aborting!"; \
		exit 1; \
	fi
	@if [ -z "$(BGLSYS_LIB_PATH)" ]; then \
		echo "BGLSYS_LIB_PATH is not set!  Aborting!"; \
		exit 1; \
	fi
	$(CC_BLRTS) $(CFLAGS_BGL) $(LDFLAGS_CN) -o $@ $< $(LIBS_CN)

ftb_simple: ftb_simple.c
	$(CC) $(CFLAGS_LINUX) $(LDFLAGS_LINUX) -o $@ $< $(LIBS_LINUX)

ftb_watchdog: ftb_watchdog.c
	$(CC) $(CFLAGS_LINUX) $(LDFLAGS_LINUX) -o $@ $< $(LIBS_LINUX)

ftb_polling_logger: ftb_polling_logger.c
	$(CC) $(CFLAGS_LINUX) $(LDFLAGS_LINUX) -o $@ $< $(LIBS_LINUX)

ftb_notify_logger: ftb_notify_logger.c
	$(CC) $(CFLAGS_LINUX) $(LDFLAGS_LINUX) -o $@ $< $(LIBS_LINUX)
