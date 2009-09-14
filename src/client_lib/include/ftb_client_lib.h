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
#ifndef FTB_CLIENT_LIB_H
#define FTB_CLIENT_LIB_H

#include "ftb_manager_lib.h"
#include "ftb_def.h"

int FTBC_Connect(const FTB_client_t * cinfo, uint8_t extension, FTB_client_handle_t * client_handle);


int FTBC_Declare_publishable_events(FTB_client_handle_t client_handle, const char *schema_file,
                                    const FTB_event_info_t * einfo, int num_events);


int FTBC_Publish(FTB_client_handle_t client_handle, const char *event_name,
                 const FTB_event_properties_t * event_properties, FTB_event_handle_t * event_handle);


int FTBC_Subscribe_with_polling(FTB_subscribe_handle_t * subscribe_handle,
                                FTB_client_handle_t client_handle, const char *subscription_str);


int FTBC_Subscribe_with_callback(FTB_subscribe_handle_t * subscribe_handle, FTB_client_handle_t handle,
                                 const char *subscription_str, int (*callback) (FTB_receive_event_t *,
                                                                                void *),
                                 const void *arg);


int FTBC_Unsubscribe(FTB_subscribe_handle_t * subscribe_handle);


int FTBC_Poll_event(FTB_subscribe_handle_t subscribe_handle, FTB_receive_event_t * receive_event);


int FTBC_Reg_throw(FTB_client_handle_t handle, const char *event);


int FTBC_Disconnect(FTB_client_handle_t handle);


int FTBC_Get_event_handle(const FTB_receive_event_t receive_event, FTB_event_handle_t * event_handle);


int FTBC_Compare_event_handles(const FTB_event_handle_t event_handle1,
                               const FTB_event_handle_t event_handle2);


#ifdef FTB_TAG
int FTBC_Add_dynamic_tag(FTB_client_handle_t handle, FTB_tag_t tag, const char *tag_data,
                         FTB_tag_len_t data_len);


int FTBC_Remove_dynamic_tag(FTB_client_handle_t handle, FTB_tag_t tag);


int FTBC_Read_dynamic_tag(const FTB_receive_event_t * event, FTB_tag_t tag, char *tag_data,
                          FTB_tag_len_t * data_len);
#endif

#endif
