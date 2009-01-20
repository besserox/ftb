/**********************************************************************************/
/* FTB:ftb-info */
/* This file is part of FTB (Fault Tolerance Backplance) - the core of CIFTS
 * (Co-ordinated Infrastructure for Fault Tolerant Systems)
 *
 * See http://www.mcs.anl.gov/research/cifts for more information.
 * 	
 */
/* FTB:ftb-info */
/* FTB:ftb-fillin */
/* FTB_Version: 0.6
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
/*********************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ftb_def.h"
#include "ftb_util.h"
#include "ftb_client_lib.h"
#include "zoid_api.h"

FILE *FTBU_log_file_fp;

static int nclients;

void ftb_zoid_client_init(int count)
{
    nclients = count;
#ifdef FTB_DEBUG
    {
        char filename[128], IP[32], TIME[32];
        FTBU_get_output_of_cmd("grep ^BG_IP /proc/personality.sh | cut -f2 -d=", IP, 32);
        FTBU_get_output_of_cmd("date +%m-%d-%H-%M-%S", TIME, 32);
        sprintf(filename, "%s/%s.%s.%s", FTB_LOGDIR, "zoid_output", IP, TIME);
        FTBU_log_file_fp = fopen(filename, "w");
        fprintf(FTBU_log_file_fp, "Begin log file\n");
        fflush(FTBU_log_file_fp);
    }
#else
    FTBU_log_file_fp = stderr;
#endif
}


void ftb_zoid_client_fini()
{
#ifdef FTB_DEBUG
    fclose(FTBU_log_file_fp);
#endif
}


int ZOID_FTB_Connect(const FTB_client_t * client_info /* in:ptr */ ,
                     FTB_client_handle_t * client_handle /* out:ptr */)
{
    int proc_id = __zoid_calling_process_id();

    /*
     * For BGL systems, subscription_style of FTB_SUBSCRIPTION_NOTIFY
     * and FTB_SUBSCRIPTION_BOTH is not supported
     */
    if ((strcasecmp(client_info->client_subscription_style, "FTB_SUBSCRIPTION_NOTIFY") == 0)
        || (strcasecmp(client_info->client_subscription_style, "FTB_SUBSCRIPTION_BOTH") == 0)) {
        return FTB_ERR_NOT_SUPPORTED;
    }
    return FTBC_Connect(client_info, proc_id, client_handle);
}


int ZOID_FTB_Publish(FTB_client_handle_t client_handle /* in:obj */ ,
                     const char *event_name /* in:str */ ,
                     const FTB_event_properties_t * event_properties /* in:ptr:nullok */ ,
                     FTB_event_handle_t * event_handle /* out:ptr */)
{
    return FTBC_Publish(client_handle, event_name, event_properties, event_handle);
}


int ZOID_FTB_Subscribe(FTB_subscribe_handle_t * subscribe_handle /* out:ptr */ ,
                       FTB_client_handle_t client_handle /* in:obj */ ,
                       const char *subscription_str /* in:str */)
{
    return FTBC_Subscribe_with_polling(subscribe_handle, client_handle, subscription_str);
}


int ZOID_FTB_Unsubscribe(FTB_subscribe_handle_t * subscribe_handle /* in:ptr */)
{
    return FTBC_Unsubscribe(subscribe_handle);
}


int ZOID_FTB_Declare_publishable_events(FTB_client_handle_t client_handle /* in:obj */ ,
                                        const char *schema_file /* in:str:nullok */ ,
                                        const FTB_event_info_t * einfo /* in:arr:size=+1 */ ,
                                        int num_events /* in:obj */)
{
    return FTBC_Declare_publishable_events(client_handle, schema_file, einfo, num_events);
}


int ZOID_FTB_Poll_event(FTB_subscribe_handle_t subscribe_handle /* in:obj */ ,
                        FTB_receive_event_t * receive_event /* out:ptr */)
{
    return FTBC_Poll_event(subscribe_handle, receive_event);

}


int ZOID_FTB_Disconnect(FTB_client_handle_t client_handle /* in:obj */)
{
    return FTBC_Disconnect(client_handle);
}


int ZOID_FTB_Get_event_handle(const FTB_receive_event_t receive_event /* in:obj */ ,
                              FTB_event_handle_t * event_handle /* out:ptr */)
{
    return FTBC_Get_event_handle(receive_event, event_handle);
}


int ZOID_FTB_Compare_event_handles(const FTB_event_handle_t event_handle1 /* in:obj */ ,
                                   const FTB_event_handle_t event_handle2 /* in:obj */)
{
    return FTBC_Compare_event_handles(event_handle1, event_handle2);
}
