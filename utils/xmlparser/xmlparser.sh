#!/bin/bash

# ==============================================================================================================================
# 		 FILE:  xml_parser.sh
#	        USAGE:  ./xml_parser name_of_input_file name_of_output_file
# 	  DESCRIPTION:  A quick and ugly bash script for parsing the XML schema files to tab-delimited 
#		        event files for FTB demo. Note that successive iterations will replace this bash 
#		        script by a perl script
#
# 	 PROJECT NAME:  CIFTS Fault Tolerant Backplane
# 	       AUTHOR:  MCS Argonne National Labs
# 	CREATION DATE:  07/10/07
#   COPYRIGHT/LICENSE:  MCS ANL
#
# 	     REVISION:  07.13.07
# 	      VERSION:  0.1
#   MODIFICATION DATE:  07/13/07
#
#        ASSUMPTIONS :  Please read the following assumptions:
# 			1. The "<" and ">" characters reserved for indicating tags. Please dont use them elsewhere in your XML file
# 			2. If you define new tags in the XML file, the script will not recognize them
# 			3. If you need to change the format/event attributed in your tab-file, the script will need to be modified
#
#     LIST OF CHANGES:  08/04/07 : Added some comments in generated file
# ===================================================================================================================================

declare -a required_tags	# Contains the XML tags to be put in the tag-delimited file 
declare -a nonrequired_tags 	# Contains the XML tags that are valid but are currently not put in the tab-delimited file
declare -i count_events=0	# temp variabe
declare -i k=0			# temp variable
declare -i write_to_file=0	# temp variable
declare -i debug=0		# (will) contains debug level information
declare -i event_filetype=0	# temp variable

required_tags=(event_name event_severity)
nonrequired_tags=(component_category component_implementation event_category event_category event_category_name event_details event_scope event_impact event_desc)

