examplesdir = ${prefix}/components/examples
examples_PROGRAMS = ftb_polling_logger ftb_notify_logger ftb_pingpong ftb_example_callback_unsubscribe ftb_eventhandle_example ftb_multiplecomp

if CLIENT_LINUX
examples_PROGRAMS += ftb_watchdog ftb_simple_publisher ftb_simple_subscriber
#ftb_ib_port_status_publisher ftb_throw_delay_mpi ftb_alltoall_mpi ftb_grouping_mpi
examples_DATA = watchdog_schema.ftb
ftb_watchdog_SOURCES = ftb_watchdog.c
ftb_watchdog_CFLAGS = -I${top_srcdir}/include
ftb_watchdog_LDADD = ../../src/client_lib/platforms/linux/libftb.la

ftb_simple_publisher_SOURCES = ftb_simple_publisher.c
ftb_simple_publisher_CFLAGS = -I${top_srcdir}/include
ftb_simple_publisher_LDADD = ../../src/client_lib/platforms/linux/libftb.la

ftb_simple_subscriber_SOURCES = ftb_simple_subscriber.c
ftb_simple_subscriber_CFLAGS = -I${top_srcdir}/include
ftb_simple_subscriber_LDADD = ../../src/client_lib/platforms/linux/libftb.la 

#CHECK NEEDED
#ftb_ib_port_status_publisher_SOURCES = ftb_ib_port_status_publisher.c
#ftb_ib_port_status_publisher_CFLAGS = -I${top_srcdir}/include -I/usr/local/ofed/include
#ftb_ib_port_status_publisher_LDFLAGS = /usr/local/ofed/lib
#ftb_ib_port_status_publisher_LDADD = ../../src/client_lib/platforms/linux/libftb.la libverbs

#CHECK NEEDED
#ftb_throw_delay_mpi_SOURCES = ftb_throw_delay_mpi.c
#ftb_throw_delay_mpi_COMPILE = mpicc
#ftb_throw_delay_mpi_CFLAGS = -I${top_srcdir}/include
#ftb_throw_delay_mpi_LDADD =../../src/client_lib/platforms/linux/libftb.la 

#CHECK NEEDED
#ftb_alltoall_mpi_SOURCES = ftb_alltoall.c
#ftb_alltoall_mpi_COMPILE = mpicc
#ftb_alltoall_mpi_CFLAGS = -I${top_srcdir}/include
#ftb_alltoall_mpi_LDADD = ../../src/client_lib/platforms/linux/libftb.la 

#CHECK NEEDED
#ftb_grouping_mpi_COMPILE =  mpicc
#ftb_grouping_mpi_SOURCES = ftb_grouping.c
#ftb_grouping_mpi_CFLAGS = -I${top_srcdir}/include
#ftb_grouping_mpi_LDADD = ../../src/client_lib/platforms/linux/libftb.la
endif

#CHECK NEEDED
if CLIENT_BGP
examples_PROGRAMS += ftb_watchdog_bgp ftb_simple_publisher_bgp ftb_simple_subscriber_bgp
examples_DATA = watchdog_schema.ftb
ftb_watchdog_bgp_SOURCES = ftb_watchdog.c
ftb_watchdog_bgp_CFLAGS = -I${top_srcdir}/include -I${ZOID_HOME}/include -I${top_srcdir}/src/client_lib/platforms/bg/zoid_client
ftb_watchdog_bgp_LDFLAGS = ../../src/client_lib/platforms/linux/libftb.la ${ZOID_LIBC_PATH} -L${BGSYS_LIB_PATH} 

ftb_simple_publisher_bgp_SOURCES = ftb_simple_publisher.c
ftb_simple_publisher_bgp_CFLAGS = -I${top_srcdir}/include -I${ZOID_HOME}/include -I${top_srcdir}/src/client_lib/platforms/bg/zoid_client
ftb_simple_publisher_bgp_LDFLAGS = ../../src/client_lib/platforms/linux/libftb.la ${ZOID_LIBC_PATH} -L${BGSYS_LIB_PATH} 

