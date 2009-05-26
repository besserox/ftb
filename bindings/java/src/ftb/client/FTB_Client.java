package ftb.client;

import java.util.*;
import java.io.*;
import java.nio.ByteBuffer;


public class FTB_Client
{
    FTB_Client_Info client_info = null;
    HashMap<String,String>  publishable_map;      // key: event_name, value: severity
    HashMap<String, FTB_Subscription> subscription_map;
    FTB_Event event_template;
    FTB_Client_ID client_id;

    public FTB_Client(FTB_Client_Info client_info) 
    {
	this.client_info = client_info;
	this.client_id = new FTB_Client_ID(client_info);
	publishable_map = new HashMap<String,String>();
	subscription_map = new HashMap<String, FTB_Subscription>();
    }

    public void add_subscription(String subscription_key, FTB_Subscription subscription)
    {
	subscription_map.put(subscription_key, subscription);
    }

    void remove_subscription(String subscription_key) 
    {
	subscription_map.remove(subscription_key);
    }

    public int declare_publishable_event(FTB_Event_Info event_info)
    {
	if ( event_template == null ) {
	    event_template = new FTB_Event(client_info);
	}
	
	if ( publishable_map.containsKey(event_info.get_event_name().toUpperCase()) ) {
	    return FTB_DEF.FTB_ERR_DUP_CALL;
	}
	    
	publishable_map.put(event_info.get_event_name().toUpperCase(), event_info.get_severity().toUpperCase());
	
	return FTB_DEF.FTB_SUCCESS;
    }

    public Iterator<FTB_Subscription> subscription_collection()
    {
	return subscription_map.values().iterator();
    }

    String declared(String event_name) 
    {
	return publishable_map.get(event_name.toUpperCase());
    }

    /*
    public int disconnect()
    {
	Iterator<String> iter = subscription_map.iterator();
	while ( iter.hasNext() ) {
	    subscription_coordinator.cancelSubscription(session, (String)iter.next());  
	}

	return FTB_DEF.FTB_SUCCESS;
    }
    */
    public FTB_Event get_event_template()
    {
	event_template = new FTB_Event(client_info);
	return event_template;
    }

    public String subscription_style()
    {
	return client_info.subscription_style();
    }
    
    public FTB_Client_ID client_id()
    {
	return client_id;
    }
}
