all: $(clients)

Linux: ftb_watchdog ftb_polling_logger ftb_simple_thrower ftb_simple_catcher ftb_notify_logger ftb_multi_comp

BGL: ftb_simple_thrower_bgl ftb_simple_catcher_bgl

CFLAGS_LINUX  = -Wall -g -I$(INC_DIR)
LDFLAGS_LINUX = -L$(FTB_LIB_PATH_SHARED)
LIBS_LINUX    = -l$(FTB_LIB_LINUX_NAME) $(LIBS_PTHREAD)

CFLAGS_BGL    = -Wall -g -I$(INC_DIR) -I$(ZOID_HOME)/include -I$(ZOID_CLIENT_DIR)
LDFLAGS_CN    = -L$(FTB_LIB_PATH_STATIC) -L$(ZOID_LIBC_PATH) -L$(BGLSYS_LIB_PATH)
LIBS_CN       = -lftb_blrts -lc -lmsglayer.rts -ldevices.rts -lrts.rts

clean:
	rm -f ftb_watchdog ftb_polling_logger ftb_simple_thrower ftb_simple_catcher ftb_notify_logger ftb_mutli_comp
	rm -f ftb_simple_thrower_bgl ftb_simple_catcher_bgl .bgl_env_test

.bgl_env_test:
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
	touch .bgl_env_test

ftb_simple_thrower_bgl: .bgl_env_test ftb_simple_thrower.c
	$(CC_BLRTS) $(CFLAGS_BGL) $(LDFLAGS_CN) -o $@ ftb_simple_thrower.c $(LIBS_CN)

ftb_simple_catcher_bgl: .bgl_env_test ftb_simple_catcher.c
	$(CC_BLRTS) $(CFLAGS_BGL) $(LDFLAGS_CN) -o $@ ftb_simple_catcher.c $(LIBS_CN)

ftb_simple_thrower: ftb_simple_thrower.c
	$(CC) $(CFLAGS_LINUX) $(LDFLAGS_LINUX) -o $@ $< $(LIBS_LINUX)

ftb_simple_catcher: ftb_simple_catcher.c
	$(CC) $(CFLAGS_LINUX) $(LDFLAGS_LINUX) -o $@ $< $(LIBS_LINUX)

ftb_watchdog: ftb_watchdog.c
	$(CC) $(CFLAGS_LINUX) $(LDFLAGS_LINUX) -o $@ $< $(LIBS_LINUX)

ftb_polling_logger: ftb_polling_logger.c
	$(CC) $(CFLAGS_LINUX) $(LDFLAGS_LINUX) -o $@ $< $(LIBS_LINUX)

ftb_notify_logger: ftb_notify_logger.c
	$(CC) $(CFLAGS_LINUX) $(LDFLAGS_LINUX) -o $@ $< $(LIBS_LINUX)

ftb_multi_comp: ftb_multi_comp.c
	$(CC) $(CFLAGS_LINUX) $(LDFLAGS_LINUX) -o $@ $< $(LIBS_LINUX)

