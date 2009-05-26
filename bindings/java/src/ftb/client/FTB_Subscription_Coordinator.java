package ftb.client;

import java.util.*;
import java.util.regex.Pattern;
import java.io.*;

public class FTB_Subscription_Coordinator implements Runnable
{
    ArrayList<FTB_Subscription>  subscription_list;
    InputStream incomming_channel;
    Thread distributor = null;
    boolean loop_is_valid = false;

    public FTB_Subscription_Coordinator(InputStream incomming_channel)
    {
	subscription_list = new ArrayList<FTB_Subscription>();
	this.incomming_channel = incomming_channel;
	loop_is_valid = true;
	distributor = new Thread(this);
	distributor.start();
    }

    public void run()
    {
	byte[] buffer = new byte[FTB_Message.length()];

	while ( loop_is_valid ) {
	    try {
		incomming_channel.read(buffer, 0, FTB_Message.length());
		FTB_Message message = new FTB_Message(buffer);
		deliever_event(message);
	    }
	    catch (IOException ie) {
	    }
	}
    }

    public void stop()
    {
	loop_is_valid = false;

	if ( distributor != null ) {
	    distributor.interrupt();
	}
    }

    void deliever_event(FTB_Message message)
    {
	String routing_key = message.event.region.toUpperCase() + "." + message.event.comp_cat.toUpperCase() + "." + message.event.comp.toUpperCase() + "."
	    + message.event.event_name.toUpperCase() + "." + message.event.severity.toUpperCase();

	ListIterator iter = subscription_list.listIterator();

	while ( iter.hasNext() ) {
	    FTB_Subscription subscriber = (FTB_Subscription)iter.next();
	    if ( routing_key.matches(subscriber.subscription_key()) ) {
		FTB_Receive_Event rvt = new FTB_Receive_Event(message);
		subscriber.put_event(rvt);
	    }
	}
    }


    public void register_subscription(FTB_Subscription subscription)
    {
	subscription_list.add(subscription);
    }

    public int  cancelSubscription(FTB_Subscription subscription)
    {
	if ( subscription_list.contains(subscription) != true ) {
	    return FTB_DEF.FTB_ERR_INVALID_HANDLE;
	}
	subscription_list.remove(subscription);
	return FTB_DEF.FTB_SUCCESS;
    }
}
