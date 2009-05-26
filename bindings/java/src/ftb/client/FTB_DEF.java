package ftb.client;

/**
 * Includes status and error codes of APIs. 
 *
 *
 *
 */
public class FTB_DEF
{
    public static final int FTB_SUCCESS                             = 0;
    public static final int FTB_ERR_GENERAL                         = -1;
    public static final int FTB_ERR_EVENTSPACE_FORMAT               = -2;
    public static final int FTB_ERR_SUBSCRIPTION_STYLE              = -3;
    public static final int FTB_ERR_INVALID_VALUE                   = -4;
    public static final int FTB_ERR_DUP_CALL                        = -5;
    public static final int FTB_ERR_NULL_POINTER                    = -6;
    public static final int FTB_ERR_NOT_SUPPORTED                   = -7;
    public static final int FTB_ERR_INVALID_FIELD                   = -8;
    public static final int FTB_ERR_INVALID_HANDLE                  = -9;
    public static final int FTB_ERR_DUP_EVENT                       = -10;
    public static final int FTB_ERR_INVALID_SCHEMA_FILE             = -11;
    public static final int FTB_ERR_INVALID_EVENT_NAME              = -12;
    public static final int FTB_ERR_INVALID_EVENT_TYPE              = -13;
    public static final int FTB_ERR_SUBSCRIPTION_STR                = -14;
    public static final int FTB_ERR_FILTER_ATTR                     = -15;
    public static final int FTB_ERR_FILTER_VALUE                    = -16;
    public static final int FTB_GOT_NO_EVENT                        = -17;
    public static final int FTB_FAILURE                             = -18;
    public static final int FTB_ERR_INVALID_PARAMETER               = -19;
    public static final int FTB_ERR_NETWORK_GENERAL                 = -20;
    public static final int FTB_ERR_NETWORK_NO_ROUTE                = -21;

    public static final int FTB_DEFAULT_POLLING_Q_LEN               =  64;
    public static final int FTB_MAX_CLIENTSCHEMA_VER                =  8;
    public static final int FTB_MAX_EVENTSPACE                      =  64;
    public static final int FTB_MAX_CLIENT_NAME                     =  16;
    public static final int FTB_MAX_CLIENT_JOBID                    =  16;
    public static final int FTB_MAX_EVENT_NAME                      =  32;
    public static final int FTB_MAX_SEVERITY                        =  16;
    public static final int FTB_MAX_HOST_ADDR                       =  64;
    public static final int FTB_MAX_PID_TIME                        =  32;
    public static final int FTB_MAX_PAYLOAD_DATA                    =  368;

    public static final int FTB_EVENT_SIZE                          = 720;
}
