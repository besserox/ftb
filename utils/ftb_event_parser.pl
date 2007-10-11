#!/usr/bin/perl -w

use strict;
use XML::DOM;

my $outfile = "ftb_throw_events.h";
my $parser  = new XML::DOM::Parser();

my $FTB_MAX_REGION = 16;
my $FTB_MAX_COMP_CAT = 32;
my $FTB_MAX_COMP = 32;
my $FTB_MAX_EVENT_NAME = 32;
my $FTB_MAX_SEVERITY = 32;
my $FTB_MAX_EVENT_DESC = 1024;

my $namespace = '';
my $comp_cat = '';
my $region  = '';
my $comp = '';

my $num_comp_cat = 2;
my $num_comp     = 2;
my $num_events = 1;
my $num_event_name = 1;
my $max_num_comp = 50;
my $max_num_comp_cat = 50;
my $max_num_events = 500;

my $event_name_prefix     = ''; 
my $event_name_suffix     = '_CODE';

my $comp_cat_def = "/* Definition of component categories */\n";
my $comp_cat_hash_def = "FTBM_comp_cat_entry_t FTB_comp_cat_table[FTB_TOTAL_COMP_CAT] = {\n";
my $comp_cat_rev_def = "char * FTB_comp_cat_table_rev[FTB_TOTAL_COMP_CAT + 1] = {\"\", \"\"";

my $comp_def = "/* Definition of components */\n";
my $comp_hash_def = "FTBM_comp_entry_t FTB_comp_table[FTB_TOTAL_COMP] = {\n";
my $comp_rev_def = "char * FTB_comp_table_rev[FTB_TOTAL_COMP + 1] = {\"\", \"\"";

my $sev_def = "/*Definition of event severities*/\n".
			"#define FTB_INFO  1\n#define FTB_FATAL 2\n".
			"#define FTB_ERROR 3\n#define FTB_PERF_WARNING 4\n#define FTB_WARNING 5\n\n".
			"#define FTB_TOTAL_SEVERITY  5\n";
my $sev_hash_def = "FTBM_severity_entry_t FTB_severity_table[FTB_TOTAL_SEVERITY] = {\n".
			"{\"FTB_INFO\", FTB_INFO},\n{\"FTB_FATAL\", FTB_FATAL},\n{\"FTB_ERROR\", FTB_ERROR},\n".
			"{\"FTB_PERF_WARNING\", FTB_PERF_WARNING},\n{\"FTB_WARNING\", FTB_WARNING}\n};\n";
my $sev_rev_def = "char * FTB_severity_table_rev[FTB_TOTAL_SEVERITY + 1] = ".
			"{\"\", \"FTB_INFO\", \"FTB_FATAL\", \"FTB_ERROR\", \"FTB_PERF_WARNING\", \"FTB_WARNING\"};\n";

my $event_name_def = "/* Definition of event names */\n";
my $event_name_rev_def = "char * FTB_event_table_rev[FTB_EVENT_DEF_TOTAL_THROW_EVENTS + 1] = {\"\"";
my $event_name_hash_def = "FTBM_throw_event_entry_t FTB_event_table[FTB_EVENT_DEF_TOTAL_THROW_EVENTS] = {\n";

my $event_cat_def = "/*Definition of placeholder event category*/\n #define FTB_EVENT_CAT_GENERAL 1\n";

my $user_struct = "";

