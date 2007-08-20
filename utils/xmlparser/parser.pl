#!/usr/bin/perl -w

use strict;
use XML::DOM;

my $outfile = "file.out";
my $parser  = new XML::DOM::Parser();

# Declare the Prefixes for the event and component attributes
my $component_ctgy_prefix  = 'FTB_EVENT_DEF_COMP_CTGY_';
my $component_prefix       = 'FTB_EVENT_DEF_COMP_';
my $event_severity_prefix  = 'FTB_EVENT_DEF_SEVERITY_';
my $event_ctgy_prefix      = 'FTB_EVENT_DEF_EVENT_CTGY_';
my $event_name_prefix      = 'FTB_EVENT_DEF_';

# Declare the global variables
my $num_component_ctgy = 0;
my $num_component = 0;
my $num_event_ctgy = 0;
my $num_event_severity = 0;
my $num_event_name = 8;
my $str_component_ctgy = "/*Definition of component categories*/\n";
my $str_component = "/*Definition of components*/\n";
my $str_event_ctgy = "/*Definition of event categories*/\n";
my $str_event_severity = "/*Definition of event severity*/\n";
my $str_event_name = "/*Definition of event name*/\n";
my $component_ctgy_name = '';
my $component_name = '';
my $DEBUG = 1;

# Subroutine to trim start and end of string
sub trim {
    my ($string) = @_;
    $string =~ s/^\s+//;
    $string =~ s/\s+$//;
    return $string;
}

# Subroutine to check existence of a file.
# Note that subroutine does not validate the
# syntax of the file contents in any way.
sub check_xml_files {
    foreach my $file (@_) {
	if ((! -e $file) || (! -r $file) || (-z $file)){
	    die "Input file \'$file\' cannot be read. It either does not exist or is unreadable or empty\n";
	}
    }
    return 0;
}

# Subroutine to write data to a file
sub write_file {
    my ($action, $data, $fname) =  @_;
    if ($action eq "CREATE") {
	open(OUTFILE, ">$fname");
    }
    elsif ($action eq "APPEND") {
	open(OUTFILE, ">>$fname");
    }
    print OUTFILE $data;
    if ($DEBUG) {
	print "Subroutine writefile():Writing the following in \'$fname\' in \'$action\' mode:\n $data";
    }
    close(OUTFILE);
}

# Subroutine to handle throw_event ELEMENT_NODE
sub evaluate_throw_event {
    my ($node, $component_ctgy, $component, $input_file) = @_;
    my $num_event_attr = 0;
    print "EVAL THROW EVENT\n";
    foreach my $event ($node->getChildNodes()) {
	if ($event->getNodeType() eq ELEMENT_NODE) {
	    SWITCH: {
		if ($event->getNodeName() eq "event_name") {
		    my $event_name = &trim($event->getFirstChild()->getData());
	    	    $event_name =~ tr/a-z/A-Z/;
		    if ($str_event_name =~ / $event_name_prefix$event_name /) { 
			    die "Duplicate event name: $event_name. Please correct errors\n"; 
		    }
		    $str_event_name .= "#define ".$event_name_prefix.$event_name."    ".hex($num_event_name)."\n";
		    $num_event_attr += 1;
		    $num_event_name += 1;
		    last SWITCH;
		}
	    	if ($event->getNodeName() eq "event_severity") {
		    my $event_severity = &trim($event->getFirstChild()->getData());
	    	    $event_severity =~ tr/a-z/A-Z/;
		    if ($str_event_severity =~ / $event_severity_prefix$event_severity /) { 
		    	last SWITCH;
		    }
		    $str_event_severity .= "#define ".$event_severity_prefix.$event_severity."    (1<<".$num_event_severity.")\n";
		    $num_event_severity += 1;
		    $num_event_attr += 1; 
		    last SWITCH;
		}
		if ($event->getNodeName() eq "event_category" ) {
		    my $event_ctgy = &trim($event->getFirstChild()->getData());
	    	    $event_ctgy =~ tr/a-z/A-Z/;
		    if ($str_event_ctgy =~ / $event_ctgy_prefix$event_ctgy /) { 
		    	last SWITCH;
		    }
	            $str_event_ctgy .= "#define ".$event_ctgy_prefix.$event_ctgy."     (1<<".$num_event_ctgy.")\n";
		    $num_event_ctgy += 1;
		    $num_event_attr += 1 ;
		    last SWITCH;
		}
	    }
	}
    }
    if ($num_event_attr != 3) {
        die "All event attributes not defined for a throw event in $input_file. Please check $input_file file\n"
    }
    
}

