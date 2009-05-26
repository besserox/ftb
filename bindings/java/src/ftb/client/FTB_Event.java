package ftb.client;

import java.util.*;
import java.net.*;
import java.io.*;

class FTB_Event implements FTB_Exchangeable 
{
    String region;
    String comp_cat;
    String comp;
    String event_name;
    String severity;
    String client_jobid;
    String client_name;
    String hostname;
    short seqnum;
    byte event_type;
    byte[] event_payload = null;

    FTB_Event(FTB_Client_Info client_info) 
    {
	split_eventspace(client_info.event_space);
	this.event_name = "";
	this.client_name = client_info.client_name;
	this.client_jobid = client_info.client_jobid;
	this.severity = "";
	this.hostname = client_info.hostname;
	this.seqnum = 0; 	// arbitrary number
	this.event_type = 0;	// arbitrary number
	//	event_payload = new byte[FTB_DEF.FTB_MAX_PAYLOAD_DATA];
    }

    FTB_Event(String region, String comp_cat, String comp,
		     String event_name, String severity, String client_jobid,
		     String client_name, String hostname, short seqnum,
		     byte event_type)
    {
	this.region = region;
	this.comp_cat = comp_cat;
	this.comp = comp;
	this.event_name = event_name;
	this.severity = severity;
	this.client_jobid = client_jobid;
	this.client_name = client_name;
	this.hostname = hostname;
	this.seqnum = seqnum;
	this.event_type = event_type;
	//	event_payload = new byte[FTB_DEF.FTB_MAX_PAYLOAD_DATA];
    }

    FTB_Event(byte[] buffer, int pos)
    {
	event_payload = new byte[FTB_DEF.FTB_MAX_PAYLOAD_DATA];

	region = FTB_Util.readString(buffer,pos,FTB_DEF.FTB_MAX_EVENTSPACE);
	pos += FTB_DEF.FTB_MAX_EVENTSPACE;

	comp_cat = FTB_Util.readString(buffer,pos,FTB_DEF.FTB_MAX_EVENTSPACE);
	pos += FTB_DEF.FTB_MAX_EVENTSPACE;

	comp = FTB_Util.readString(buffer,pos,FTB_DEF.FTB_MAX_EVENTSPACE);
	pos += FTB_DEF.FTB_MAX_EVENTSPACE;
	
	event_name = FTB_Util.readString(buffer,pos,FTB_DEF.FTB_MAX_EVENT_NAME);
	pos += FTB_DEF.FTB_MAX_EVENT_NAME;

	severity = FTB_Util.readString(buffer,pos,FTB_DEF.FTB_MAX_SEVERITY);
	pos += FTB_DEF.FTB_MAX_SEVERITY;

	client_jobid = FTB_Util.readString(buffer,pos, FTB_DEF.FTB_MAX_CLIENT_JOBID);
	pos += FTB_DEF.FTB_MAX_CLIENT_JOBID;

	client_name = FTB_Util.readString(buffer,pos,FTB_DEF.FTB_MAX_CLIENT_NAME);	
	pos += FTB_DEF.FTB_MAX_CLIENT_NAME;

	hostname = FTB_Util.readString(buffer,pos, FTB_DEF.FTB_MAX_HOST_ADDR);
	pos += FTB_DEF.FTB_MAX_HOST_ADDR;

	seqnum = FTB_Util.readShort(buffer, pos);
	pos+=2;

	event_type = buffer[pos];
	// pos += 2; ???
	pos += 1;

	System.arraycopy(buffer, pos, event_payload, 0, event_payload.length);
    }

    private void split_eventspace(String space) 
    {
	String[] token = space.split("\\.");
	if ( token.length != 3 ) {
	    System.out.println("oops");
	    System.exit(-1);
	}

	region   = token[0];
	comp_cat = token[1];
	comp     = token[2];
    }

    void set_eventname(String name)
    {
	this.event_name = name;
    }

    void set_severity(String severity)
    {
	this.severity = severity;
    }
    
    void set_seqnum(short seqnum) 
    {
	this.seqnum = seqnum;
    }

    void set_eventtype(byte type)
    {
	this.event_type = type;
    }

    void set_payload(byte[] payload)
    {
	event_payload = payload;
    }

    public void writeBytes(byte[] template, int pos)
    {
	try {
	    byte[] buffer = region.getBytes("ASCII");
	    System.arraycopy(buffer, 0, template, pos, buffer.length);
	    pos += FTB_DEF.FTB_MAX_EVENTSPACE;

	    buffer = comp_cat.getBytes("ASCII");
	    System.arraycopy(buffer, 0, template, pos, buffer.length);
	    pos += FTB_DEF.FTB_MAX_EVENTSPACE;

	    buffer = comp.getBytes("ASCII");
	    System.arraycopy(buffer, 0, template, pos, buffer.length);
	    pos += FTB_DEF.FTB_MAX_EVENTSPACE;

	    buffer = event_name.getBytes("ASCII");
	    System.arraycopy(buffer, 0, template, pos, buffer.length);
	    pos += FTB_DEF.FTB_MAX_EVENT_NAME;

	    buffer = severity.getBytes("ASCII");
	    System.arraycopy(buffer, 0, template, pos, buffer.length);
	    pos += FTB_DEF.FTB_MAX_SEVERITY;

	    buffer = client_jobid.getBytes("ASCII");
	    System.arraycopy(buffer, 0, template, pos, buffer.length);
	    pos += FTB_DEF.FTB_MAX_CLIENT_JOBID;

	    buffer = client_name.getBytes("ASCII");
	    System.arraycopy(buffer, 0, template, pos, buffer.length);
	    pos += FTB_DEF.FTB_MAX_CLIENT_NAME;

	    buffer = hostname.getBytes("ASCII");
	    System.arraycopy(buffer, 0, template, pos, buffer.length);
	    pos += FTB_DEF.FTB_MAX_HOST_ADDR;

	    FTB_Util.writeShort(template, pos, seqnum);
	    pos += 2;
	    
	    template[pos] = event_type;

	    //	    pos += 2; // ??? This is my wild guess!!!
	    pos += 1; // ??? This is my wild guess!!!

	    if ( event_payload != null ) 
		System.arraycopy(event_payload, 0, template, pos, event_payload.length);
	}
	catch (java.io.UnsupportedEncodingException uee) {
	    uee.printStackTrace();
	}
	
    }

    static int length()
    {
	return 
	    FTB_DEF.FTB_MAX_EVENTSPACE +
	    FTB_DEF.FTB_MAX_EVENTSPACE +
	    FTB_DEF.FTB_MAX_EVENTSPACE +
	    FTB_DEF.FTB_MAX_EVENT_NAME +
	    FTB_DEF.FTB_MAX_SEVERITY   +
	    FTB_DEF.FTB_MAX_CLIENT_JOBID +
	    FTB_DEF.FTB_MAX_CLIENT_NAME +
	    FTB_DEF.FTB_MAX_HOST_ADDR + 2 + 2 +
	    FTB_DEF.FTB_MAX_PAYLOAD_DATA;
    }
}