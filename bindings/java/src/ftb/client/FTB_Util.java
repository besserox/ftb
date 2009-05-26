package ftb.client;

import java.util.*;
import java.net.*;
import java.io.*;
import java.text.DateFormat;

class FTB_Util
{
    public static short readShort(byte[] buffer, int offset)
    {
	short number = 
	    (short) ( (buffer[offset] & 0xff ) |
		      ((buffer[offset + 1] & 0xff ) << 8 ) );

	return number;
    }

    public static int readInt(byte[] buffer, int offset)
    {
	int number = 
	    (buffer[offset] & 0xff ) |
	    (buffer[offset + 1] & 0xff ) << 8 |
	    (buffer[offset + 2] & 0xff ) << 16 |
	    (buffer[offset + 3] & 0xff ) << 24;

	return number;
    }

    public static String readString(byte[] buffer, int offset, int max)
    {
	int i;
	String string = null;
	
	for (i=0; i<max; i++) {
	    if ( buffer[offset + i] == 0 ) 
		break;
	}

	try {
	    string = new String(buffer, offset, i, "ASCII");
	}
	catch ( UnsupportedEncodingException uee ) {
	    uee.printStackTrace();
	}

	return string;
    }

    public static void writeShort(byte[] buffer, int offset, short number) 
    {
	buffer[offset]     = (byte) ( number & 0x00ff);
	buffer[offset + 1] = (byte) ((number & 0xff00) >> 8);
    }

    public static void writeInt(byte[] buffer, int offset, int number) 
    {
	buffer[offset]     = (byte) ( number & 0x000000ff);
	buffer[offset + 1] = (byte) ((number & 0x0000ff00) >> 8);
	buffer[offset + 2] = (byte) ((number & 0x00ff0000) >> 16);
	buffer[offset + 3] = (byte) ((number & 0xff000000) >> 24);
    }

    public static String resolve_location_id()
    {
	String hostname = null;

	try {
	    InetAddress localMachine = InetAddress.getLocalHost();
	    hostname = localMachine.getHostName();

	}
	catch (java.net.UnknownHostException uhe) { 
	    uhe.printStackTrace();
	    System.exit(-1);
	}

	return hostname;
    }

    public static String resolve_starttime()
    {
	return   DateFormat.getDateTimeInstance(DateFormat.SHORT,DateFormat.MEDIUM).format(new Date());
    }
    
}
