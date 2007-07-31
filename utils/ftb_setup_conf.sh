#!/bin/sh

export FTB_ID=1
export FTB_PORT=10808

echo "/* Automatically generated.  Do not edit!  */" > ftb_conf.h
echo "#ifndef FTB_CONF_H" >> ftb_conf.h
echo "#define FTB_CONF_H" >> ftb_conf.h
echo "#define FTB_DEFAULT_CONF_FILE \"$PWD/ftb_conf\"" >> ftb_conf.h
echo "#endif" >> ftb_conf.h

echo "$FTB_ID `hostname -s` $FTB_PORT" > ftb_conf