#------------------------------------------------------------------------------------------------
# Declare an output array for every required tag. These arrays will hold the tag values
#------------------------------------------------------------------------------------------------
for (( i=0 ; i<${#required_tags[@]} ; i++ )); do
	`declare -a ${required_tags[$i]}`
done

if [ "`echo $1 | tr A-Z a-z`" == "--help" ] || [ "`echo $1 | tr A-Z a-z`" == "--usage" ] || 
	[  "`echo $1 | tr A-Z a-z`" == "-h" ] || [ $# -lt 2 ] ; then {
	echo -e "usage: ./xmlparser input_file output_file [event_file_type]"
	echo "     input_file: absolute path name to input XML file (Required parameter)"
	echo "    output-file: absolute path name to output space-delimited file (Required parameter)"
	echo -e "event-file_type: [1|0] '1' indicates built-in events and '0' (default) indicates component-unique events (Optional parameter)\n"
	exit -1
} fi

xml_filename=$1		# Input file name is store in xml_filename
output_filename=$2
event_filetype=$3
debug=$4

if [ $debug -eq 1 ]; then { 
	echo "DEBUG: Input XML file is $xml_filename"
	echo "DEBUG: Output tab-delimited file is $output_filename"
} fi

#---------------------------------------------------------------------------------------------------------
# DESCRIPTION: Pass 1 which validates the XML event attribute tags and exits if it finds an undefined tag
# 	       Every XML tag should belong to the nonrequired_tags or required_tags array
# 	       If tag is not recognizable, the program displays the error message and quits
#       TO DO: The script does not check if the event attributes are in the right order in the XML file
#	       The script break is event description contains a new line character 
#--------------------------------------------------------------------------------------------------------
if [ ! -e $xml_filename ]; then
	echo "$xml_filename XML input file does not exist. Please re-enter file path and name"
	exit -1;
fi
for tag in `cat $xml_filename | sed -e 's/\t//g' | sed -e 's/  */ /g' |  sed -e 's/^ *//g' | 
	sed -e 's/</\n</g' | sed -e 's/<!--/\n<!--/g' | sed -e 's/> */>/g'| sed -r 's/>/> /g' |
	sed -e '/^ *$/d' | sed '/^<\//d' | 
	awk '{ if ($0 ~ /<!--/) del = 1; if (!del) print $0; else if ($0 ~ /-->/) del = 0; }' |
	awk '{ if (($0 ~ /<\?/) || ($0 ~ "^<Component_and_Events") || ($0 ~ "^</Component_and_Events>")) del = 1;  
	if (!del) print $0;  else if ($0 ~ />/) del =0; }' |		
	sed -e 's/>.*/>/g' | grep [\<\>] | sort | uniq`; do {
		found=0;
		for i in ${required_tags[@]}; do
			if [ "$tag" == "<$i>" ]; then found=1; break; fi
		done
		if [ $found -eq 0 ]; then
			for i in ${nonrequired_tags[@]}; do
				if [ "$tag" == "<$i>" ]; then found=1; break; fi
			done
		fi
		if [ $found -eq 0 ]; then { 
			echo "XML file has unspecified tags : $tag not recognizable"; exit -1;
		} fi
	} done
	
#------------------------------------------------------------------------------------------------
# DESCRIPTION: Pass 2 obtains the relevant information from the XML file and adds it to the tab file
#------------------------------------------------------------------------------------------------
if [ -e $output_filename ]; then
	new_output_file=$output_filename.$RANDOM
	mv $output_filename  $new_output_file
	echo "The output file ($output_filename) already exists and will be backed up to $new_output_file"
fi

#------------------------------------------------------------------------------------------------
# The below statements are added to the beginning of the output file
#------------------------------------------------------------------------------------------------
echo "# This file is automatically generated by the FTB XML Schema parser" >> $output_filename
echo "# Event list file, listing events supported by FTB" >> $output_filename
echo "# This file will be used to generate FTB_events.h file" >> $output_filename
echo "# Notes:" >> $output_filename
echo "#    event_id is an automatically-assigned 32-bit unsigned integer" >> $output_filename
echo "# Format:" >> $output_filename
echo "#    EVENT_NAME EVENT_SEVERITY" >> $output_filename
echo "#    EVENT_NAME: encourage following name_space convention" >> $output_filename
echo "#        BACKPLANE:   for backplane events" >> $output_filename
echo "#        JOB:         for events regarding a job, like MPI job" >> $output_filename
echo "#        SYSTEM:      for events regarding a system, like a node" >> $output_filename
echo "#        GENERAL:     for all other events" >> $output_filename
echo "#    EVENT_SEVERITY: on of the following" >> $output_filename
echo "#        INFO" >> $output_filename
echo "#        PERFORMANCE" >> $output_filename
echo "#        WARNING" >> $output_filename
echo "#        ERROR" >> $output_filename
echo "#        FATAL" >> $output_filename
echo "#        " >> $output_filename
if [ $event_filetype -eq 1 ]; then {
	echo -e "VERSION 0.01\n" >> $output_filename
} fi

cat $xml_filename | 
	sed -e 's/\t//g' |						# Remove tabs
	sed -e 's/  */ /g' |  sed -e 's/^ *//g'|			# Remove redundant white spaces
	sed -e 's/</\n</g' |						# Move all text starting with "<" to new line
	sed -e 's/<!--/\n<!--/g' | 
	awk '{ if ($0 ~ /<!--/) del = 1; 
		if (!del) print $0; else if ($0 ~ /-->/) del = 0; }' |	# Delete all comments
	sed -e '/^ *$/d' |						# Delete blank lines
	sed '/^<\//d' |							# Delete end tags
	awk '{ if (($0 ~ /<\?/) || ($0 ~ "^<Component_and_Events") || ($0 ~ "^</Component_and_Events>")) del = 1; 
		if (!del) print $0;  else if ($0 ~ />/) del =0; }' |	# Delete starting xml and Component_and_Events declaration
	while read parse_line; do  {
		input_tag=`echo "$parse_line" | cut -d '>' -f 1`
		if [ "$input_tag" == "<event_details" ]; then {
			if [ $count_events -ne ${#required_tags[@]} ] && [ $k -gt 0 ]; then
				echo "Warning : An event has not been added since required tags are missing for that event. Please check XML file";
			fi
			count_events=0
			k=$k+1
			write_to_file=1
		} fi
		for (( j=0 ; j<${#required_tags[@]} ; j++)); do
				if [ "<${required_tags[$j]}" == "$input_tag" ]; then {
					input_tag_value=`echo "$parse_line" | cut -d '>' -f 2`
					if [ "$input_tag" == "<event_name" ]; then
						event_name[$k]=$input_tag_value;
					elif [ "$input_tag" == "<event_severity" ]; then
						event_severity[$k]=$input_tag_value;
					fi
					count_events=$count_events+1;
					break;
				} fi
		done
		if [ $count_events -eq ${#required_tags[@]} ] && [ $write_to_file -eq 1 ]; then {
				if [ $debug -eq 1 ]; then { 
					echo "DEBUG: Writing event with ${event_name[$k]} to output file";
				} fi
				# The output file is currently space-delimited instead of it being tab-delimited
				echo -e "${event_name[$k]} ${event_severity[$k]}\n" | tr a-z A-Z >>$output_filename;
				write_to_file=0
		} fi
	} done
