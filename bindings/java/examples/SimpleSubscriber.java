/**
 *
 * SimpleSubscriber demonstrates an event subscription with polling
 * option. This is a Java version of "ftb_simplesubscriber.c"
 *
 */
import ftb.client.*;

public class SimpleSubscriber
{
    public static void main(String args[])
    {
	SimpleSubscriber simple_subscriber = new SimpleSubscriber();
	FTB_World ftb = new FTB_World();
	FTB_Client_Info info = new FTB_Client_Info();
	info.set_attribute(FTB_Client_Info.FTB_CLIENT_EVENTSPACE, "FTB.FTB_EXAMPLE.SIMPLE");
	info.set_attribute(FTB_Client_Info.FTB_CLIENT_NAME, "laputa");
	FTB_Client_Handle client_handle = null;
	FTB_Subscription subscription = null;

	// Connect to FTB and subscribe to an event.
	try {
	    client_handle = ftb.connect(info);
	    subscription = ftb.subscribe(client_handle,"event_name=SIMPLE_EVENT", null, null);
	}
	catch (FTB_Exception fe) {
	    System.out.println(fe);
	    System.exit(0);
	}

	for (int i=0; i<10; i++) {
	    FTB_Receive_Event event = null;
	    event = subscription.poll();

	    if ( event != null ) {
		System.out.println(event);
	    }

	    try {
		Thread.sleep(10000);
	    }
	    catch ( InterruptedException ie ) {
		ie.printStackTrace();
	    }
	}

	// Unsubscribe and disconnect
	ftb.unsubscribe(subscription);
	ftb.disconnect(client_handle);
    }
}
