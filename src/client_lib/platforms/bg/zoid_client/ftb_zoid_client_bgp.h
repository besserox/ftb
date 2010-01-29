/**********************************************************************************/
/* This file is part of FTB (Fault Tolerance Backplance) - the core of CIFTS
 * (Co-ordinated Infrastructure for Fault Tolerant Systems)
 * 
 * See http://www.mcs.anl.gov/research/cifts for more information.
 *  
 */
/* This software is licensed under BSD. See the file FTB/misc/license.BSD for
 * complete details on your rights to copy, modify, and use this software.
 */
/*********************************************************************************/
#ifndef FTB_ZOID_CLIENT_H
#define FTB_ZOID_CLIENT_H

#include <stdio.h>
#include <stdlib.h>
#include "zoid_api.h"

#include "ftb_client_lib_defs.h"
#include "ftb_def.h"

/* START-ZOID-SCANNER ID=10 INIT=ftb_zoid_client_init FINI=ftb_zoid_client_fini PROC=NULL */

int ZOID_FTB_Connect(const FTB_client_t * client_info /* in:ptr */ ,
                     FTB_client_handle_t * client_handle /* out:ptr */);


int ZOID_FTB_Publish(FTB_client_handle_t client_handle /* in:obj */ ,
                     const char *event_name /* in:str */ ,
                     const FTB_event_properties_t * event_properties /* in:ptr:nullok */ ,
                     FTB_event_handle_t * event_handle /* out:ptr */);


int ZOID_FTB_Subscribe(FTB_subscribe_handle_t * subscribe_handle /* out:ptr */ ,
                       FTB_client_handle_t client_handle /* in:obj */ ,
                       const char *subscription_str /* in:str */);


int ZOID_FTB_Unsubscribe(FTB_subscribe_handle_t * subscribe_handle /* in:ptr */);


int ZOID_FTB_Declare_publishable_events(FTB_client_handle_t client_handle /* in:obj */ ,
                                        const char *schema_file /* in:str:nullok */ ,
                                        const FTB_event_info_t * einfo /* in:arr:size=+1 */ ,
                                        int num_events /* in:obj */);


int ZOID_FTB_Poll_event(FTB_subscribe_handle_t subscribe_handle /* in:obj */ ,
                        FTB_receive_event_t * receive_event /* out:ptr */);


int ZOID_FTB_Disconnect(FTB_client_handle_t client_handle /* in:obj */);


int ZOID_FTB_Get_event_handle(const FTB_receive_event_t receive_event /* in:obj */ ,
                              FTB_event_handle_t * event_handle /* out:ptr */);


int ZOID_FTB_Compare_event_handles(const FTB_event_handle_t event_handle1 /* in:obj */ ,
                                   const FTB_event_handle_t event_handle2 /* in:obj */);

#endif /* FTB_ZOID_CLIENT_H */
