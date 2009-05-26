package ftb.client;

/**
 * Represents an incomming event received by a client. The event includes,
 * <ul>
 * <li> event space </li>
 * <li> event name </li>
 * <li> severity </li>
 * <li> client job ID </li>
 * <li> client extension </li>
 * <li> sequence number </li>
 * <li> host name of the sender </li>
 * <li> start time of the sender </li>
 * <li> process ID of the sender </li>
 * <li> payload byte array </li>
 * </ul>
 * These information can be retrieved individually by designated methods.
 *
 */
public class FTB_Receive_Event
{
    protected String event_space;
    protected String event_name;
    protected String severity;
    protected String client_jobid;
    protected String client_name;
    protected byte client_extension;
    protected short seqnum;
    protected FTB_Location_ID incoming_src;
    protected byte event_type;
    protected byte[] event_payload;

    public FTB_Receive_Event(FTB_Message message)
    {
	event_space = message.event.region + "." + message.event.comp_cat + "." + message.event.comp;
	event_name  = message.event.event_name;
	client_jobid = message.event.client_jobid;
	client_name = message.event.client_name;
	client_extension = message.src.client_id.ext;
	seqnum = message.event.seqnum;
	incoming_src = message.src.location_id;;
	event_type = message.event.event_type;
	event_payload = message.event.event_payload;
    }

    /**
     * Returns event space.
     *
     * @return event space
     *
     */
    public String event_space()
    {
	return event_space;
    }

    /**
     * Returns event name.
     *
     * @return event name 
     *
     */
    public String event_name()
    {
	return event_name;
    }

    /**
     * Returns severity of the event.
     *
     * @return severity
     *
     */
    public String severity()
    {
	return severity;
    }

    /**
     * Returns the job ID of the sender.
     *
     * @return client jobid
     *
     */
    public String client_jobid()
    {
	return client_jobid;
    }

    /**
     * Returns the name of the sender.
     *
     * @return client name
     *
     */
    public String client_name()
    {
	return client_name;
    }

    /**
     * Returns the client extension.
     *
     * @return client extension;
     *
     */
    public byte client_extension()
    {
	return client_extension();
    }

    /**
     * Return the sequence number.
     *
     * @return seqnum;
     *
     */
    public short seqnum()
    {
	return seqnum;
    }

    /**
     * Return the event type.
     *
     * @return event type
     *
     */
    public byte event_type()
    {
	return event_type;
    }

    /**
     * Return the payload array.
     *
     * @return event payload byte array.
     *
     */
    public byte[] event_payload()
    {
	return event_payload;
    }

    /**
     * Returns the IP address of the sender. 
     *
     * @return host name of the sender.
     *
     */
    public String incoming_host()
    {
	return incoming_src.hostname;
    }

    /**
     * Returns the start time of the sender
     *
     * @return start time of the sender.
     *
     */
    public String FTB_pid_starttime_t()
    {
	return incoming_src.FTB_pid_starttime_t;
    }

    /**
     * Returns the process ID of the sender
     *
     * @return Process ID of the sender.
     *
     */
    public int sender_pid()
    {
	return incoming_src.pid;
    }

    /**
     * Returns String representation of the event.
     *
     * @return event in String format
     */
    public String toString()
    {
	return "Sender Name: " + this.incoming_src.hostname + "\n"
	    +  "Event Space: " + this.event_space + "\n"
	    +  "Event Name:  " + this.event_name  + "\n"
	    +  "Severity: "    + this.severity    + "\n"
	    +  "PID Start Time: " + this.incoming_src.FTB_pid_starttime_t + "\n\n";
    }
}
