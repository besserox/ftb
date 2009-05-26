package ftb.client;

import java.net.*;
import java.util.*;
import java.text.DateFormat;

/**
 * Contains information of a FTB client. A client should create an
 * instance of FTB_Client_Info and use it to connect to FTB_World.
 * <p>
 * 
 *
 */
public class FTB_Client_Info
{
    public static final int FTB_CLIENT_EVENTSPACE         = 1;
    public static final int FTB_CLIENT_NAME               = 2;
    public static final int FTB_CLIENT_JOBID              = 3;
    public static final int FTB_CLIENT_SUBSCRIPTION_STYLE = 4;

    String event_space = "";
    String client_name = "";
    String client_jobid = "";
    String subscription_style = "";
    int    client_polling_queue_len = 10;

    String hostname;
    String FTB_pid_starttime_t;

    public FTB_Client_Info()
    {
	resolve_location_id();
    }

    /**
     * Sets an attribute of the client information.
     *
     * @param type type of information to set, i.e. one of the following four.
     *        <ul>
     *        <li>FTB_CLIENT_EVENT_SPACE</li>
     *        <li>FTB_CLIENT_NAME</li>
     *        <li>FTB_CLIENT_JOBID</li>
     *        <li>FTB_CLIENT_SUBSCRIPTION_STYLE</li>
     *        </ul>
     *
     * @param entry value for the information type to be set.
     *
     * @return status code of the operation as defined in FTB_DEF.
     * @see FTB_DEF
     *
     */
    public int set_attribute(int type, String entry)
    {
	if ( entry.matches("[a-zA-Z0-9_\\.]+") ) {
	    switch ( type ) {
	    case FTB_CLIENT_EVENTSPACE:
		event_space = entry.toUpperCase();
		break;
	    case FTB_CLIENT_NAME:
		client_name = entry.toUpperCase();
		break;
	    case FTB_CLIENT_JOBID:
		client_jobid = entry.toUpperCase();
		break;
	    case FTB_CLIENT_SUBSCRIPTION_STYLE:
		subscription_style = entry.toUpperCase();
		break;
	    default:
		return FTB_DEF.FTB_ERR_INVALID_FIELD;
	    }

	    return FTB_DEF.FTB_SUCCESS;
	}
	
	return FTB_DEF.FTB_ERR_FILTER_VALUE;
    }

    protected void resolve_location_id()
    {
	try {
	    InetAddress localMachine = InetAddress.getLocalHost();
	    hostname = localMachine.getHostName();

	}
	catch (java.net.UnknownHostException uhe) { 
	    uhe.printStackTrace();
	    System.exit(-1);
	}

	
	FTB_pid_starttime_t = DateFormat.getDateTimeInstance(DateFormat.SHORT,DateFormat.MEDIUM).format(new Date());
    }

    /**
     * Returns event space where this client belongs.
     *
     * @return event_space event space.
     *
     */
    public String get_event_space()
    {
	return event_space;
    }

    /**
     * Returns the description of the client.
     * The description includes,
     * <ul>
     * <li> event space </li>
     * <li> client name </li>
     * <li> client jobid </li>
     * <li> host name </li>
     * <li> client start time </li>
     * </ul>
     * @return client description.
     */
    public String client_info()
    {
	return event_space + "-" 
	    + client_name + "-" 
	    + client_jobid + "-" 
	    + hostname + "-" 
	    + FTB_pid_starttime_t;
    }

    /**
     * Returns subscription style of the client.
     *
     * @return subscription_style subscription style.
     *
     */
    public String subscription_style()
    {
	return subscription_style;
    }
}
