package ftb.client;

/**
 * Used to send <i>payload</i> data with
 * an event. It consists of
 * <ul>
 * <li>event type</li>
 * <li>event payload</li>
 * </ul>
 * 
 *
 */
public class FTB_Event_Property
{
    public static byte INFO = 1;
    public static byte RESPONSE = 2;

    byte event_type;
    byte[] event_payload;

    /**
     * Constructs an event property. An event property includes
     * <ul>
     * <li> event type </li>
     * <li> event payload </li>
     * </ul>
     *
     * @param event_type type of the event to send.
     * event_type is one of the following two,
     * <ul>
     * <li> INFO </li>
     * <li> RESPONSE </li>
     * </ul>
     * 
     */
    public FTB_Event_Property(byte event_type)
    {
	this.event_type = event_type;
	this.event_payload = new byte[FTB_DEF.FTB_MAX_PAYLOAD_DATA];
    }

    /**
     * Returns the event payload array.
     *
     * @return event_payload event payload array.
     *
     */
    public byte[] event_payload()
    {
	return event_payload;
    }
    
    /**
     * Returns the event type
     *
     * @return event_type event type
     *
     */
    public byte event_type()
    {
	return event_type;
    }

    /**
     * Assigns a message of string as the content of event payload.
     *
     * @param message message content string.
     * @return status code as defined in FTB_DEF.
     * @see FTB_DEF        
     *
     */
    public int set_payload(String message) 
    {
	try {
	    byte[] buffer = message.getBytes("ASCII");

	    if ( buffer.length > FTB_DEF.FTB_MAX_PAYLOAD_DATA ) {
		return FTB_DEF.FTB_ERR_GENERAL;
	    }

	    System.arraycopy(buffer, 0, event_payload, 0, buffer.length);
	}
	catch (java.io.UnsupportedEncodingException ue) {
	    ue.printStackTrace();
	    return FTB_DEF.FTB_ERR_GENERAL;
	}

	return FTB_DEF.FTB_SUCCESS;
    }

    /**
     * Copies the input byte array 
     *
     */
    public int set_payload(byte[] buffer) 
    {
	if ( buffer.length > FTB_DEF.FTB_MAX_PAYLOAD_DATA ) {
	    return FTB_DEF.FTB_ERR_GENERAL;
	}

	System.arraycopy(buffer, 0, event_payload, 0, buffer.length);
	return FTB_DEF.FTB_SUCCESS;
    }
}
