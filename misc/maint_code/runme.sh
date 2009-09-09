#!/bin/sh

# Copy the ftb-info, ftb-fillin, ftb-bad, getfiles.sh, phbsrcfix and this file in the main directory of the software
# Run this script to replace the old information
# You will need to rerun this script with "--strip" option to remove the labels
sh ./getfiles.sh > outputfiles
for i in `cat outputfiles`; do
./phbsrcfix --label=ftb-info --insert=ftb-info $i 
./phbsrcfix --label=ftb-fillin --insert=ftb-fillin $i
./phbsrcfix --label=ftb-info --insert=ftb-bsd $i
done
