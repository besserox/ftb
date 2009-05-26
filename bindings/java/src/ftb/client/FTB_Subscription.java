package ftb.client;
import java.util.*;

/**
 * Represents an event subscription by a client.
 * This class is never to be instantiated directly by a client. It is
 * instantiated and returned by FTB_World when a client subscribes to
 * an event.  <p> FTB_Subscription serves an a handle. It is used to
 * cancel (unsubscribe) the event it denotes. Also, more importantly,
 * a client can poll events that the subscription handle denotes if
 * the subscription is registered with <i>polling option</i>.
 * 
 *
 */
public class FTB_Subscription
{
    String subscription_key;
    Object mutex;
    LinkedList<FTB_Receive_Event> queue;
    FTB_Client client;
    FTB_Event event;

    protected FTB_Subscription(String subscription_key, FTB_Event event, FTB_Client client)
    {
	this.subscription_key = subscription_key;
	this.event = event;
	this.client = client;

	queue = new LinkedList<FTB_Receive_Event>();
	mutex = new Object();
    }

    protected void put_event(FTB_Receive_Event event) 
    {
	synchronized (mutex) {
	    queue.add(event);
	}
    }

    /**
     * Returns an event in the queue if any.
     *
     * @return event an event in the queue.
     *         <b>null</b> is returned if no event is available.
     *
     */
    public FTB_Receive_Event poll()
    {
	synchronized (mutex) {
	    return queue.poll();
	}
    }

    protected String subscription_key()
    {
	return subscription_key;
    }

    protected void cleanup()
    {
    }

    protected FTB_Event event()
    {
	return event;
    }
}


class FTB_Subscription_With_Callback extends FTB_Subscription implements Runnable
{
    FTB_Callable callback;
    Object argument;
    Thread handler = null;
    boolean loop_is_valid = false;

    protected FTB_Subscription_With_Callback(String subscription_key, FTB_Callable callable, Object arg, 
					  FTB_Event event, FTB_Client client)
    {
	super(subscription_key, event, client);
	
	callback = callable;
	argument = arg;
	loop_is_valid = true;
	handler = new Thread(this);
	handler.start();
    }


    protected void put_event(FTB_Receive_Event event)
    {
	synchronized (mutex) {
	    queue.add(event);
	    mutex.notify();
	}
    }

    public void run()
    {
	while ( loop_is_valid ) {
	    synchronized ( mutex ) {
		if ( queue.isEmpty() ) {
		    try {
			mutex.wait();
		    }
		    catch (InterruptedException ie) {
			//	ie.printStackTrace();
		    }

		    FTB_Receive_Event event = queue.poll();
		    if ( event != null ) {
			callback.message_received(event, argument);
		    }
		}
	    }
	}
    }

    protected void cleanup()
    {
	loop_is_valid = false;

	if ( handler != null ) 
	    handler.interrupt();
    }
}
