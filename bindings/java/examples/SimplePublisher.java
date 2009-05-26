/**
 *
 * SimplePublisher demonstrates event publishing.  It particularly
 * shows how to send a message of string by an event.
 *
 */

import ftb.client.*;

public class SimplePublisher
{
    public static void main(String args[])
    {	
	FTB_World ftb = new FTB_World();
	FTB_Event_Info event_info = new FTB_Event_Info("SIMPLE_EVENT", "INFO");
	FTB_Event_Property event_property = new FTB_Event_Property(FTB_Event_Property.INFO);
	FTB_Client_Handle client_handle = null;

	// set a message as the content of event payload.
	String message = "This is a test mail from Davinci!";
	event_property.set_payload(message);

	FTB_Client_Info client_info = new FTB_Client_Info();
	client_info.set_attribute(FTB_Client_Info.FTB_CLIENT_EVENTSPACE, "FTB.FTB_EXAMPLES.SIMPLE");
	client_info.set_attribute(FTB_Client_Info.FTB_CLIENT_NAME, "davinci");

	// Connect to FTB
	try {
	    client_handle = ftb.connect(client_info);
	}
	catch (FTB_Exception fe) {
	}

	// Declare (register) an event to publish
	if ( ftb.declare_publishable_event(client_handle, event_info) != FTB_DEF.FTB_SUCCESS ) {
	    System.out.println("Event Declaration Failed");
	    System.exit(-1);
	}

	// Publish the event 10 times
	for (int i=0; i<10; i++) {
	    try {
		FTB_Event_Handle ehandle = ftb.publish(client_handle, "SIMPLE_EVENT", event_property);
	    }
	    catch (FTB_Exception e) {
		System.out.println(e);
	    }

	    try {
		Thread.sleep(10000);
	    }
	    catch ( InterruptedException ie ) {
		ie.printStackTrace();
	    }
	}

	// Disconnect from FTB
	ftb.disconnect(client_handle);
    }
}
