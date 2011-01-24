AC_PREREQ([2.67])

AC_INIT([FTB], [0.7], [cifts_discuss@googlegroups.com], [ftb],
	       [http://www.mcs.anl.gov/research/cifts])
AC_CONFIG_AUX_DIR(config)
AC_CONFIG_MACRO_DIR(config)
AM_INIT_AUTOMAKE([-Wall -Werror foreign 1.10])

LT_PREREQ([2.2.6])

# Bug in libtool adds -O2 and -g by default
save_cflags=$CFLAGS
LT_INIT
CFLAGS=$save_cflags

if test -s "$srcdir/VERSION" ; then
    . $srcdir/VERSION
else
    AC_MSG_ERROR([Version information not found. Configuration aborted.])
fi
AC_SUBST(libftb_so_version)
AC_SUBST(libftbm_so_version)

AC_CONFIG_HEADERS([include/ftb_config.h])

## Non-verbose make
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

# Checks for programs
save_cflags=$CFLAGS
AC_PROG_CC
CFLAGS=$save_cflags

# Require C99 support
AC_PROG_CC_C99
if test "$ac_cv_prog_cc_c99" = "no" ; then
   AC_ERROR([Compiler does not support C99. Aborting.])
fi

# Checks for header files.
AC_CHECK_HEADERS([arpa/inet.h fcntl.h netdb.h netinet/in.h stdint.h stdlib.h string.h \
	sys/ioctl.h sys/socket.h sys/time.h unistd.h pthread.h])

AC_C_INLINE
AC_TYPE_PID_T

# Checks for library functions.
AC_CHECK_FUNCS([gethostname gettimeofday inet_ntoa localtime_r memset select socket \
	strcasecmp strncasecmp strstr fork])

# Checks for pthread library
AC_SEARCH_LIBS([pthread_create], [pthread], [have_pthreads=yes],
	[AC_ERROR([Unable to find pthreads. Aborting.])])

warnings_set="List of warning displayed during configure time :"$'\n'
output_string="Variables_set :"$'\n'


# The version code is taken from the MPICH2 software package from
# Argonne National Laboratory.
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
AC_SUBST(FTB_VERSION)
AC_SUBST(FTB_NUMVERSION)


## Get bootstrap server name
AC_ARG_WITH(bstrap-server, 
	AC_HELP_STRING([--with-bstrap-server=ip_address],
		[IP Address of Bootstrapping server <no default>]),
	FTB_BSTRAP_SERVER=$withval,
	AC_WARN([No bootstrap server provided; provide at runtime]))
AC_MSG_CHECKING(for bootstrap server IP Address)
AC_MSG_RESULT($FTB_BSTRAP_SERVER)
if test "x$FTB_BSTRAP_SERVER" != "x"; then
	AC_DEFINE_UNQUOTED(FTB_BSTRAP_SERVER, "${FTB_BSTRAP_SERVER}",
		[IP Address for the FTB bootstrap server])
fi
output_string+="	FTB_BSTRAP_SERVER = ${FTB_BSTRAP_SERVER}"$'\n'


## Get bootstrap server port number
AC_ARG_WITH(bstrap-port,
	AC_HELP_STRING([--with-bstrap-port=port_number],
		[Port number of Bootstrapping server (default: 14455)]),
	FTB_BSTRAP_PORT=$withval,FTB_BSTRAP_PORT=14455)
AC_MSG_CHECKING(for bootstrap server port number)
AC_MSG_RESULT($FTB_BSTRAP_PORT)
AC_DEFINE_UNQUOTED(FTB_BSTRAP_PORT, ${FTB_BSTRAP_PORT},
		[Port number for the FTB bootstrap server])
output_string+="	FTB_BSTRAP_PORT   = ${FTB_BSTRAP_PORT}"$'\n'


## Get agent port number
AC_ARG_WITH(agent-port,
	AC_HELP_STRING([--with-agent-port=port_number],
		[Port number on which agent is running (default: 10809)]),
	FTB_AGENT_PORT=$withval, FTB_AGENT_PORT=10809)
AC_MSG_CHECKING(for agent port number)
AC_MSG_RESULT($FTB_AGENT_PORT)
AC_DEFINE_UNQUOTED(FTB_AGENT_PORT, ${FTB_AGENT_PORT}, [FTB Agent port])
output_string+="	FTB_AGENT_PORT    = ${FTB_AGENT_PORT}"$'\n'


## Get network type
FTB_NETWORK_TYPE_SOCK=false
AC_ARG_WITH(network-type,
	AC_HELP_STRING([--with-network-type=network_type],[Type of network (default: sock)]),
	FTB_NETWORK_TYPE=$withval,FTB_NETWORK_TYPE="sock")
AC_MSG_CHECKING(for network type)
AC_MSG_RESULT($FTB_NETWORK_TYPE)
case "$FTB_NETWORK_TYPE" in
	sock)
		FTB_NETWORK_TYPE_SOCK=true
	;;
	*)
		AC_ERROR([Invalid FTB network type])
	;;
esac
AC_DEFINE_UNQUOTED(FTB_NETWORK_TYPE, ${FTB_NETWORK_TYPE}, [FTB Network Type]) 
AC_DEFINE([FTB_NETWORK_SOCK], [1], [Define FTB_NETWORK_SOCK])
AM_CONDITIONAL([FTB_NETWORK_TYPE_SOCK], [$FTB_NETWORK_TYPE_SOCK])
output_string+="	FTB_NETWORK_TYPE  = ${FTB_NETWORK_TYPE}"$'\n'


## Debug
AC_ARG_ENABLE(debug,
	AC_HELP_STRING([--enable-debug],[Enable debug messages]),
	FTB_DEBUG=$enableval, FTB_DEBUG=no)
AC_MSG_CHECKING(for debug log directory)
AC_MSG_RESULT($FTB_DEBUG)
if test "x$FTB_DEBUG" != "xno"; then
   AC_DEFINE([FTB_DEBUG],[1], [FTB Debug definition])
   AC_DEFINE_UNQUOTED([FTB_LOGDIR],"${FTB_DEBUG}", [Path to the FTB Log Directory])
fi
output_string+="	DEBUG_ENABLED with log directory location = ${FTB_DEBUG}"$'\n'
echo "Attn Rinku: Need to clear up confusion with the --enable-debug:logdir"


## Expose option for platform type
AC_ARG_WITH(platform,
	AC_HELP_STRING([--with-platform=platform type],[Possible values: bgp, cray, linux]),
	PLATFORM=$withval, PLATFORM=linux)
AC_MSG_CHECKING(for platform support)
AC_MSG_RESULT($PLATFORM)
case "${PLATFORM}" in
	linux|cray)
	;;
	bgp) 
		AC_DEFINE([FTBI_PLATFORM_IS_BGP],1, [Set Platform to BGP])
	;;
	*)
		AC_ERROR([Invalid platform type])
	;;
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

AC_CONFIG_FILES([Makefile \
	include/ftb.h])
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

