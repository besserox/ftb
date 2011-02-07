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

/*Compute node libftb wrapper based on ZOID for BG/L*/
#include <string.h>
#include "ftb.h"

#if defined FTBI_PLATFORM_IS_BGL
#include "ftb_zoid_client_bgl.h"
#elif defined FTBI_PLATFORM_IS_BGP
#include "ftb_zoid_client_bgp.h"
#endif


int FTB_Connect(const FTB_client_t * client_info, FTB_client_handle_t * client_handle)
{
#if defined FTBI_PLATFORM_IS_BGP
    __zoid_init();
#endif
    return ZOID_FTB_Connect(client_info, client_handle);
}


int FTB_Declare_publishable_events(FTB_client_handle_t client_handle, const char *schema_file,
                                   const FTB_event_info_t * einfo, int num_events)
{
    return ZOID_FTB_Declare_publishable_events(client_handle, NULL, einfo, num_events);
}


int FTB_Publish(FTB_client_handle_t client_handle, const char *event_name,
                const FTB_event_properties_t * event_properties, FTB_event_handle_t * event_handle)
{
    return ZOID_FTB_Publish(client_handle, event_name, event_properties, event_handle);
}


int FTB_Subscribe(FTB_subscribe_handle_t * subscribe_handle, FTB_client_handle_t client_handle,
                  const char *subscription_str, int (*callback) (FTB_receive_event_t *, void *),
                  void *arg)
{
    if (callback != NULL)
        return FTB_ERR_NOT_SUPPORTED;
    else
        return ZOID_FTB_Subscribe(subscribe_handle, client_handle, subscription_str);
}


int FTB_Unsubscribe(FTB_subscribe_handle_t * subscribe_handle)
{
    return ZOID_FTB_Unsubscribe(subscribe_handle);
}


int FTB_Poll_event(FTB_subscribe_handle_t subscribe_handle, FTB_receive_event_t * receive_event)
{
    if (receive_event == NULL) {
        return FTB_ERR_NULL_POINTER;
    }
    else
        return ZOID_FTB_Poll_event(subscribe_handle, receive_event);
}


int FTB_Disconnect(FTB_client_handle_t client_handle)
{
    return ZOID_FTB_Disconnect(client_handle);
}


int FTB_Get_event_handle(const FTB_receive_event_t receive_event, FTB_event_handle_t * event_handle)
{
    return ZOID_FTB_Get_event_handle(receive_event, event_handle);
}


int FTB_Compare_event_handles(const FTB_event_handle_t event_handle1,
                              const FTB_event_handle_t event_handle2)
{
    return ZOID_FTB_Compare_event_handles(event_handle1, event_handle2);
}

int FTB_Check_error_code(const int error_code, int *error_class, int *error_value)
{
    return ZOID_FTB_Check_error_code(error_code, error_class, error_value);
}
