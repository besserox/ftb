package ftb.client;

/**
 * Interface that a client uses to defines a callback.
 * A FTB client needs to implement this interface if it wishes
 * a method to be invoked upon arrival of certain events.
 *
 */
public interface FTB_Callable
{
    /**
     * To be executed up arrival of event (i.e., this is <i>callback</i>).
     *
     * @param event Event received by the client.
     * @param arg User-specified argument.
     */
    public abstract void message_received(FTB_Receive_Event event, Object arg);
}
