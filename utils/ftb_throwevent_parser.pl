#!/usr/bin/perl -w

use strict;
use XML::DOM;

my $outfile = "ftb_throw_events.h";
my $parser  = new XML::DOM::Parser();

# Declare the Prefixes for the event and component attributes
my $component_cat_prefix  = ''; #'FTB_EVENT_DEF_COMP_CAT_';
my $component_prefix      = ''; #'FTB_EVENT_DEF_COMP_';
my $event_cat_prefix      = ''; #FTB_EVENT_DEF_EVENT_CAT_';
my $event_name_prefix     = ''; #FTB_EVENT_DEF_';
my $event_name_suffix     = '_CODE';

# Declare the global variables
#  The below variabes will function as counters.
my $max_num_component_cat = 16;
my $max_num_component = 8;
my $max_num_event_cat = 8;
my $max_num_event_name = 5000; # currently limiting events for each component to 5000
my $num_component_cat = 1;
my %num_component;
my $num_event_cat = 0;
my $num_event_name = 0;

# The below variables will function as string for holding the data to be written to output file
my $str_component_cat = "/*Definition of component categories*/\n";
my $str_component = "/*Definition of components*/\n";
my $str_event_cat = "/*Definition of event categories*/\n";
#NOTE: There can only be a maximum of 8 severity values
my $str_event_severity= "/*Definition of event severities*/\n".
			"#define FTB_INFO  (1<<0)\n".
			"#define FTB_FATAL (1<<1)\n".
			"#define FTB_ERROR (1<<2)\n".
			"#define FTB_PERF_WARNING (1<<3)\n".
			"#define FTB_WARNING (1<<4)\n";
my $str_event_name = "/*Definition of event name*/\n";
my $str_struct = '';

# The below variables will fucntion as temporary variables
my $component_cat_name_new = '';
my $component_name_new = '';
my $total_throw_events = 0;
my $DEBUG = 0;

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
	    print "Input file \'$file\' cannot be read. It either does not exist or is unreadable or empty\n";
	    exit;
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
    my ($node, $component_cat_new, $component_new, $input_file) = @_;
    my $num_event_attr = 0;
    my $event_name_new = '';
    my $event_severity_new = '';
    my $event_cat_new = '';
    my $event_name = '';
    foreach my $event ($node->getChildNodes()) {
	if ($event->getNodeType() eq ELEMENT_NODE) {
	    SWITCH: {
		if ($event->getNodeName() eq "event_name") {
		    if ($num_event_name == $max_num_event_name) {
			$_=$component_new; s/$component_prefix//;
		        print "FTB cannot add more components to $_. Max limit of ( $max_num_event_name) reached\n";
		        exit;
	            }
		    $event_name = &trim($event->getFirstChild()->getData());
	    	    $event_name =~ tr/a-z/A-Z/;
		    $event_name_new = "$event_name_prefix"."$event_name"."$event_name_suffix";
		    $num_event_name += 1;
		    $num_event_attr += 1;
		    if ($str_event_name =~ / $event_name_new /) { 
			    print "Duplicate event name: $event_name. Please correct errors\n";
			    exit;
		    }
		    #$str_event_name .= "#define ".$event_name_new."    ".hex($num_event_name)."\n";
		    $str_event_name .= "#define ".$event_name_new."    ".$num_event_name."\n";
		    last SWITCH;
		}
	    	if ($event->getNodeName() eq "event_severity") {
		    my $event_severity = &trim($event->getFirstChild()->getData());
		    $event_severity =~ tr/a-z/A-Z/;
		    $event_severity_new = $event_severity;
		    $num_event_attr += 1; 
		    if ($str_event_severity =~ / $event_severity_new /) { 
		    	last SWITCH;
		    }
		    else {
			#This severity string does not match the predefined ones
			print "Severity value $event_severity does not match predefined severity values. Please correct errors\n";
			exit;
		    }
		    last SWITCH;
		}
		if ($event->getNodeName() eq "event_category" ) {
		    my $event_cat = &trim($event->getFirstChild()->getData());
	    	    $event_cat =~ tr/a-z/A-Z/;
		    $event_cat_new = "$event_cat_prefix"."$event_cat";
		    $num_event_attr += 1 ;
		    if ($str_event_cat =~ / $event_cat_new /) { 
		    	last SWITCH;
		    }
	            $str_event_cat .= "#define ".$event_cat_new."     (1<<".$num_event_cat.")\n";
		    $num_event_cat += 1;
		    last SWITCH;
		}
	    }
	}
    }
    # Currently we are considering only 3 event attributes: event name, event severity and event category
    if ($num_event_attr != 3) {
        print "value = $num_event_attr. Event Attributes are missing for a throw event in $input_file.  Parser exiting.\n";
	exit;
    }
    else {
	$total_throw_events += 1;
	$str_struct .= "\{\"$event_name\", $event_name_new, $event_severity_new, $event_cat_new, $component_cat_new, $component_new\}, \n";
    }
}

