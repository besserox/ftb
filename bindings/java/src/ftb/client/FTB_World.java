package ftb.client;
import java.util.*;
import java.net.*;
import java.io.*;

/**
 * Portal class to Fault Tolerant Backplance (FTB). A
 * client that wishes to send or receive events through FTB should
 * connect to an instance of FTB_World. FTB_World provides
 * all APIs of FTB (except API for event polling).
 * <p>
 * 
 *
 */
public class FTB_World
{
    final byte   FTBNI_BOOTSTRAP_MSG_TYPE_ADDR_REG = 1;
    String agent_home;
    int agent_port;
    OutputStream outgoing_channel;
    InputStream incomming_channel;
    FTB_Location_ID location_id;
    FTB_Location_ID parent_location_id;
    int client_population = 0;
    FTB_Subscription_Coordinator subscription_coordinator;
    Object mutex;
    String FTB_BSTRAP_SERVER;
    int FTB_BSTRAP_PORT;

    /**
     * Creates an instance of FTB_World.
     *
     */
    public FTB_World()
    {
	mutex = new Object();
	location_id = new FTB_Location_ID(FTB_Util.resolve_location_id(), FTB_Util.resolve_starttime(), 10);
    }

    private int connect_to_agent()
    {
	InetAddress address;
	byte[] buffer = new byte[FTB_Message.length()];
	

	FTB_BSTRAP_SERVER = System.getenv("FTB_BSTRAP_SERVER");
	String port   = System.getenv("FTB_BSTRAP_PORT");

	if ( FTB_BSTRAP_SERVER == null ) 
	    FTB_BSTRAP_SERVER = "localhost";

	if ( port == null ) 
	    FTB_BSTRAP_PORT = 14455;
	else 
	    FTB_BSTRAP_PORT = Integer.getInteger(port);

	try {
	    address = resolve_agent(FTB_BSTRAP_SERVER, FTB_BSTRAP_PORT);
	}
	catch (FTB_Exception fe) {
	    return fe.error_code();
	}

	try {
	    Socket socket = new Socket(address, agent_port);
	    outgoing_channel = socket.getOutputStream();
	    incomming_channel = socket.getInputStream();
	}
	catch ( IOException ie ) {
	    return FTB_DEF.FTB_ERR_NETWORK_NO_ROUTE;
	}

	try {
	  FTB_Util.writeInt(buffer, 0, 1);
	  outgoing_channel.write(buffer,0,4);
	
	  location_id.writeBytes(buffer,0);
	  outgoing_channel.write(buffer,0, FTB_Location_ID.length());

	  int message_type = 0x11;
	  FTB_Util.writeInt(buffer, 0, 0x11);
	  location_id.writeBytes(buffer,4);
	  outgoing_channel.write(buffer, 0, FTB_Message.length());
	  incomming_channel.read(buffer,0, FTB_Location_ID.length());
	}
	catch (IOException ie) {
	  ie.printStackTrace();
	}
	
	parent_location_id = new FTB_Location_ID(buffer,0);
	return FTB_DEF.FTB_SUCCESS;
    }


    /**
     * Connects to FTB_World and returns a client handle.
     *
     * @param client_info client information.
     * @return client handle of the connecting client.
     * @throws FTB_Exception
     *
     */
    public FTB_Client_Handle connect(FTB_Client_Info client_info) throws FTB_Exception
    { 
	int ret;

	synchronized (mutex) {
	    if ( client_population == 0 ) {
		if ( (ret = connect_to_agent()) != FTB_DEF.FTB_SUCCESS ) {
		    throw new FTB_Exception(ret);
		}
		client_population++;
	    }
	}

	FTB_Client        client = new FTB_Client(client_info);
	FTB_Client_Handle client_handle = new FTB_Client_Handle(client);

	// Now register
	FTB_ID ftb_id = new FTB_ID(location_id, client.client_id());
	FTB_ID parent = new FTB_ID(parent_location_id, client.client_id());
	FTB_Message message = new FTB_Message(FTB_Message.FTBM_MSG_TYPE_CLIENT_REG,ftb_id,parent,null);

	if ( (ret = send_message(message)) != FTB_DEF.FTB_SUCCESS ) {
	    throw new FTB_Exception(ret);
	}
	
	return client_handle;
    }

    /**
     * Declares an event to publish.
     *
     * @param client_handle Client handle
     * @param event_info event information
     * @return status code of the operation (e.g. FTB_SUCCESS) defined in FTB_DEF
     * @see FTB_DEF
     *         
     * 
     */
    public int declare_publishable_event(FTB_Client_Handle client_handle, FTB_Event_Info event_info) 
    {
	int ret;
	
	if ( (ret = client_handle.client().declare_publishable_event(event_info)) != FTB_DEF.FTB_SUCCESS ) {
	    return ret;
	}

	return FTB_DEF.FTB_SUCCESS;
    }

