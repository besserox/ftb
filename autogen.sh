#!/bin/bash

usage()
{
    echo "Usage: $0 {clean}"
}

if [ "$1" == "clean" ]; then
    echo -n "Cleaning up configuration files... "
    rm -rf config.* configure Makefile autom4te.cache `find . -name \*~` \
	`find . -name \*\.d`
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