# The below variables will fucntion as temporary variables
my $schema_ver_new ='';
my $namespace_new = '';
my $component_cat_name_new = '';
my $component_name_new = '';
my $DEBUG = 0;
my $total_comp_events;

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
sub evaluate_publish_event {
    my ($node, $comp_cat_new, $comp_new, $namespace, $input_file) = @_;
    my $num_event_attr = 0;
    my $event_name = '';
    my $event_name_new = '';
    my $event_sev_new = '';
    my $event_desc_new = '';
    
    foreach my $event ($node->getChildNodes()) {
	if ($event->getNodeType() eq ELEMENT_NODE) {
	    SWITCH: {
		if ($event->getNodeName() eq "event_name") {
		    $event_name = &trim($event->getFirstChild()->getData());
	    	    $event_name =~ tr/a-z/A-Z/;
		    $event_name_new = "$event_name_prefix"."$event_name"."$event_name_suffix";
		    $num_event_attr += 1;
		    if ($event_name_def =~ / $event_name_new /) { 
			    print "Duplicate event name: $event_name. Please correct errors\n";
			    exit;
		    }
		    $event_name_def .= "#define ".$event_name_new."    ".$num_event_name."\n";
		    $event_name_rev_def .= ",\"$event_name\"";
		    $num_event_name += 1;
		    last SWITCH;
		}
	    	if ($event->getNodeName() eq "event_severity") {
	            if ($event_name_new eq "") {
			    print "Event name not specified in correct location in xml file $input_file\n";
			    exit;
		    }
		    my $event_severity = &trim($event->getFirstChild()->getData());
		    $event_severity =~ tr/a-z/A-Z/;
		    $event_sev_new = $event_severity;
		    $num_event_attr += 1; 
		    if ($sev_def =~ / $event_sev_new /) { 
		    	last SWITCH;
		    }
		    else {
			print "Severity value $event_severity does not match predefined severity values. Please correct errors\n";
			exit;
		    }
		    last SWITCH;
		}
		if ($event->getNodeName() eq "event_desc" ) {
  		    if ($event_sev_new eq "") {
			    print "Event severity not specified at correct location in xml file $input_file\n";
			    exit;
		    }
		    my $event_desc = &trim($event->getFirstChild()->getData());
		    $event_desc_new = $event_desc;
		    $num_event_attr += 1 ;
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
	$event_name_hash_def .= "\{\"$event_name\", $event_name_new, $event_sev_new, FTB_EVENT_CAT_GENERAL, $comp_cat_new, $comp_new\}, \n";
	$user_struct .= "{\"$namespace\",\"$event_name_new\", \"$event_sev_new\", \"$event_desc_new\"},\n";
	$total_comp_events +=1;
    }
}

sub split_namespace {
	my ($temp_ns, $input_file) = @_;
	my @values = split('\\.', $temp_ns);
	if ((scalar(@values) != 3) || ($values[0] ne "FTB") || ($values[1] eq "") || ($values[2] eq "") || (length($values[0]) > ($FTB_MAX_REGION - 1)) || 
		(length($values[1]) > ($FTB_MAX_COMP_CAT - 1)) || (length($values[2]) > ($FTB_MAX_COMP -1)) ){
		print "Namespace needs to be of format region.comp_category.comp. Region for this release is FTB\n";
		exit;
	}
	$region = &trim($values[0]);
	$comp_cat = &trim($values[1]);
	$comp = &trim($values[2]);
	if ($DEBUG) {
		print "Namespace has been split into region=$region, component category=$comp_cat component=$comp\n";
	}
}

# Subroutine to evaluate value contained in second-level ELEMENT_NODE
sub evaluate_element_node {
    my ($node, $input_file) = @_;

    if (($node->getNodeName() ne "schema_ver") && ($schema_ver_new eq "")) {
	    print "Schema_ver not specified in the correct location in xml file: $input_file\n";
	    exit;
    }
   
    SWITCH: {
	if ($node->getNodeName() eq "schema_ver") {
		my $schema_ver = &trim($node->getFirstChild()->getData());
	    	if ($schema_ver ne "0.5") {
	 		print "Schema version should be 0.5 for this release. Ignoring XML schema version and setting it to 0.5\n";
	    	}
	    	$schema_ver_new="0.5";
	    	last SWITCH;
	}
	if ($node->getNodeName() eq "namespace") {
		$namespace = &trim($node->getFirstChild()->getData());
		$namespace =~ tr/a-z/A-Z/;
		$namespace_new = $namespace;
	        &split_namespace($namespace, $input_file);	
		print "Value of region $region , comp_cat $comp_cat and component is $comp\n";
		if ($num_comp_cat == $max_num_comp_cat) {
			print "FTB cannot add more component categories. Max limit of ( $num_comp_cat) reached\n";
			exit;
		}
		elsif ($num_comp == $max_num_comp) {
			print "FTB cannot add more components. Max limit of ( $num_comp ) reached\n";
			exit;
		}
		if ($comp_def =~ / $comp /) {
			print "Duplicate component in file $input_file for component $comp\n";
			exit;
		}
		else {
			$comp_def .= "#define $comp $num_comp\n";
			$comp_hash_def .= "{\"$comp\", $comp},\n";
			$comp_rev_def .= ",\"$comp\"";
			$num_comp += 1;
		}
		if ($comp_cat_def =~ / $comp_cat /) {
			last SWITCH;
		}
		else {
			$comp_cat_def .= "#define $comp_cat $num_comp_cat\n";
			$comp_cat_hash_def .= "{\"$comp_cat\", $comp_cat},\n";
			$comp_cat_rev_def .= ",\"$comp_cat\"";
			$num_comp_cat += 1;
		}
		last SWITCH;
	}
	if ($node->getNodeName() eq "publish_event") {
	    	if ($DEBUG) {
			print "Subroutine evaluate_element_node(): Found throw event. Further evaluating it.\n";
	    	}
		if ($num_events == $max_num_events) {
			printf "FTB cannot add more events. Max limit of ($num_events) reached\n";
			exit;
		}
		elsif ($namespace_new eq "") {
	      		print "Namespace not specified in the correct location in xml file: $input_file\n";
	      		exit;
            	}
	    	&evaluate_publish_event($node, $comp_cat, $comp, $namespace, $input_file);
		$num_events += 1;
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
    $schema_ver_new='';
    $region = "";
    $comp_cat = "";
    $comp = "";
    $total_comp_events = 0;
    $user_struct = '';
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
		    if (($element_type eq 'schema_ver') || ($element_type eq 'namespace')) {
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
    # Do you want to check if num_comp_attr ==2?
    if ($found != 1) {
	print "<ftb_component_details> tag not found in $file. Parser exiting\n";
	exit;
    }
    
#    $region =~ tr/A-Z/a-z/; $comp_cat =~ tr/A-Z/a-z/; $comp =~ tr/A-Z/a-z/;
#    my $comp_filename = "$region\_$comp_cat\_$comp\_publishevents.h";
#    &write_file("CREATE", "/* This file is automatically generated */\n\n", $comp_filename);
#    &write_file("APPEND", "#include \"ftb_def.h\"\n\n", $comp_filename);
#    my $user_def = "$region\_$comp_cat\_$comp\_TOTAL\_EVENTS";
#    $user_def =~ tr/a-z/A-Z/;
#    &write_file("APPEND", "#define $user_def $total_comp_events\n\n", $comp_filename);
#    &write_file("APPEND", "FTB_event_info_t $region\_$comp_cat\_$comp\_events[$user_def] = {\n", $comp_filename);
#    $user_struct .= "};";
#    &write_file("APPEND", $user_struct, $comp_filename);
    
    
} # done analyzing all input files

# Now write the data to the output file
my $start_data = "/* This file is automatically generated by the xmlparser. It will be \n".
			"used by the FTB framework to interpret components and events*/\n\n";
my $end_data = "\n/* End of file */";

$num_comp_cat -= 1;
$comp_cat_def .= "\n#define FTB_TOTAL_COMP_CAT  $num_comp_cat\n";
$comp_cat_hash_def .= "};\n";
$comp_cat_rev_def .= "};\n"; 

$num_comp -= 1;
$comp_def .= "\n#define FTB_TOTAL_COMP $num_comp\n";
$comp_hash_def .= "};\n";
$comp_rev_def .= "};\n";

$num_events -= 1;
$event_name_def .= "\n#define FTB_EVENT_DEF_TOTAL_THROW_EVENTS $num_events\n";
$event_name_hash_def .= "};\n";
$event_name_rev_def .= "};\n";


&write_file("CREATE", $start_data."\n\n", $outfile);
&write_file("APPEND", $sev_def."\n\n", $outfile);
&write_file("APPEND", $event_cat_def."\n\n", $outfile);
&write_file("APPEND", $comp_cat_def."\n\n", $outfile);
&write_file("APPEND", $comp_def."\n\n", $outfile);
&write_file("APPEND", $event_name_def."\n\n", $outfile);
&write_file("APPEND", $sev_hash_def."\n\n", $outfile);
&write_file("APPEND", $comp_cat_hash_def."\n\n", $outfile);
&write_file("APPEND", $comp_hash_def."\n\n", $outfile);
&write_file("APPEND", $event_name_hash_def."\n\n", $outfile);
&write_file("APPEND", $sev_rev_def."\n\n", $outfile);
&write_file("APPEND", $comp_cat_rev_def."\n\n", $outfile);
&write_file("APPEND", $comp_rev_def."\n\n", $outfile);
&write_file("APPEND", $event_name_rev_def."\n\n", $outfile);
# End of the Parser
