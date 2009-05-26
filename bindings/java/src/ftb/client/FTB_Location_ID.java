package ftb.client;

import java.util.*;
import java.net.*;
import java.io.*;

public class FTB_Location_ID implements FTB_Exchangeable
{
    public String hostname;
    public String FTB_pid_starttime_t;
    public int pid;

    public FTB_Location_ID(String hostname, String FTB_pid_starttime_t, int pid)
    {
	this.hostname = hostname;
	this.FTB_pid_starttime_t = FTB_pid_starttime_t;
	this.pid = pid;
    }

    public FTB_Location_ID(byte[] buffer, int pos)
    {
	hostname = FTB_Util.readString(buffer,pos,FTB_DEF.FTB_MAX_HOST_ADDR);
	pos += FTB_DEF.FTB_MAX_HOST_ADDR;

	FTB_pid_starttime_t = FTB_Util.readString(buffer,pos,FTB_DEF.FTB_MAX_PID_TIME);
	pos += FTB_DEF.FTB_MAX_PID_TIME;;

	pid = FTB_Util.readInt(buffer, pos);
    }
    
    public boolean equals(FTB_Location_ID location_id)
    {
	return
	    hostname.equals(location_id.hostname) &&
	    FTB_pid_starttime_t.equals(location_id.FTB_pid_starttime_t) &&
	    (pid == location_id.pid);
    }

    public void writeBytes(byte[] template, int pos) 
    {
	try {
	    byte[] buffer = hostname.getBytes("ASCII");
	    System.arraycopy(buffer, 0, template, pos, buffer.length);
	    pos += FTB_DEF.FTB_MAX_HOST_ADDR;

	    buffer = FTB_pid_starttime_t.getBytes("ASCII");
	    System.arraycopy(buffer, 0, template, pos, buffer.length);
	    pos += FTB_DEF.FTB_MAX_PID_TIME;

	    FTB_Util.writeInt(template, pos, pid);
	}
	catch (java.io.UnsupportedEncodingException uee) {
	    uee.printStackTrace();
	}
    }

    static int length()
    {
	return FTB_DEF.FTB_MAX_HOST_ADDR + FTB_DEF.FTB_MAX_PID_TIME + 4;
    }

    public String toString()
    {
	return "hostname: " + hostname
	    + "\nFTB_pid_starttime: " + FTB_pid_starttime_t + "\npid: " + pid + "\n";
    }
}
