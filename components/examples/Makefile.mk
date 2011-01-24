bin_PROGRAMS += ftb_watchdog ftb_polling_logger ftb_notify_logger

ftb_watchdog_SOURCES = $(top_srcdir)/components/examples/ftb_watchdog.c 
ftb_watchdog_LDADD = libftb.la

bin_SCRIPTS += $(top_srcdir)/components/examples/watchdog_schema.ftb
EXTRA_DIST += $(top_srcdir)/components/examples/watchdog_schema.ftb

ftb_polling_logger_SOURCES = $(top_srcdir)/components/examples/ftb_polling_logger.c
ftb_polling_logger_LDADD = libftb.la

ftb_notify_logger_SOURCES = $(top_srcdir)/components/examples/ftb_notify_logger.c
ftb_notify_logger_LDADD = libftb.la
