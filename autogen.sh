#!/bin/bash
#**************************************************************************
#  This file is part of FTB (Fault Tolerance Backplance) - the core of CIFTS
#  (Co-ordinated Infrastructure for Fault Tolerant Systems)
#  
#  See http://www.mcs.anl.gov/research/cifts for more information.
#   
#  This software is licensed under BSD. See the file FTB/misc/license.BSD for
#  complete details on your rights to copy, modify, and use this software.
#**************************************************************************

usage()
{
    echo "Usage: $0 {clean}"
}

if [ "$1" == "clean" ]; then
    echo -n "Cleaning up configuration files... "
    if test -f Makefile; then
        make clean
    fi
    rm -rf aclocal.m4 m4/lib* m4/lt* configure Makefile autom4te.cache confdb `find . -name \*~` \
    `find . -name \*\.d` `find . -name Makefile` `find . -name Makefile.in` include/ftb_config.* config.*
    echo "done"
    exit
elif [ "$1" != "" ]; then
    usage;
    exit
else
    echo -n "Setting up configuration files... "
#    aclocal
#    autoheader
    autoreconf -i
#    automake --add-missing
    echo done
fi

echo "Please proceed with the configuration"

