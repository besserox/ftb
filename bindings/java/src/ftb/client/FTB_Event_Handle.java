package ftb.client;

import java.io.*;

public class FTB_Event_Handle implements FTB_Exchangeable
{
    String event_name;
    String severity;
    FTB_Client_ID client_id;
    short seqnum;
    FTB_Location_ID location_id;

    public FTB_Event_Handle(String event_name, String severity, 
			    FTB_Client_ID client_id, short seqnum, 
			    FTB_Location_ID location_id) 
    {
	this.event_name = event_name;
	this.severity = severity;
	this.client_id = client_id;
	this.seqnum = seqnum;
	this.location_id = location_id;
    }

    public FTB_Event_Handle(byte[] buffer, int pos)
    {
	event_name = FTB_Util.readString(buffer,pos,FTB_DEF.FTB_MAX_EVENT_NAME);
	pos += FTB_DEF.FTB_MAX_EVENT_NAME;

	severity = FTB_Util.readString(buffer,pos,FTB_DEF.FTB_MAX_SEVERITY);
	pos += FTB_DEF.FTB_MAX_SEVERITY;

	client_id = new FTB_Client_ID(buffer, pos);
	pos += FTB_Client_ID.length();

	pos++; // Padding ???

	seqnum = FTB_Util.readShort(buffer, pos);
	pos += 2;

	location_id = new FTB_Location_ID(buffer, pos);
    }


    public String toString()
    {
	return "event_name: " + event_name + "\n" 
	    +  "severity: " + severity + "\n"
	    +  client_id.toString() 
	    +  "seqnum: " + seqnum + "\n"
	    +  location_id.toString();
    }

    public static int length()
    {
	return 
	    FTB_DEF.FTB_MAX_EVENT_NAME + FTB_DEF.FTB_MAX_CLIENT_NAME 
	    + FTB_Client_ID.length() + 2 + FTB_Location_ID.length() + 1; // one extra for padding
    }

    public void writeBytes(byte[] template, int pos)
    {
	try {
	    byte[] buffer = event_name.getBytes("ASCII");
	    System.arraycopy(buffer, 0, template, pos, buffer.length);
	    pos += FTB_DEF.FTB_MAX_EVENT_NAME;

	    buffer = severity.getBytes("ASCII");
	    System.arraycopy(buffer, 0, template, pos, buffer.length);
	    pos += FTB_DEF.FTB_MAX_SEVERITY;;

	    client_id.writeBytes(template, pos);
	    pos += FTB_Client_ID.length();
	    
	    FTB_Util.writeShort(template, pos, seqnum);
	    pos += 2;

	    pos++; // Padding
	    location_id.writeBytes(template, pos);
	}
	catch (java.io.UnsupportedEncodingException uee) {
	    uee.printStackTrace();
	}
    }
}
