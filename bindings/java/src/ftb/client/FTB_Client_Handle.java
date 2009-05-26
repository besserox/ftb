package ftb.client;

import java.util.*;
import java.net.*;
import java.io.*;

public class FTB_Client_Handle
{
    private FTB_Client client;

    public FTB_Client_Handle(FTB_Client client)
    {
	this.client = client;
    }

    FTB_Client client()
    {
	return client;
    }
}
