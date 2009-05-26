package ftb.client;

/**
 * Contains the error code of API. Error code is as defined in FTB_DEF.
 * @see FTB_DEF
 *
 */
public class FTB_Exception extends Exception
{
    int code;

    /**
     * 
     *
     * @param code Error code
     *
     */
    public FTB_Exception(int code)
    {
	super(String.valueOf(code));
	this.code = code;
    }

    /**
     * Returns error code of the exception.
     *
     * @return error code.
     *
     */
    public int error_code() 
    {
	return code;
    }
}
