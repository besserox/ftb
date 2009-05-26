package ftb.client;

import java.util.*;
import java.net.*;
import java.io.*;

class FTB_Message implements FTB_Exchangeable
{
    public static final int FTBM_MSG_TYPE_NOTIFY       = 0x01;
    public static final int FTBM_MSG_TYPE_CLIENT_REG   = 0x11;
    public static final int FTBM_MSG_TYPE_CLIENT_DEREG = 0x12;

    public static final int FTBM_MSG_TYPE_REG_SUBSCRIPTION = 0x22;
    public static final int FTBM_MSG_TYPE_SUBSCRIPTION_CANCEL = 0x24;

    int msg_type;
    FTB_ID src;
    FTB_ID dst;
    FTB_Event  event;
    

    FTB_Message(int type, FTB_ID src, FTB_ID dst, FTB_Event event)
    {
	this.msg_type  = type;
	this.src   = src;
	this.dst   = dst;
	this.event = event;
    }

    FTB_Message(byte[] input) 
    {
	int pos = 0;

	msg_type = FTB_Util.readInt(input, pos);
	pos += 4;

	src = new FTB_ID(input, pos);
	pos += FTB_ID.length();

	dst = new FTB_ID(input,pos);
	pos += FTB_ID.length();

	event = new FTB_Event(input, pos);
    }

    public void writeBytes(byte[] template, int pos)
    {
	int offset = pos;

	FTB_Util.writeInt(template, offset, msg_type);
	offset += 4;
	src.writeBytes(template, offset);
	offset += FTB_ID.length();

	dst.writeBytes(template, offset);
	offset += FTB_ID.length();

	if ( event != null ) 
	    event.writeBytes(template, offset);

	/*
	FTB_Util.writeInt(template, pos, msg_type);
	src.writeBytes(template, pos+4);
	dst.writeBytes(template, pos+4+FTB_ID.length());
	event.writeBytes(template, pos+4+FTB_ID.length()*2);
	*/
    }

    static int length()
    {
	return 4 + FTB_ID.length()*2 + FTB_Event.length();
    }

    void print()
    {
	System.out.println("type: " + msg_type);
	System.out.println("source: " + src.location_id.hostname);


	System.out.println("src location_id hostname: " + src.location_id.hostname);
	System.out.println("src location_id FTB_pid_starttime_t: " + src.location_id.FTB_pid_starttime_t);
	System.out.println("src location_id pid: " + src.location_id.pid);

	System.out.println("src client_id region: " + src.client_id.region);
	System.out.println("src client_id comp_cat: " + src.client_id.comp_cat);
	System.out.println("src cilent_id comp: " + src.client_id.comp);
	System.out.println("src cilent_id client_name: " + src.client_id.client_name);
	System.out.println("src cilent_id ext: " + src.client_id.ext);

	System.out.println("dst location_id hostname: " + dst.location_id.hostname);
	System.out.println("dst location_id FTB_pid_starttime_t: " + dst.location_id.FTB_pid_starttime_t);
	System.out.println("dst location_id pid: " + dst.location_id.pid);

	System.out.println("dst client_id region: " + dst.client_id.region);
	System.out.println("dst client_id comp_cat: " + dst.client_id.comp_cat);
	System.out.println("dst cilent_id comp: " + dst.client_id.comp);
	System.out.println("dst cilent_id client_name: " + dst.client_id.client_name);
	System.out.println("dst cilent_id ext: " + dst.client_id.ext);

	System.out.println("region: " + event.region);
	System.out.println("comp_cat: " + event.comp_cat);
	System.out.println("comp: " + event.comp);
	System.out.println("event_name: " + event.event_name);
	System.out.println("severity: " + event.severity);
	System.out.println("hostname: " + event.hostname);
    }
}

