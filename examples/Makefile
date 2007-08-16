all: ftb_watchdog ftb_polling_logger

CFLAGS_LINUX = -Wall -g -L$(FTB_LIB_PATH_SHARED) -I../include
LIBS_LINUX   = -l$(FTB_LIB_LINUX_NAME) $(LIBS_PTHREAD)

clean:
	rm -f ftb_watchdog ftb_polling_logger

ftb_watchdog: ftb_watchdog.c
	$(CC) $(CFLAGS_LINUX) -o $@ $< $(LIBS_LINUX)

ftb_polling_logger: ftb_polling_logger.c
	$(CC) $(CFLAGS_LINUX) -o $@ $< $(LIBS_LINUX)