# Subroutine to evaluate value contained in second-level ELEMENT_NODE
sub evaluate_element_node {
    my ($node, $input_file) = @_;
    my $component_cat_name = '';
    my $component_name = '';

    if (($node->getNodeName() ne "component_category") && ($component_cat_name_new eq "")) {
	    print "Component category not specified in the correct location in xml file: $input_file\n";
	    exit;
    }

    SWITCH: {
    	if ($node->getNodeName() eq "component_category") {
	    if ($num_component_cat == $max_num_component_cat) {
		    print "FTB cannot add more component categories. Max limit of ( $num_component_cat ) reached\n";
		    exit;
	    } 
	    $component_cat_name = &trim($node->getFirstChild()->getData());
	    $component_cat_name =~ tr/a-z/A-Z/;
	    $component_cat_name_new = "$component_cat_prefix"."$component_cat_name";
	    if ($str_component_cat =~ / $component_cat_name_new /) { 
                last SWITCH;
	    }
	    $str_component_cat .= "#define ".$component_cat_name_new.	
	    				"     (1<<".$num_component_cat.")\n";
	    $num_component{$component_cat_name_new} = 0;
	    $num_component_cat += 1;
	    if ($DEBUG) {
		print "Subroutine evaluate_element_node(): Found ",$node->getNodeName(),
				"=",$component_cat_name,"\n";
	    }
	    last SWITCH;
	}
    	if ($node->getNodeName() eq "component") {
	    if ($num_component{$component_cat_name_new} == $max_num_component) {
		    $_=$component_cat_name_new; s/$component_cat_prefix//;
		    print "FTB cannot add more components to $_ category. Max limit of ( $max_num_component ) reached\n";
		    exit;
	    }
	    $component_name = &trim($node->getFirstChild()->getData());
	    $component_name =~ tr/a-z/A-Z/;
	    $component_name_new = "$component_prefix"."$component_name";
	    if ($str_component =~ / $component_name_new /) { 
	        print "Duplicate component name: $component_name. Please correct errors\n"; 
		exit;
	    }
	    $str_component .= "#define ".$component_name_new."     (1<<".$num_component{$component_cat_name_new}.")\n";
	    $num_component{$component_cat_name_new} += 1;
	    #$num_event_name = 0;
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
            if ($component_name_new eq "") {
	      print "Component not specified in the correct location in xml file: $input_file\n";
	      exit;
            }
	    &evaluate_throw_event($node, $component_cat_name_new, $component_name_new, $input_file);
	    last SWITCH;
	}
	print "An unexpected tag called \'", $node->getNodeName(),
			"\' is present in one of the input files. Exiting..\n";
	exit;
    }
    # Below it the return value; 
    $node->getNodeName(); 
}


# Main section of the parser
# The parser is aware of the format and tree layout of the node in the XML file
my @input_files = @ARGV;
if ($#input_files == -1 ) {
    print "Usage:\nperl parser.pl file1.xml file2.xml ...\n";
    exit;
}

# &check_xml_files(@input_files);
foreach my $file (@input_files) {
    my $found = 0;  
    my $element_type = '';
    my $num_comp_attr = 0;
    my $doc = $parser->parsefile($file);
    $component_cat_name_new='';
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
		    $element_type = &evaluate_element_node($cnodes, $file);
		    if (($element_type eq 'component_category') || ($element_type eq 'component')) {
			    $num_comp_attr +=1;
		    }
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
    if ($num_comp_attr != 2) {
        print "Component and/or component category missing from $file. Parser exiting\n";
	exit;
    }
    if ($found != 1) {
	print "<ftb_component_details> tag not found in $file. Parser exiting\n";
	exit;
    }
    
} # done analyzing all input files

# Now write the data to the output file
$str_struct = "#define FTB_EVENT_DEF_TOTAL_THROW_EVENTS ".$total_throw_events."\n\nFTBM_throw_event_t FTB_event_table[".$total_throw_events."] = {".$str_struct."};";
my $start_data = "/* This file is automatically generated by the xmlparser. It will be \n".
			"used by the FTB framework to interpret components and events*/\n\n";
my $end_data = "";
&write_file("CREATE", $start_data."\n\n", $outfile);
&write_file("APPEND", $str_event_severity."\n\n", $outfile);
&write_file("APPEND", $str_component_cat."\n\n", $outfile);
&write_file("APPEND", $str_component."\n\n", $outfile);
&write_file("APPEND", $str_event_name."\n\n", $outfile);
&write_file("APPEND", $str_event_cat."\n\n", $outfile);
&write_file("APPEND", $str_struct."\n\n", $outfile);
&write_file("APPEND", $end_data."\n\n", $outfile);

# End of the Parser