    /**
     * Publishes an event.
     *
     * @param client_handle Client handle of the publishing client.
     * @param event_name Event name.
     * @param event_property Event property that includes payload data.
     * @return event handle that represents the event sent out.
     * @throws FTB_Exception
     *
     */
    public FTB_Event_Handle publish(FTB_Client_Handle client_handle, String event_name, FTB_Event_Property event_property) 
	throws FTB_Exception
    {
	FTB_Client client = client_handle.client();
	FTB_Event event_template = client.get_event_template();
	String severity;
	int resp;

	// Check whether the given event is registered.
	if ( (severity=client_handle.client().declared(event_name)) == null ) {
	    throw new FTB_Exception(FTB_DEF.FTB_ERR_INVALID_EVENT_NAME);
	}

	if ( event_property != null ) {
	    if ( event_property.event_type() > 2 || event_property.event_type() < 1 ) {
		throw new FTB_Exception(FTB_DEF.FTB_ERR_INVALID_EVENT_TYPE);
	    }

	    event_template.set_eventtype(event_property.event_type());
	    event_template.set_payload(event_property.event_payload());
	}
	else {
	    event_template.set_eventtype(FTB_Event_Property.INFO);
	}
	
	FTB_ID ftb_id = new FTB_ID(location_id, client.client_id());
	FTB_ID parent = new FTB_ID(parent_location_id, client.client_id());
	event_template.set_eventname(event_name.toUpperCase());
	event_template.set_severity(severity);

	FTB_Message message = new FTB_Message(FTB_Message.FTBM_MSG_TYPE_NOTIFY, ftb_id, parent, event_template);

	if ( (resp = send_message(message)) != FTB_DEF.FTB_SUCCESS ) {
	    throw new FTB_Exception(resp);
	}
	
	return new FTB_Event_Handle(event_name.toUpperCase(), severity, client.client_id(), event_template.seqnum,location_id);
    }

    /**
     * Compares two event handles to test whether they represent the same event.
     *
     * @param handle_a The first event handle.
     * @param handle_b The second event handle.
     * @return <code>true</code> if two event handles are identical, <br>
     *         <code>false</code> otherwise.
     *
     */
    public boolean compare_event_handles(FTB_Event_Handle handle_a, FTB_Event_Handle handle_b)
    {
	return 
	    handle_a.event_name.equals(handle_b.event_name) &&
	    handle_a.severity.equals(handle_b.severity) &&
	    (handle_a.seqnum == handle_b.seqnum) &&
	    handle_a.client_id.equals(handle_b.client_id) &&
	    handle_a.location_id.equals(handle_b.location_id);
    }

    /**
     * Obtains the event handle from an event.
     *
     * @param event Event from which the event handle will be extracted.
     * @return event handle of the event.
     * @see FTB_EVent_Handle
     *
     */
    public FTB_Event_Handle get_event_handle(FTB_Receive_Event event)
    {
	FTB_Client_ID client_id = new FTB_Client_ID(event.event_space, event.client_name, event.client_extension);
	return new FTB_Event_Handle(event.event_name, event.severity, client_id, event.seqnum, event.incoming_src);
    }

    /**
     * Disconncects from FTB_World.
     *
     * @param client_handle The client handle of the disconnecting client.
     *
     * @return status code of the operation.
     * @see FTB_DEF        
     *
     */
    public int disconnect(FTB_Client_Handle client_handle)
    {
	int resp;

	FTB_Client client = client_handle.client();

	FTB_ID ftb_id = new FTB_ID(location_id, client.client_id());
	FTB_ID parent = new FTB_ID(parent_location_id, client.client_id());

	FTB_Message message = new FTB_Message(FTB_Message.FTBM_MSG_TYPE_CLIENT_DEREG, ftb_id, parent, null);

	if ( (resp = send_message(message)) != FTB_DEF.FTB_SUCCESS ) {
	    return resp;
	}

	if ( --client_population == 0 && subscription_coordinator != null ) {
	    Iterator<FTB_Subscription> iter = client.subscription_collection();
	    while ( iter.hasNext() ) {
		FTB_Subscription subscription = iter.next();
		subscription_coordinator.cancelSubscription(subscription);
		subscription.cleanup();
	    }

	    subscription_coordinator.stop();
	    subscription_coordinator = null;

	    try {
		incomming_channel.close();
		outgoing_channel.close();
	    }
	    catch (IOException ie) {
		return FTB_DEF.FTB_ERR_NETWORK_GENERAL;
	    }
	    incomming_channel = null;
	    outgoing_channel = null;
	}

	return FTB_DEF.FTB_SUCCESS;
    }

