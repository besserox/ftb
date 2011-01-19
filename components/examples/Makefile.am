bin_PROGRAMS = ftb_watchdog ftb_polling_logger ftb_notify_logger
ftb_watchdog_SOURCES = ftb_watchdog.c 
ftb_watchdog_CFLAGS = -I${top_srcdir}/include
ftb_watchdog_LDADD = ../../src/client_lib/libftb.la
bin_SCRIPTS = watchdog_schema.ftb
EXTRA_DIST = watchdog_schema.ftb

ftb_polling_logger_SOURCES = ftb_polling_logger.c
ftb_polling_logger_CFLAGS = -I${top_srcdir}/include 
ftb_polling_logger_LDADD = ../../src/client_lib/libftb.la

ftb_notify_logger_SOURCES = ftb_notify_logger.c
ftb_notify_logger_CFLAGS = -I${top_srcdir}/include 
ftb_notify_logger_LDADD = ../../src/client_lib/libftb.la
