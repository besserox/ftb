bin_PROGRAMS += ftb_watchdog ftb_polling_logger ftb_notify_logger ftb_eventhandle_example ftb_example_callback_unsubscribe ftb_multiplecomp ftb_pingpong ftb_simple_publisher ftb_simple_subscriber 

#How can I detect the presence of MPI and OpenIB and then compile the below programs? Need to add the fix in
#if HAVE_MPI
#bin_PROGRAMS += ftb_throw_delay_mpi ftb_alltoall ftb_grouping
#endif

ftb_watchdog_SOURCES = $(top_srcdir)/components/examples/ftb_watchdog.c 
ftb_watchdog_LDADD = libftb.la
bin_SCRIPTS += $(top_srcdir)/components/examples/watchdog_schema.ftb
EXTRA_DIST += $(top_srcdir)/components/examples/watchdog_schema.ftb

ftb_polling_logger_SOURCES = $(top_srcdir)/components/examples/ftb_polling_logger.c
ftb_polling_logger_LDADD = libftb.la

ftb_notify_logger_SOURCES = $(top_srcdir)/components/examples/ftb_notify_logger.c
ftb_notify_logger_LDADD = libftb.la

ftb_eventhandle_example_SOURCES = $(top_srcdir)/components/examples/ftb_eventhandle_example.c
ftb_eventhandle_example_LDADD = libftb.la

ftb_example_callback_unsubscribe_SOURCES = $(top_srcdir)/components/examples/ftb_example_callback_unsubscribe.c
ftb_example_callback_unsubscribe_LDADD = libftb.la

ftb_multiplecomp_SOURCES = $(top_srcdir)/components/examples/ftb_multiplecomp.c
ftb_multiplecomp_LDADD = libftb.la

ftb_pingpong_SOURCES = $(top_srcdir)/components/examples/ftb_pingpong.c
ftb_pingpong_LDADD = libftb.la

ftb_simple_publisher_SOURCES = $(top_srcdir)/components/examples/ftb_simple_publisher.c
ftb_simple_publisher_LDADD = libftb.la

ftb_simple_subscriber_SOURCES = $(top_srcdir)/components/examples/ftb_simple_subscriber.c
ftb_simple_subscriber_LDADD = libftb.la

#ftb_throw_delay_mpi_SOURCES = $(top_srcdir)/components/examples/ftb_throw_delay_mpi.c
#ftb_throw_delay_mpi_LDADD = libftb.la

#ftb_alltoall_SOURCES = $(top_srcdir)/components/examples/ftb_alltoall.c
#ftb_alltoall_LDADD = libftb.la

#ftb_grouping_SOURCES = $(top_srcdir)/components/examples/tb_grouping.c
#ftb_grouping_LDADD = libftb.la
