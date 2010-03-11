#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.
AC_PREREQ([2.61])
AC_INIT([FTB], [0.7], [cifts@googlegroups.com], [ftb], [http://www.mcs.anl.gov/research/cifts])
AC_CONFIG_AUX_DIR([confdb])
AM_INIT_AUTOMAKE([-Wall -Werror foreign 1.10])
LT_INIT

AC_CONFIG_SRCDIR([src/client_lib/ftb_client_lib.c])
AC_CONFIG_MACRO_DIR([m4])
# Need to explicitly include this macro. Is there a workaround for it
m4_include([m4/ax_prefix_config_h.m4])
AC_CONFIG_HEADERS([include/ftb_config.h])
AX_PREFIX_CONFIG_H([include/ftb_config.h],[FTB])


# This denotes the SVN revision number for the configure.ac script. It needs to be updated 
# for every release
AC_REVISION([$Revision: Rev: 627 $])

######################################################################3
# Find the full path of the source dir
abs_srcdir=`(cd $srcdir && pwd)`
######################################################################3

# Checks for programs.
#saveCFLAGS=$CFLAGS
AC_PROG_CC
#CFLAGS=$saveCFLAGS
#AC_PROG_INSTALL

# Checks for libraries.

# Checks for header files.
AC_CHECK_HEADERS([arpa/inet.h fcntl.h netdb.h netinet/in.h stdint.h stdlib.h string.h sys/ioctl.h sys/socket.h sys/time.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_INLINE
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT8_T

# Checks for library functions.
AC_FUNC_FORK
AC_FUNC_MALLOC
AC_CHECK_FUNCS([gethostname inet_ntoa localtime_r memset select socket strcasecmp strncasecmp strstr])

PTHREAD_LIBS=error
AC_SEARCH_LIBS([pthread_create],[pthread],[PTHREAD_LIBS="-lpthread"])
if test "x${PTHREAD_LIBS}" = xerror; then
    AC_SEARCH_LIBS([pthread_create],[c_r],[PTHREAD_LIBS="-lc_r"])
fi
if test "x${PTHREAD_LIBS}" = xerror; then
    AC_CHECK_FUNC([pthread_create],[PTHREAD_LIBS=""])
fi
AC_SUBST(PTHREAD_LIBS)


###################################################################
# Get FTB version number (both string and numeric format)
#
# This code is taken from the MPICH2 software package from Argonne National Laboratory.
# Produce a numeric version assuming the following format:
# Version: [MAJ].[MIN].[REV][EXT][EXT_NUMBER]
# Example: 0.6.1rc1 has
#          MAJ = 0
#          MIN = 6
#          REV = 1
#          EXT = rc
#          EXT_NUMBER = 1
#
# Converting to numeric version will convert EXT to a format number:
#          ALPHA (a) = 0
#          BETA (b)  = 1
#          RC (rc)   = 2
#          PATCH (p) = 3
# Regular releases are treated as patch 0
#
# Numeric version will have 1 digit for MAJ, 2 digits for MIN,
# 2 digits for REV, 1 digit for EXT and 2 digits for EXT_NUMBER.
###################################################################
if test -s "$srcdir/version/version" ; then
    VERSION="`cat $srcdir/version/version`"
else
    AC_MSG_ERROR([Version information not found. Configuration aborted.])
fi
changequote(<<,>>)
V1=`expr $VERSION : '\([0-9]*\)\.[0-9]*[\.]*[0-9]*[a-zA-Z]*[0-9]*'`
V2=`expr $VERSION : '[0-9]*\.\([0-9]*\)[\.]*[0-9]*[a-zA-Z]*[0-9]*'`
V3=`expr $VERSION : '[0-9]*\.[0-9]*[\.]*\([0-9]*\)[a-zA-Z]*[0-9]*'`
V4=`expr $VERSION : '[0-9]*\.[0-9]*[\.]*[0-9]*\([a-zA-Z]*\)[0-9]*'`
V5=`expr $VERSION : '[0-9]*\.[0-9]*[\.]*[0-9]*[a-zA-Z]*\([0-9]*\)'`
changequote([,])
if test "$V2" -le 9 ; then V2=0$V2 ; fi
if test "$V3" = "" ; then V3=0; fi
if test "$V3" -le 9 ; then V3=0$V3 ; fi
if test "$V4" = "a" ; then
    V4=0
elif test "$V4" = "b" ; then
    V4=1
elif test "$V4" = "rc" ; then
    V4=2
elif test "$V4" = "" ; then
    V4=3
    V5=0
elif test "$V4" = "p" ; then
    V4=3
fi
if test "$V5" -le 9 ; 
    then V5=0$V5 ; 
fi
NUMVERSION=`echo $V1$V2$V3$V4$V5 | sed -e 's/^0*//'`
AC_DEFINE_UNQUOTED(NUMVERSION,$NUMVERSION,[FTB version number])
AC_DEFINE_UNQUOTED(VERSION,"$VERSION",[FTB version string])


AC_PRESERVE_HELP_ORDER
###################################################################
# Get bootstrap server's ip address and port number
# These can also be specified through environment variables
# on the FTB client side.
###################################################################
AC_ARG_WITH(bstrap-server, [AS_HELP_STRING([--with-bstrap-server=IP ADDRESS],
                            [IP Address of Bootstrapping server. NO default is set (An environment 
                            variable is expected on nodes running FTB clients. Check documentation.)])],
                [BSTRAP_SERVER=$withval])
AC_MSG_CHECKING(for bootstrap server IP Address)
AC_MSG_RESULT($BSTRAP_SERVER)
if test "$BSTRAP_SERVER" != ""; then
    AC_DEFINE_UNQUOTED(BSTRAP_SERVER, "${BSTRAP_SERVER}", 
                            [IP-Address for the FTB bootstrap server])
fi

AC_ARG_WITH(bstrap-port,[AS_HELP_STRING([--with-bstrap-port=PORT NUMBER],
                            [Port number of Bootstrapping server. Default is 14455])],
                [BSTRAP_PORT=$withval],
                [BSTRAP_PORT=14455])
AC_MSG_CHECKING(for bootstrap server port number)
AC_MSG_RESULT($BSTRAP_PORT)
AC_DEFINE_UNQUOTED(BSTRAP_PORT, ${BSTRAP_PORT}, 
                        [Port number for the FTB bootstrap server])


###################################################################
# Get port number where FTB Agent will listen
# This can also be specified through an environment variable
# on the FTB client side.
###################################################################
AC_ARG_WITH(agent-port, [AS_HELP_STRING([--with-agent-port=PORT NUMBER],
                            [Port number on which agent is running. Default is 10809])],
                [AGENT_PORT=$withval],
                [AGENT_PORT=10809])
AC_MSG_CHECKING(for agent port number)
AC_MSG_RESULT($AGENT_PORT)
AC_DEFINE_UNQUOTED(AGENT_PORT, ${AGENT_PORT}, [FTB Agent port])


###################################################################
# Get platform type
###################################################################
AC_ARG_WITH(platform, [AS_HELP_STRING([--with-platform=PLATFORM TYPE],
                            [Possible values: linux, bgp_zepto. Default is linux. 
                             Use "linux" if FTB will NOT be used by clients on Blue Gene compute nodes. 
                             Use "bgp_zepto" if FTB will be used by clients on Blue Gene compute nodes])],
                [CLIENTS=$withval],
                [CLIENTS=linux])
AC_MSG_CHECKING(for platform support)
AC_MSG_RESULT($CLIENTS)
AS_CASE([$CLIENTS],
  [linux],[
   platform_type="linux"
   with_platform_linux=yes],
  [bgp_zepto],[
   platform_type="bgp"
   with_platform_bgp=yes],
  [bgl_zepto],[
   platform_type="bgl"
   with_platform_bgl=yes],
  [AC_MSG_ERROR([Invalid argument to --with-platform: $CLIENTS])])
AM_CONDITIONAL([CLIENT_LINUX],[test "x$with_platform_linux" = xyes])
AM_CONDITIONAL([CLIENT_BGP],[test "x$with_platform_bgp" = xyes])
AM_CONDITIONAL([CLIENT_BGL],[test "x$with_platform_bgl" = xyes])
AC_MSG_RESULT([$platform_type])

###################################################################
# Get network type
###################################################################
AC_ARG_WITH(network, [AS_HELP_STRING([--with-network=NETWORK TYPE],
                            [Possible values: sock. Default is sock. 
                             Use "sock" if TCP/IP will be used by FTB for communication])],
                [NETWORK=$withval],
                [NETWORK=sock])
AC_MSG_CHECKING(for platform support)
AC_MSG_RESULT($NETWORK)
if test [ "$NETWORK" == "sock" ]; then
    FTB_NETWORK_EXTRA_CFLAGS="-DFTB_NETWORK_SOCK"
fi


AC_ARG_ENABLE(debug, [  --enable-debug: To enable debug messages. Default is no debug], DEBUG=$enableval, DEBUG=no)
AC_MSG_CHECKING(for debug log directory)
if [[ "$DEBUG" != "no" ]]; then
    AC_DEFINE(DEBUG,, [FTB Debug definition])
    AC_DEFINE_UNQUOTED(LOGDIR,"${FTB_DEBUG}", [FTB Log Directory])
fi
AC_MSG_RESULT($DEBUG)

###################################################################
# Get IBM Blue Gene Environment file.
# This is needed to compile ftb with ZOID/ZeptoOS sofwtare
# Also check to ensure that all ZOID/ZeptoOs variables are defined
###################################################################
AC_ARG_WITH(platform-envfile, [AS_HELP_STRING([--with-platform-envfile= BLUE_GENE ENVIRONMENT FILE], 
                                    [Default file is bg_setup_env for BG platforms. 
                                    For all other platforms, this variable is ignored.])],
                 [BG_ENVFILE=$withval], 
                 [BG_ENVFILE=])
AC_MSG_CHECKING(for user specified platform environment file)
if  test [ "$CLIENTS" == "bgp_zepto" ]; then
    if test [ "$BG_ENVFILE" == "" ]; then
        BG_ENVFILE="${abs_srcdir}/bg_setup_env"
    fi
    if test ! -r "$BG_ENVFILE" ; then
        echo "error: BG environment file '$BG_ENVFILE' does not exist or is not readable"
        echo "configure script aborting...correct errors first"
        exit 1
    else
        source $BG_ENVFILE
    fi
fi
AC_MSG_RESULT($BG_ENVFILE)
PLATFORM=linux
unset_var=no
if test [ "$CLIENTS" == "bgp_zepto" ]; then
    if test -z "$ZOID_HOME"; then
        unset_var="yes";
        echo "error: ZOID_HOME value undefined." >&2;
    fi
    if test -z "$ZOID_SRC"; then
        unset_var="yes";
        echo "error: ZOID_SRC value undefined." >&2;
    fi
    if test -z "$ZOID_LIBC_PATH"; then
        unset_var="yes";
        echo "error: ZOID_LIBC_PATH value undefined." >&2;
    fi
    if test -z "$BLRTS_PATH"; then
        unset_var="yes";
        echo "error: BLRTS_PATH value undefined." >&2;
    fi
    if test -z "$CC_BLRTS"; then
        unset_var="yes";
        echo "error: CC_BLRTS value undefined.";
    fi
    if test -z "$AR_BLRTS"; then
        unset_var="yes";
        echo "error: AR_BLRTS value undefined.";
    fi
    if test -z "$RANLIB_BLRTS"; then
        unset_var="yes";
        echo "error: RANLIB_BLRTS value undefined.";
    fi
    if test -z "$BGSYS_LIB_PATH"; then
        unset_var="yes";
        echo "error: BGSYS_LIB_PATH value undefined." >&2;
    fi
    if test "x${unset_var}" = xyes; then
        echo "configure script aborting...correct Blue Gene/P environment variable errors first" >&2;
        exit 1;
    fi
    PLATFORM="linux bg"
fi



AC_SUBST(ENVFILE)
AC_SUBST(ZOID_HOME)
AC_SUBST(ZOID_SRC)
AC_SUBST(ZOID_LIBC_PATH)
AC_SUBST(BLRTS_PATH)
AC_SUBST(CC_BLRTS)
AC_SUBST(AR_BLRTS)
AC_SUBST(RANLIB_BLRTS)
AC_SUBST(BGSYS_LIB_PATH)
AC_SUBST(PLATFORM)
AC_SUBST(NETWORK)

AC_CONFIG_FILES([Makefile
                 include/Makefile
                 src/Makefile
                 src/util/Makefile
                 src/manager_lib/Makefile
                 src/manager_lib/network/Makefile
                 src/manager_lib/network/network_sock/Makefile
                 src/agent/Makefile
                 src/client_lib/Makefile
                 src/client_lib/platforms/Makefile
                 src/client_lib/platforms/linux/Makefile
                 src/client_lib/platforms/bg/Makefile
                 src/client_lib/platforms/bg/zoid_client/Makefile
                 src/client_lib/platforms/bg/zoid_client/implementation/Makefile
                 src/client_lib/platforms/bg/zoid_client/server/Makefile
                 src/client_lib/platforms/bg/zoid_client/client/Makefile
                 docs/Makefile
                 docs/chapters/Makefile
                 docs/chapters/bg_sample_files/Makefile
                 docs/userguide/Makefile
                 components/Makefile
                 components/examples/Makefile])
#                 docs/Makefile])
#AC_CONFIG_SUBDIRS([components])
AC_OUTPUT
