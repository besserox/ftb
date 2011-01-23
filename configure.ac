#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.67])
AC_INIT([FTB], [0.7], [cifts_discuss@googlegroups.com], [ftb], [http://www.mcs.anl.gov/research/cifts])
AM_INIT_AUTOMAKE
LT_INIT
AC_CONFIG_SRCDIR([src/util/ftb_util.c])
AC_CONFIG_HEADERS([include/ftb_config.h])

# Checks for programs.
AC_PROG_CC
AM_PROG_CC_C_O

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
AC_CHECK_FUNCS([gethostname gettimeofday inet_ntoa localtime_r memset select socket strcasecmp strncasecmp strstr])

#Checks for pthread library
have_pthreads=no
AC_SEARCH_LIBS([pthread_create], [pthread], [have_pthreads=yes])

warnings_set="List of warning displayed during configure time :"$'\n'
output_string="Variables_set :"$'\n'
#check for pthreads header
if test "x${have_pthreads}" = xyes; then
	AC_CHECK_HEADERS([pthread.h],[], [have_pthreads=no])
fi

if test "x${have_pthreads}" = xno; then
	echo "************** ERROR: Unable to find pthreads on this system. Configure exiting"
	(exit 1); exit 1;
fi

# Try to find the version
# The version code is taken from the MPICH2 software package from Argonne National Laboratory.
if test -s "$srcdir/version/version" ; then
    FTB_VERSION="`cat $srcdir/version/version`"
else
    AC_MSG_ERROR([Version information not found. Configuration aborted.])
fi

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
changequote(<<,>>)
V1=`expr $FTB_VERSION : '\([0-9]*\)\.[0-9]*[\.]*[0-9]*[a-zA-Z]*[0-9]*'`
V2=`expr $FTB_VERSION : '[0-9]*\.\([0-9]*\)[\.]*[0-9]*[a-zA-Z]*[0-9]*'`
V3=`expr $FTB_VERSION : '[0-9]*\.[0-9]*[\.]*\([0-9]*\)[a-zA-Z]*[0-9]*'`
V4=`expr $FTB_VERSION : '[0-9]*\.[0-9]*[\.]*[0-9]*\([a-zA-Z]*\)[0-9]*'`
V5=`expr $FTB_VERSION : '[0-9]*\.[0-9]*[\.]*[0-9]*[a-zA-Z]*\([0-9]*\)'`
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
if test "$V5" -le 9 ; then V5=0$V5 ; fi

FTB_NUMVERSION=`echo $V1$V2$V3$V4$V5 | sed -e 's/^0*//'`
AC_DEFINE_UNQUOTED(FTB_NUMVERSION,$FTB_NUMVERSION,[FTB version number])
AC_DEFINE_UNQUOTED(FTB_VERSION,"$FTB_VERSION",[FTB version string])


dnl Get bootstrap server name
AC_ARG_WITH(bstrap-server, 
	[  --with-bstrap-server=   <IP Address> IP Address of Bootstrapping server <no default>], 
	 FTB_BSTRAP_SERVER=$withval)
AC_MSG_CHECKING(for bootstrap server IP Address)
AC_MSG_RESULT($FTB_BSTRAP_SERVER)
if test "x$FTB_BSTRAP_SERVER" != "x"; then
	AC_DEFINE_UNQUOTED(FTB_BSTRAP_SERVER, "${FTB_BSTRAP_SERVER}", [IP Address for the FTB bootstrap server])
fi
if test "x$FTB_BSTRAP_SERVER" = "x"; then
	echo "************** WARNING: Bootstrap IP address not provided. Expecting it to be provided as environment variable FTB_BSTRAP_SERVER during run-time"
	warnings_set+="************** WARNING: Bootstrap IP address not provided. Expecting it to be provided as environment variable FTB_BSTRAP_SERVER during run-time"
	warnings_set+=$'\n'
fi
output_string+="	FTB_BSTRAP_SERVER = ${FTB_BSTRAP_SERVER}"$'\n'

dnl Get bootstrap server port number
AC_ARG_WITH(bstrap-port, [  --with-bstrap-port=<port number>	Port number of Bootstrapping server <14455>],
	                 FTB_BSTRAP_PORT=$withval, FTB_BSTRAP_PORT=14455)
AC_MSG_CHECKING(for bootstrap server port number)
AC_MSG_RESULT($FTB_BSTRAP_PORT)
AC_DEFINE_UNQUOTED(FTB_BSTRAP_PORT, ${FTB_BSTRAP_PORT}, [Port number for the FTB bootstrap server])
output_string+="	FTB_BSTRAP_PORT   = ${FTB_BSTRAP_PORT}"$'\n'

dnl Get agent port number
AC_ARG_WITH(agent-port, [  --with-agent-port=<port number>	Port number on which agent is running <10809>],
			FTB_AGENT_PORT=$withval, FTB_AGENT_PORT=10809)
AC_MSG_CHECKING(for agent port number)
AC_MSG_RESULT($FTB_AGENT_PORT)
AC_DEFINE_UNQUOTED(FTB_AGENT_PORT, ${FTB_AGENT_PORT}, [FTB Agent port])
output_string+="	FTB_AGENT_PORT    = ${FTB_AGENT_PORT}"$'\n'

dnl Get network type
AC_ARG_WITH(network-type, [  --with-network-type=<network_type>	Type of network <sock>],
			FTB_NETWORK_TYPE=$withval, FTB_NETWORK_TYPE="sock")
AC_MSG_CHECKING(for network type)
AC_MSG_RESULT($FTB_NETWORK_TYPE)
case "$FTB_NETWORK_TYPE" in
	sock);;
	*) echo "************** ERROR: FTB_NETWORK_TYPE is not a valid option to --with-network-type. Valid option is 'sock' (default) as of now. Exiting. Please run with the correct options"; (exit 1); exit 1;;
