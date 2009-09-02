#!/bin/bash
#*****************************************************************************
# FTB:ftb-info
#  This software is licensed under BSD. See the file FTB/misc/license.BSD for
#  complete details on your rights to copy, modify, and use this software.
# FTB:ftb-info

# FTB:ftb-fillin
#  FTB_Version: 0.6.1
#  FTB_API_Version: 0.5
#  FTB_Heredity:FOSS_ORIG
#  FTB_License:BSD
# FTB:ftb-fillin
# FTB:ftb-bsd
#  This software is licensed under BSD. See the file FTB/misc/license.BSD for
#  complete details on your rights to copy, modify, and use this software.
# FTB:ftb-bsd
#*****************************************************************************

usage()
{
    echo "Usage: $0 {clean}"
}

if [ "$1" == "clean" ]; then
    echo -n "Cleaning up configuration files... "
	if test -f Makefile; then
    	make clean
	fi
    rm -rf config.* configure Makefile autom4te.cache `find . -name \*~` \
    `find . -name \*\.d` `find . -name Makefile` include/ftb_throw_events.h
    echo "done"
    exit
elif [ "$1" != "" ]; then
    usage;
    exit
else
    echo -n "Setting up configuration files... "
#    aclocal
    autoconf
#    automake --add-missing
    echo done
fi

echo "Please proceed with the configuration"
