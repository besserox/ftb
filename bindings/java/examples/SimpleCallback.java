/**
 *
 * SimpleCallback demonstrates an event subscription with callback option.
 *
 */


import ftb.client.*;

public class SimpleCallback implements FTB_Callable
{
    boolean loop = true;

    /**
     * Callback function.
     *
     */
    public void message_received(FTB_Receive_Event event, Object arg)
    {
        System.out.println(event);
	loop = false;
    }

    public static void main(String args[])
    {
	SimpleCallback simple_subscriber = new SimpleCallback();
	FTB_World ftb = new FTB_World();
	FTB_Client_Info info = new FTB_Client_Info();
	info.set_attribute(FTB_Client_Info.FTB_CLIENT_EVENTSPACE, "FTB.FTB_EXAMPLE.SIMPLE");
	info.set_attribute(FTB_Client_Info.FTB_CLIENT_NAME, "laputa");
	FTB_Client_Handle client_handle = null;
	FTB_Subscription subscription = null;

	// Connect to FTB and subscribe to an event
	try {
	    client_handle = ftb.connect(info);
	    subscription = ftb.subscribe(client_handle,"event_name=SIMPLE_EVENT", simple_subscriber, null);
	}
	catch (FTB_Exception fe) {
	    System.out.println(fe);
	    System.exit(0);
	}

	// Loop until an event is received.
	while ( simple_subscriber.loop ) {
	    try {
		Thread.sleep(10000);
	    }
	    catch (InterruptedException ie) {
	    }
	}

	// Unsubscribe and disconnect
	ftb.unsubscribe(subscription);
	ftb.disconnect(client_handle);
    }
}