ftb_simple_subscriber_bgp_SOURCES = ftb_simple_subscriber.c
ftb_simple_subscriber_bgp_CFLAGS = -I${top_srcdir}/include -I${ZOID_HOME}/include -I${top_srcdir}/src/client_lib/platforms/bg/zoid_client
ftb_simple_subscriber_bgp_LDFLAGS = ../../src/client_lib/platforms/linux/libftb.la ${ZOID_LIBC_PATH} -L${BGSYS_LIB_PATH} 
endif

#CHECK NEEDED
if CLIENT_BGL
examples_PROGRAMS += ftb_watchdog_bgl ftb_simple_publisher_bgl ftb_simple_subscriber_bgl
examples_DATA = watchdog_schema.ftb
ftb_watchdog_bgl_SOURCES = ftb_watchdog.c
ftb_watchdog_bgl_CFLAGS = -I${top_srcdir}/include -I${ZOID_HOME}/include -I${top_srcdir}/src/client_lib/platforms/bg/zoid_client
ftb_watchdog_bgl_LDFLAGS = ../../src/client_lib/platforms/linux/libftb.la ${ZOID_LIBC_PATH} -L${BGSYS_LIB_PATH} 

ftb_simple_publisher_bgl_SOURCES = ftb_simple_publisher.c
ftb_simple_publisher_bgl_CFLAGS = -I${top_srcdir}/include -I${ZOID_HOME}/include -I${top_srcdir}/src/client_lib/platforms/bg/zoid_client
ftb_simple_publisher_bgl_LDFLAGS = ../../src/client_lib/platforms/linux/libftb.la ${ZOID_LIBC_PATH} -L${BGSYS_LIB_PATH} 

ftb_simple_subscriber_bgl_SOURCES = ftb_simple_subscriber.c
ftb_simple_subscriber_bgl_CFLAGS = -I${top_srcdir}/include -I${ZOID_HOME}/include -I${top_srcdir}/src/client_lib/platforms/bg/zoid_client
ftb_simple_subscriber_bgl_LDFLAGS = ../../src/client_lib/platforms/linux/libftb.la ${ZOID_LIBC_PATH} -L${BGSYS_LIB_PATH} 
endif

ftb_polling_logger_SOURCES = ftb_polling_logger.c
ftb_polling_logger_CFLAGS = -I${top_srcdir}/include
ftb_polling_logger_LDADD = ../../src/client_lib/platforms/linux/libftb.la

ftb_notify_logger_SOURCES = ftb_notify_logger.c
ftb_notify_logger_CFLAGS = -I${top_srcdir}/include
ftb_notify_logger_LDADD = ../../src/client_lib/platforms/linux/libftb.la

ftb_pingpong_SOURCES = ftb_pingpong.c
ftb_pingpong_CFLAGS = -I${top_srcdir}/include
ftb_pingpong_LDADD = ../../src/client_lib/platforms/linux/libftb.la

ftb_example_callback_unsubscribe_SOURCES = ftb_example_callback_unsubscribe.c
ftb_example_callback_unsubscribe_CFLAGS = -I${top_srcdir}/include
ftb_example_callback_unsubscribe_LDADD = ../../src/client_lib/platforms/linux/libftb.la

ftb_eventhandle_example_SOURCES = ftb_eventhandle_example.c
ftb_eventhandle_example_CFLAGS = -I${top_srcdir}/include
ftb_eventhandle_example_LDADD = ../../src/client_lib/platforms/linux/libftb.la

ftb_multiplecomp_SOURCES = ftb_multiplecomp.c
ftb_multiplecomp_CFLAGS = -I${top_srcdir}/include 
ftb_multiplecomp_LDADD = ../../src/client_lib/platforms/linux/libftb.la
