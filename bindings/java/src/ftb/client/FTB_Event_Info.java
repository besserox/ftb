package ftb.client;

/**
 * Denotes an event that the client wishes to publish.
 * A client should create FTB_EVent_Info and register it with the
 * FTB_World.
 *
 */
public class FTB_Event_Info
{
    String event_name;
    String severity;

    /**
     * Constructs event information for registration.
     * @param event_name ame of the event.
     * @param severity Severity of the event.
     *
     */
    public FTB_Event_Info(String event_name, String severity)
    {
	this.event_name = event_name;
	this.severity   = severity;
    }

    /**
     * Returns the event name.
     *
     * @return event name.
     *
     */
    public String get_event_name()
    {
	return event_name;
    }

    /**
     * Returns the severity.
     * 
     * @return severity of the event.
     *
     */
    public String get_severity()
    {
	return severity;
    }
}
