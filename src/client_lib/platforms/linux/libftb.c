/***********************************************************************************/
/* FTB:ftb-info */
/* This file is part of FTB (Fault Tolerance Backplance) - the core of CIFTS
 * (Co-ordinated Infrastructure for Fault Tolerant Systems)
 *
 * See http://www.mcs.anl.gov/research/cifts for more information.
 * 	
 */
/* FTB:ftb-info */

/* FTB:ftb-fillin */
/* FTB_Version: 0.6.2
 * FTB_API_Version: 0.5
 * FTB_Heredity:FOSS_ORIG
 * FTB_License:BSD
 */
/* FTB:ftb-fillin */

/* FTB:ftb-bsd */
/* This software is licensed under BSD. See the file FTB/misc/license.BSD for
 * complete details on your rights to copy, modify, and use this software.
 */
/* FTB:ftb-bsd */
/***********************************************************************************/

#include <stdio.h>
#include <string.h>
#include "ftb.h"
#include "ftb_client_lib.h"

/*Linux wrapper for client_lib*/
FILE *FTBU_log_file_fp;

int FTB_Connect(const FTB_client_t * client_info, FTB_client_handle_t * client_handle)
{
    FTBU_log_file_fp = stderr;
    return FTBC_Connect(client_info, 0, client_handle); /* Set extention to 0 */
}


int FTB_Publish(FTB_client_handle_t client_handle, const char *event_name,
                const FTB_event_properties_t * event_properties, FTB_event_handle_t * event_handle)
{
    return FTBC_Publish(client_handle, event_name, event_properties, event_handle);
}


int FTB_Subscribe(FTB_subscribe_handle_t * subscribe_handle, FTB_client_handle_t client_handle,
                  const char *subscription_str, int (*callback) (FTB_receive_event_t *, void *),
                  void *arg)
{
    if (callback == NULL)
        return FTBC_Subscribe_with_polling(subscribe_handle, client_handle, subscription_str);
    else
        return FTBC_Subscribe_with_callback(subscribe_handle, client_handle, subscription_str, callback,
                                            arg);
}


int FTB_Unsubscribe(FTB_subscribe_handle_t * subscribe_handle)
{
    return FTBC_Unsubscribe(subscribe_handle);
}


int FTB_Declare_publishable_events(FTB_client_handle_t client_handle, const char *schema_file,
                                   const FTB_event_info_t * einfo, int num_events)
{
    return FTBC_Declare_publishable_events(client_handle, schema_file, einfo, num_events);
}


int FTB_Poll_event(FTB_subscribe_handle_t subscribe_handle, FTB_receive_event_t * receive_event)
{
    if (receive_event == NULL) {
        return FTB_ERR_NULL_POINTER;
    }
    else
        return FTBC_Poll_event(subscribe_handle, receive_event);
}


int FTB_Disconnect(FTB_client_handle_t client_handle)
{
    return FTBC_Disconnect(client_handle);
}


int FTB_Get_event_handle(const FTB_receive_event_t receive_event, FTB_event_handle_t * event_handle)
{
    return FTBC_Get_event_handle(receive_event, event_handle);
}


int FTB_Compare_event_handles(const FTB_event_handle_t event_handle1,
                              const FTB_event_handle_t event_handle2)
{
    return FTBC_Compare_event_handles(event_handle1, event_handle2);
}

int FTB_Check_error_code(const int error_code, int *error_class, int *error_value)
{
       return FTBC_Check_error_code(error_code, error_class, error_value);

}