esac
AC_DEFINE_UNQUOTED(FTB_NETWORK_TYPE, ${FTB_NETWORK_TYPE}, [FTB Network Type]) 
AC_DEFINE([FTB_NETWORK_SOCK], [1], [Define FTB_NETWORK_SOCK])
output_string+="	FTB_NETWORK_TYPE  = ${FTB_NETWORK_TYPE}"$'\n'


AC_ARG_ENABLE(debug, [  --enable-debug: To enable debug messages. Default had debug disabled], FTB_DEBUG=$enableval, FTB_DEBUG=no)
AC_MSG_CHECKING(for debug log directory)
AC_MSG_RESULT($FTB_DEBUG)
if test "x$FTB_DEBUG" != "xno"; then
   AC_DEFINE([FTB_DEBUG],[1], [FTB Debug definition])
   AC_DEFINE_UNQUOTED([FTB_LOGDIR],"${FTB_DEBUG}", [Path to the FTB Log Directory])
fi
output_string+="	DEBUG_ENABLED with log directory location = ${FTB_DEBUG}"$'\n'
echo "Attn Rinku: Need to clear up confusion with the --enable-debug:logdir"

dnl Expose option for platform type
AC_ARG_WITH(platform, [  --with-platform=<platform type>        Possible values: bgp, cray, linux],
                      PLATFORM=$withval, PLATFORM=linux)
AC_MSG_CHECKING(for platform support)
AC_MSG_RESULT($PLATFORM)
case "${PLATFORM}" in
	linux);;
	cray);;
	bgp);;
	*) echo "************** $PLATFORM is not a valid option to --with-platform. Valid options are bgp, cray, linux(default). Configure exiting. Please run with the correct options."; (exit 1); exit 1;;
esac
AM_CONDITIONAL([PLATFORM_IS_LINUX], [test "x$PLATFORM" = "xlinux"])
AM_CONDITIONAL([PLATFORM_IS_BGP], [test "x$PLATFORM" = "xbgp"])
AM_CONDITIONAL([PLATFORM_IS_CRAY], [test "x$PLATFORM" = "xcray"])
output_string+="	PLATFORM          = ${PLATFORM}"$'\n'

dnl Expose option for BG environment file
dnl Check if all the correct variables are present in the Blue Gene environment file
AC_ARG_WITH(platform-envfile, [  --with-platform-envfile=<bg envfile>   Default file is bg_setup_env for BGP
                                        platforms. For all other platforms, this file
                                        is ignored.],
                        ENVFILE=$withval, ENVFILE=none)
AC_MSG_CHECKING(for user specified platform environment file)
AC_MSG_RESULT($ENVFILE)
if test "x$ENVFILE" = "xnone"; then
dnl BUG: The below statement might get executed if the user specifies --with-platform-envfile=none and --with-platform=bgp, causing configure to exit.
	if test "x$PLATFORM" = "xbgp"; then
		echo "************** If the option --with-platform=bgp is being used, then you need to provide the location of the Blue Gene environment file with the --with-platform-envfile option. Configure exiting. Please correct errors first"; (exit 1); exit 1;
	fi
