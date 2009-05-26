package ftb.client;

import java.io.*;
import java.util.*;

public class FTB_Subscription_Parser
{
    String region;
    String comp_cat;
    String comp;
    String event_name;
    String hostname;
    String client_name;
    String client_jobid;
    String severity;

    public FTB_Subscription_Parser()
    {
	reset_attributes();
    }

    private void reset_attributes()
    {
	region = "all";
	comp_cat = "all";
	comp = "all";
	event_name = "all";
	severity = "all";
	client_jobid = "all";
	hostname = "all";
	client_name = "all";
    }

    public int parse_subscription_string(String subscription_str)
    {
	String[] field = subscription_str.split("\\s*\\,\\s*");
	reset_attributes();
	
	for (int i=0; i<field.length; i++) {
	    int resp;
	    if ( (resp = assign_attribute(field[i])) != FTB_DEF.FTB_SUCCESS ) {
		return resp;
	    }
	}

	return FTB_DEF.FTB_SUCCESS;
    }

    public String get_subscription_key()
    {
	String key = null;
	
	if ( region.equals("all") )
	    key = "\\w*";
	else    
	    key = region;

	key += "\\.";

	if ( comp_cat.equals("all") ) 
	    key += "\\w*";
	else 
	    key += comp_cat;

	key += "\\.";

	if ( comp.equals("all") ) 
	    key += "\\w*";
	else 
	    key += comp;

	key += "\\.";

	if ( event_name.equals("all") )
	    key += "\\w*";
	else 
	    key += event_name;

	key += "\\.";

	if ( severity.equals("all") )
	    key += "\\w*";
	else 
	    key += severity;

	return key;
    }

    private int assign_event_space(String entry) 
    {
	String[] space = entry.split("\\.");
	
	if ( space[0].matches("[a-zA-Z0-9_]+") &&
	     space[1].matches("[a-zA-Z0-9_]+") &&
	     space[2].matches("[a-zA-Z0-9_]+") ) {
	    region   = space[0].toUpperCase();
	    comp_cat = space[1].toUpperCase();
	    comp     = space[2].toUpperCase();
	    return FTB_DEF.FTB_SUCCESS;
	}

	return FTB_DEF.FTB_ERR_FILTER_VALUE;
    }

    private int assign_attribute(String entry)
    {
	String[] pair = entry.split("\\s*=\\s*");

	if ( pair[0].compareTo("event_space") == 0 ) {
	    return assign_event_space(pair[1]);
	}
	else if ( pair[0].compareTo("comp_cat") == 0 ) {
	    if ( pair[1].matches("[a-zA-Z0-9_]+") ) {
		comp_cat = pair[1].toUpperCase();
	    }
	    else {
		return FTB_DEF.FTB_ERR_FILTER_VALUE;
	    }
	}
	else if ( pair[0].compareTo("comp") == 0 ) {
	    if ( pair[1].matches("[a-zA-Z0-9_]+") ) {
		comp = pair[1].toUpperCase();
	    }
	    else {
		return FTB_DEF.FTB_ERR_FILTER_VALUE;
	    }
	}
	else if ( pair[0].compareTo("event_name") == 0 ) {
	    if ( pair[1].matches("[a-zA-Z0-9_]+") ) {
		event_name = pair[1].toUpperCase();
	    }
	    else {
		return FTB_DEF.FTB_ERR_FILTER_VALUE;
	    }
	}
	else if ( pair[0].compareTo("severity") == 0 ) {
	    if ( pair[1].matches("[a-zA-Z0-9_]+") ) {
		severity = pair[1].toUpperCase();
	    }
	    else {
		return FTB_DEF.FTB_ERR_FILTER_VALUE;
	    }
	}
	else if ( pair[0].compareTo("client_name") == 0 ) {
	    if ( pair[1].matches("[a-zA-Z0-9_]+") ) {
		client_name = pair[1].toUpperCase();
	    }
	    else {
		return FTB_DEF.FTB_ERR_FILTER_VALUE;
	    }
	}
	else if ( pair[0].compareTo("client_jobid") == 0 ) {
	    if ( pair[1].matches("[a-zA-Z0-9_]+") ) {
		client_jobid = pair[1].toUpperCase();
	    }
	    else {
		return FTB_DEF.FTB_ERR_FILTER_VALUE;
	    }
	}
	else if ( pair[0].compareTo("hostname") == 0 ) {
	    if ( pair[1].matches("[a-zA-Z0-9_]+") ) {
		hostname = pair[1].toUpperCase();
	    }
	    else {
		return FTB_DEF.FTB_ERR_FILTER_VALUE;
	    }
	}
	else {
	    return FTB_DEF.FTB_ERR_FILTER_ATTR;
	}

	return FTB_DEF.FTB_SUCCESS;
    }
}
