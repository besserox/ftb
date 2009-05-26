package ftb.client;

import java.util.*;
import java.net.*;
import java.io.*;

class FTB_ID implements FTB_Exchangeable
{
    FTB_Location_ID location_id;
    FTB_Client_ID   client_id;

    public FTB_ID(FTB_Location_ID location_id, FTB_Client_ID client_id)
    {
	this.location_id = location_id;
	this.client_id = client_id;
    }

    public FTB_ID(byte[] buffer, int pos) 
    {
	location_id = new FTB_Location_ID(buffer, pos);
	pos += FTB_Location_ID.length();
	//	pos += 3;
	client_id = new FTB_Client_ID(buffer, pos);
    }

    static int length()
    {
	return FTB_Location_ID.length() + 3 + FTB_Client_ID.length();
    }

    public void writeBytes(byte[] template, int pos)
    {
	location_id.writeBytes(template, pos);
	pos += FTB_Location_ID.length();
	//	pos += 3;
	client_id.writeBytes(template, pos);
    }

    public String toString()
    {
	return client_id.toString();
    }
}