else
	if test "x$PLATFORM" != "xbgp"; then
		echo "************** WARNING: --with-platform-envfile is a valid option only when you also specify --with-platform=bgp. Since, --with-platform is NOT set to bgp, the --with-platform-envfile is being ignored";
		warnings_set+="************** WARNING: --with-platform-envfile is a valid option only when you also specify --with-platform=bgp. Since, --with-platform is NOT set to bgp, the --with-platform-envfile is being ignored"
		warnings_set+=$'\n'
	else
		if test ! -r "$ENVFILE" ; then
			echo "************** ERROR: The Blue Gene confguration file $ENVFILE specified by --with-platform-envfile is not readable or does not exit.  Configure exiting. Please correct errors first"; (exit 1); exit 1;
		else
			source $ENVFILE;
			output_string+=$'\n'$'\n'"If platform = BGP, the following variables will be set"$'\n'
			output_string+="Environment file location (applicable only for BGP systems) = ${ENVFILE}"$'\n'
			unset_var=0;
			if test -z "$ZOID_HOME"; then
				unset_var=1;
				echo "************** ERROR: Required environment variable not set: ZOID_HOME. Please check BG environment variables file.";
			fi
			output_string+="	ZOID_HOME 	= ${ZOID_HOME}"$'\n'
			if test -z "$ZOID_SRC"; then
				unset_var=1;
				echo "************** ERROR: Required environment variable not set: ZOID_SRC. Please check BG environment variables file.";
			fi
			output_string+="	ZOID_SRC 	= ${ZOID_SRC}"$'\n'
			if test -z "$ZOID_LIBC_PATH"; then
				unset_var=1;
				echo "************** ERROR: Required environment variable not set: ZOID_LIBC_PATH. Please check BG environment variables file.";
			fi
			output_string+="	ZOID_LIBC_PATH	= ${ZOID_LIBC_PATH}"$'\n'
			if test -z "$BLRTS_PATH"; then
				unset_var=1;
				echo "************** ERROR: Required environment variable not set: BLRTS_PATH. Please check BG environment variables file.";
			fi
			output_string+="	BLRTS_PATH	= ${BLRTS_PATH}"$'\n'
			if test -z "$CC_BLRTS"; then
				unset_var=1;
				echo "************** ERROR: Required enviroment variable not ste: CC_BLRTS. Please check BG environment variables file.";
			fi
			output_string+="	CC_BLRTS	= ${CC_BLRTS}"$'\n'
			if test -z "$AR_BLRTS"; then
				unset_var=1;
				echo "************** ERROR: Required enviroment variable not ste: AR_BLRTS. Please check BG environment variables file.";
			fi
			output_string+="	AR_BLRTS	= ${AR_BLRTS}"$'\n'
			if test -z "$RANLIB_BLRTS"; then
				unset_var=1;
				echo "************** ERROR: Required enviroment variable not ste: RANLIB_BLRTS. Please check BG environment variables file.";
			fi
			output_string+="	RANLIB_BLRTS	= ${RANLIB_BLRTS}"$'\n'
			if test -z "$BGSYS_LIB_PATH"; then
				unset_var=1;
				echo "************** ERROR: Required environment variable not set: BGSYS_LIB_PATH. Please check BG environment variables file.";
			fi
			output_string+="	BGSYS_LIB_PATH	= ${BGSYS_LIB_PATH}"$'\n'
			if test $unset_var == 1; then
				echo "************** ERROR: Configure script aborting...correct above errors first";
				exit 1;
			fi
		fi
	fi
fi

AC_CONFIG_FILES([Makefile
		include/Makefile
		src/Makefile
		src/util/Makefile
		src/manager_lib/Makefile
		src/manager_lib/network/Makefile
		src/manager_lib/network/network_sock/Makefile
		src/manager_lib/network/include/Makefile
		src/agent/Makefile
		src/client_lib/Makefile
		src/client_lib/platforms/Makefile
		src/client_lib/platforms/linux/Makefile
		components/Makefile
		components/examples/Makefile
		docs/Makefile
		docs/chapters/Makefile
		docs/chapters/bg_sample_files/Makefile
		version/Makefile
		src/client_lib/platforms/bg/Makefile
		src/client_lib/platforms/bg/zoid_client/implementation/Makefile
		src/client_lib/platforms/bg/zoid_client/Makefile
		src/client_lib/platforms/bg/zoid_client/server/Makefile
		src/client_lib/platforms/bg/zoid_client/client/Makefile
])
AC_OUTPUT

echo \
"-----------------------------------------------------------

${PACKAGE_NAME} Version ${PACKAGE_VERSION}
Prefix: {prefix}
Compiler: ${CC} ${CFLAGS} ${CPPFLAGS}

${warnings_set}
${output_string}

Now type: 'make @<:@<target>@>:@'
	where the optional <target> is:
	all  	- build all binaries
	install	- install everything

----------------------------------------------------------"

