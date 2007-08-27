#!/usr/bin/perl -w

use strict;
use XML::DOM;

my $outfile = "ftb_throw_events.h";
my $parser  = new XML::DOM::Parser();

# Declare the Prefixes for the event and component attributes
my $component_ctgy_prefix  = 'FTB_EVENT_DEF_COMP_CTGY_';
my $component_prefix       = 'FTB_EVENT_DEF_COMP_';
my $event_severity_prefix  = 'FTB_EVENT_DEF_SEVERITY_';
my $event_ctgy_prefix      = 'FTB_EVENT_DEF_EVENT_CTGY_';
my $event_name_prefix      = 'FTB_EVENT_DEF_';
my $event_name_suffix      = '_CODE';

# Declare the global variables
#  The below variabes will function as counters.
my $num_component_ctgy = 0;
my $num_component = 0;
my $num_event_ctgy = 0;
my $num_event_severity = 0;
my $num_event_name = 0;

# The below variables will function as string for holding the data to be written to output file
my $str_component_ctgy = "/*Definition of component categories*/\n";
my $str_component = "/*Definition of components*/\n";
my $str_event_ctgy = "/*Definition of event categories*/\n";
my $str_event_severity = "/*Definition of event severity*/\n";
my $str_event_name = "/*Definition of event name*/\n";
my $str_struct = '';

# The below variables will fucntion as temporary variables
my $component_ctgy_name_new = '';
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
    my ($node, $component_ctgy_new, $component_new, $input_file) = @_;
    my $num_event_attr = 0;
    my $event_name_new = '';
    my $event_severity_new = '';
    my $event_ctgy_new = '';
    my $event_name = '';
    foreach my $event ($node->getChildNodes()) {
	if ($event->getNodeType() eq ELEMENT_NODE) {
	    SWITCH: {
		if ($event->getNodeName() eq "event_name") {
		    $event_name = &trim($event->getFirstChild()->getData());
	    	    $event_name =~ tr/a-z/A-Z/;
		    $event_name_new = "$event_name_prefix"."$event_name"."$event_name_suffix";
		    $num_event_name += 1;
		    $num_event_attr += 1;
		    if ($str_event_name =~ / $event_name_new /) { 
			    die "Duplicate event name: $event_name. Please correct errors\n"; 
		    }
		    #$str_event_name .= "#define ".$event_name_new."    ".hex($num_event_name)."\n";
		    $str_event_name .= "#define ".$event_name_new."    ".$num_event_name."\n";
		    last SWITCH;
		}
	    	if ($event->getNodeName() eq "event_severity") {
		    my $event_severity = &trim($event->getFirstChild()->getData());
	    	    $event_severity =~ tr/a-z/A-Z/;
		    $event_severity_new = "$event_severity_prefix"."$event_severity";
		    $num_event_attr += 1; 
		    if ($str_event_severity =~ / $event_severity_new /) { 
		    	last SWITCH;
		    }
		    $str_event_severity .= "#define ".$event_severity_new."    (1<<".$num_event_severity.")\n";
		    $num_event_severity += 1;
		    last SWITCH;
		}
		if ($event->getNodeName() eq "event_category" ) {
		    my $event_ctgy = &trim($event->getFirstChild()->getData());
	    	    $event_ctgy =~ tr/a-z/A-Z/;
		    $event_ctgy_new = "$event_ctgy_prefix"."$event_ctgy";
		    $num_event_attr += 1 ;
		    if ($str_event_ctgy =~ / $event_ctgy_new /) { 
		    	last SWITCH;
		    }
	            $str_event_ctgy .= "#define ".$event_ctgy_new."     (1<<".$num_event_ctgy.")\n";
		    $num_event_ctgy += 1;
		    last SWITCH;
		}
	    }
	}
    }
    # Currently we are considering only 3 event attributes: event name, event severity and event category
    if ($num_event_attr != 3) {
        die "value = $num_event_attr. Event Attributes are missing for a throw event in $input_file.  Parser exiting.\n"
    }
    else {
	$total_throw_events += 1;
	$str_struct .= "\{\"$event_name\", $event_name_new, $event_severity_new, $event_ctgy_new, $component_ctgy_new, $component_new\}, \n";
    }
}

# Subroutine to evaluate value contained in second-level ELEMENT_NODE
sub evaluate_element_node {
    my ($node, $input_file) = @_;
    my $component_ctgy_name = '';
    my $component_name = '';
    SWITCH: {
    	if ($node->getNodeName() eq "component_category") {
	    $component_ctgy_name = &trim($node->getFirstChild()->getData());
	    $component_ctgy_name =~ tr/a-z/A-Z/;
	    $component_ctgy_name_new = "$component_ctgy_prefix"."$component_ctgy_name";
	    if ($str_component_ctgy =~ / $component_ctgy_name_new /) { 
                last SWITCH;
	    }
	    $str_component_ctgy .= "#define ".$component_ctgy_name_new.	
	    				"     (1<<".$num_component_ctgy.")\n";
	    $num_component_ctgy += 1;
	    if ($DEBUG) {
		print "Subroutine evaluate_element_node(): Found ",$node->getNodeName(),
				"=",$component_ctgy_name,"\n";
	    }
	    last SWITCH;
	}
    	if ($node->getNodeName() eq "component") {
	    $component_name = &trim($node->getFirstChild()->getData());
	    $component_name =~ tr/a-z/A-Z/;
	    $component_name_new = "$component_prefix"."$component_name";
	    if ($str_component =~ / $component_name_new /) { 
	        die "Duplicate component name: $component_name. Please correct errors\n"; 
	    }
	    $str_component .= "#define ".$component_name_new."     (1<<".$num_component.")\n";
	    $num_component += 1;
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
	    &evaluate_throw_event($node, $component_ctgy_name_new, $component_name_new, $input_file);
	    last SWITCH;
	}
	die "An unexpected tag called \'", $node->getNodeName(),
			"\' is present in one of the input files. Exiting..\n";
    }
    # Below it the return value; 
    $node->getNodeName(); 
}


# Main section of the parser
# The parser is aware of the format and tree layout of the node in the XML file
my @input_files = @ARGV;
if ($#input_files == -1 ) {
    die "Usage:\nperl parser.pl file1.xml file2.xml ...\n";
}

# &check_xml_files(@input_files);
foreach my $file (@input_files) {
    my $found = 0;  
    my $element_type = '';
    my $num_comp_attr = 0;
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
        die "Component and/or component category missing from $file. Parser exiting\n";
    }
    if ($found != 1) {
	die "<ftb_component_details> tag not found in $file. Parser exiting\n";
    }
    
} # done analyzing all input files

# Now write the data to the output file
$str_struct = "#define FTB_EVENT_DEF_TOTAL_THROW_EVENTS ".$total_throw_events."\n\nFTBM_throw_event_t FTB_event_table[".$total_throw_events."] = {".$str_struct."};";
my $start_data = "/* This file is automatically generated by the xmlparser. It will be \n".
			"used by the FTB framework to interpret components and events*/\n";
my $end_data = "";
&write_file("CREATE", $start_data."\n\n", $outfile);
&write_file("APPEND", $str_component_ctgy."\n\n", $outfile);
&write_file("APPEND", $str_component."\n\n", $outfile);
&write_file("APPEND", $str_event_name."\n\n", $outfile);
&write_file("APPEND", $str_event_ctgy."\n\n", $outfile);
&write_file("APPEND", $str_event_severity."\n\n", $outfile);
&write_file("APPEND", $str_struct."\n\n", $outfile);
&write_file("APPEND", $end_data."\n\n", $outfile);

# End of the Parser
