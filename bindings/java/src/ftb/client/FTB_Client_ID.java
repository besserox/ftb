package ftb.client;

import java.util.*;
import java.net.*;
import java.io.*;

class FTB_Client_ID implements FTB_Exchangeable
{
    String region;
    String comp_cat;
    String comp;
    String client_name;
    byte ext;

    FTB_Client_ID(FTB_Client_Info client_info)
    {
	split_eventspace(client_info.event_space);
	this.client_name = client_info.client_name;
	this.ext = 0; //HOONY :)~~~ should check later!!!
    }

    FTB_Client_ID(String event_space, String client_name, byte ext)
    {
	split_eventspace(event_space);
	this.client_name = client_name;
	this.ext = ext;
    }

    FTB_Client_ID(byte[] buffer, int pos)
    {
	region = FTB_Util.readString(buffer,pos,FTB_DEF.FTB_MAX_EVENTSPACE);
	pos += FTB_DEF.FTB_MAX_EVENTSPACE;

	comp_cat = FTB_Util.readString(buffer,pos,FTB_DEF.FTB_MAX_EVENTSPACE);
	pos += FTB_DEF.FTB_MAX_EVENTSPACE;

	comp = FTB_Util.readString(buffer,pos,FTB_DEF.FTB_MAX_EVENTSPACE);
	pos  += FTB_DEF.FTB_MAX_EVENTSPACE;

	client_name = FTB_Util.readString(buffer,pos,FTB_DEF.FTB_MAX_CLIENT_NAME);
	pos += FTB_DEF.FTB_MAX_CLIENT_NAME;

	ext = buffer[pos];
    }

    public String toString()
    {
	return "region: " + region + "\n" + "comp_cat: " + comp_cat + "\n" + "comp: " + comp + "\n" + 
	    "client_name: " + client_name + "\n" + "ext: " + ext + "\n";
    }

    boolean equals(FTB_Client_ID client_id)
    {
	return 
	    region.equals(client_id.region) &&
	    comp_cat.equals(client_id.comp_cat) &&
	    comp.equals(client_id.comp) &&
	    client_name.equals(client_id.client_name) &&
	    (ext == client_id.ext);
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

    public static int length()
    {
	return  FTB_DEF.FTB_MAX_EVENTSPACE*3 + FTB_DEF.FTB_MAX_CLIENT_NAME + 1;
    }

    public void writeBytes(byte[] template, int pos)
    {
	int offset = pos;
	try {
	    byte[] buffer = region.getBytes("ASCII");
	    System.arraycopy(buffer, 0, template, offset, buffer.length);
	    offset += FTB_DEF.FTB_MAX_EVENTSPACE;

	    buffer = comp_cat.getBytes("ASCII");
	    System.arraycopy(buffer, 0, template, offset, buffer.length);
	    offset += FTB_DEF.FTB_MAX_EVENTSPACE;

	    buffer = comp.getBytes("ASCII");
	    System.arraycopy(buffer, 0, template, offset, buffer.length);
	    offset += FTB_DEF.FTB_MAX_EVENTSPACE;

	    buffer = client_name.getBytes("ASCII");
	    System.arraycopy(buffer, 0, template, offset, buffer.length);
	    offset += FTB_DEF.FTB_MAX_CLIENT_NAME;

	    template[offset] = ext;;
	}
	catch (java.io.UnsupportedEncodingException uee) {
	    uee.printStackTrace();
	}
    }

}