    /**
     * Subscribes to an event.
     *
     * @param client_handle Client handle of the subscribing client.
     * @param subscription_str Subscription string of the event to subscribe.
     * @param callback Interface to receive events for callback.
     * @param arg User-specified argument for the callback.
     *
     * @return FTB_Subscription that represent this event subscription.
     * @throws FTB_Exception
     *
     */
    public FTB_Subscription subscribe(FTB_Client_Handle client_handle, String subscription_str, 
				      FTB_Callable callback, Object arg)
	throws FTB_Exception
    {
	int resp;
	FTB_Client client = client_handle.client();
	FTB_Subscription_Parser parser = new FTB_Subscription_Parser();
	FTB_Subscription subscription;
	
	if ( (resp=parser.parse_subscription_string(subscription_str)) != FTB_DEF.FTB_SUCCESS ) {
	    throw new FTB_Exception(resp);
	}
	
	FTB_Event event = new FTB_Event(parser.region, parser.comp_cat, parser.comp, parser.event_name, parser.severity,
					parser.client_jobid, parser.client_name, parser.hostname,
					(short)0,(byte)1);

	if ( subscription_coordinator == null ) {
	    subscription_coordinator = new FTB_Subscription_Coordinator(incomming_channel);
	}
	
	if ( callback != null ) 
	    subscription = new FTB_Subscription_With_Callback(parser.get_subscription_key(), callback, arg, event, client);
	else
	    subscription = new FTB_Subscription(parser.get_subscription_key(), event, client);

	subscription_coordinator.register_subscription(subscription);
	client.add_subscription(parser.get_subscription_key(), subscription);

	FTB_ID ftb_id = new FTB_ID(location_id, client.client_id());
	FTB_ID parent = new FTB_ID(parent_location_id, client.client_id());

	FTB_Message message = new FTB_Message(FTB_Message.FTBM_MSG_TYPE_REG_SUBSCRIPTION, ftb_id, parent, event);

	if ( (resp = send_message(message)) != FTB_DEF.FTB_SUCCESS ) {
	    // remove subscription from both subscription_coordinator and client
	    client.remove_subscription(subscription.subscription_key());
	    subscription_coordinator.cancelSubscription(subscription);
	    throw new FTB_Exception(resp);
	}
	return subscription;
    }

    /**
     * Unsubscribes an event.
     *
     * @param subscription FTB_Subscription (subscription handle) of the event to cancel subscription.
     * @return status code of the <code>unsubscription</code> as defined in FTB_DEF.
     * @see FTB_DEF
     *
     */
    public int unsubscribe(FTB_Subscription subscription)
    {
	int resp;
	
	FTB_Client client = subscription.client;
	FTB_ID ftb_id = new FTB_ID(location_id, client.client_id());
	FTB_ID parent = new FTB_ID(parent_location_id, client.client_id());

	FTB_Message message = new FTB_Message(FTB_Message.FTBM_MSG_TYPE_SUBSCRIPTION_CANCEL, ftb_id, parent, subscription.event());

	if ( (resp = send_message(message)) != FTB_DEF.FTB_SUCCESS ) {
	    return resp;
	}
	subscription_coordinator.cancelSubscription(subscription);
	client.remove_subscription(subscription.subscription_key());
	subscription.cleanup();

	return FTB_DEF.FTB_SUCCESS;
    }


    private synchronized int send_message(FTB_Message message)
    {
	byte[] buffer = new byte[FTB_Message.length()];
	message.writeBytes(buffer,0);

	try {
	    outgoing_channel.write(buffer,0, FTB_Message.length());
	}
	catch (IOException ie) {
	    System.out.println("network error");
	    return FTB_DEF.FTB_ERR_NETWORK_GENERAL;
	}
	
	return FTB_DEF.FTB_SUCCESS;
    }

    private InetAddress resolve_agent(String server, int port) throws FTB_Exception
    {
	InetAddress address, agent_address;
	DatagramPacket packet;
	DatagramSocket socket;
	byte[] buf = new byte[4+FTB_DEF.FTB_MAX_HOST_ADDR+4+4];

	try {
	    address = InetAddress.getByName(server);
	    socket = new DatagramSocket();
	}
	catch (SocketException se) {
	    throw new FTB_Exception(FTB_DEF.FTB_ERR_NETWORK_GENERAL);
	}
	catch (UnknownHostException ue) {
	    throw new FTB_Exception(FTB_DEF.FTB_ERR_NETWORK_GENERAL);
	}

	buf[0] = FTBNI_BOOTSTRAP_MSG_TYPE_ADDR_REG;
	short level = 0-1;

	FTB_Util.writeShort(buf, 4+FTB_DEF.FTB_MAX_HOST_ADDR + 4, level);

	packet = new DatagramPacket(buf, buf.length, 
				    address, port);

	try {
	    socket.send(packet);
	    packet = new DatagramPacket(buf, buf.length);
	    socket.receive(packet);
	}
	catch (IOException ie) {
	    throw new FTB_Exception(FTB_DEF.FTB_ERR_NETWORK_GENERAL);
	}

	agent_port = FTB_Util.readInt(buf, 4+FTB_DEF.FTB_MAX_HOST_ADDR);
	socket.close();

	agent_home = FTB_Util.readString(buf, 4, FTB_DEF.FTB_MAX_HOST_ADDR);

	try {
	    agent_address = InetAddress.getByName(agent_home);
	}
	catch (UnknownHostException uhe) {
	    throw new FTB_Exception(FTB_DEF.FTB_ERR_NETWORK_GENERAL);
	}

	return agent_address;
    }
}