# Subroutine to evaluate value contained in second-level ELEMENT_NODE
sub evaluate_element_node {
    my ($node, $input_file) = @_;
    SWITCH: {
    	if ($node->getNodeName() eq "component_category") {
	    $num_component_ctgy += 1;
	    $component_ctgy_name = &trim($node->getFirstChild()->getData());
	    $component_ctgy_name =~ tr/a-z/A-Z/;
	    if ($str_component_ctgy =~ / $component_ctgy_prefix$component_ctgy_name /) { 
                last SWITCH;
	    }
	    $str_component_ctgy .= "#define ".$component_ctgy_prefix.$component_ctgy_name.	
	    				"     (1<<".$num_component_ctgy.")\n";
	    if ($DEBUG) {
		print "Subroutine evaluate_element_node(): Found ",$node->getNodeName(),
				"=",$component_ctgy_name,"\n";
	    }
	    last SWITCH;
	}
    	if ($node->getNodeName() eq "component") {
	    $num_component += 1;
	    $component_name = &trim($node->getFirstChild()->getData());
	    $component_name =~ tr/a-z/A-Z/;
	    if ($str_component =~ / $component_prefix$component_name /) { 
	        die "Duplicate component name: $component_name. Please correct errors\n"; 
	    }
	    $str_component .= "#define ".$component_prefix.$component_name."     (1<<".$num_component.")\n";
	    if ($DEBUG) {
	    	print "Subroutine evaluate_element_node(): Found ",$node->getNodeName(),
				"=",$component_name,"\n";
	    }
	    last SWITCH;
	}
	if ($node->getNodeName() eq "throw_event") {
	    if ($DEBUG) {
		print "Subroutine evaluate_element_node(): Found throw event. Further evaluating it.\n";
	    }
	    &evaluate_throw_event($node, $component_ctgy_name, $component_name, $input_file);
	    last SWITCH;
	}
	die "An unexpected tag called \'", $node->getNodeName(),
			"\' is present in one of the input files. Exiting..\n";
    }
}


# Main section of the parser
# The parser is aware of the format and tree layout of the node in the XML file
my @input_files = @ARGV;
my $start_data = "/* This file is automatically generated by the xmlparser. It will be \n".
			"used by the FTB framework to interpret components and events*/\n".
			"#ifndef FTB_EVENT_DEF_H\n#define FTB_EVENT_DEF_H\n\n#include \"ftb_def.h\" \n";
my $found = 0;
&check_xml_files(@input_files);
&write_file("CREATE", $start_data, $outfile);
foreach my $file (@input_files) {
    $found = 0;  
    my $doc = $parser->parsefile($file);
    foreach my $node ($doc->getChildNodes()) {
	if ($DEBUG) { 
	    print "-----------------------------------------------------------\n";
	    print "Main section: Input File \'$file\' has first-level node name: ", $node->getNodeName(),"\n"; 
	}
	if ($node->getNodeName() eq "ftb_component_details") { 
	    $found = 1;
	    foreach my $cnodes ($node->getChildNodes()) {
		if ($cnodes->getNodeType() == ELEMENT_NODE) {
		    if ($DEBUG) {
	    		print "-----------------------------------------------------------\n";
			print "Main section: Found second-level ELEMENT_NODE. Futher evaluating it.\n";
		    }
		    &evaluate_element_node($cnodes, $file);
		}
		else {
		    if ($DEBUG) { 
	    		print "-----------------------------------------------------------\n";
			print "Main section: Found second-level non-ELEMENT node ", "with value \'",$cnodes->getData(),"\'. This is ignored\n"; 
		    }
		}
	    }
	}
    }
    if ($found != 1) {
	die "<ftb_component_details> tag not found in $file. Parser cannot proceed\n";
    }
    print "FINAL =".$str_component_ctgy."\n";
    print "FINAL =".$str_component."\n";
    print "FINAL= ". $str_event_name."\n";
    print "FINAL= ". $str_event_ctgy."\n";
    print "FINAL= ". $str_event_severity."\n";
} # done analyzing all input files
