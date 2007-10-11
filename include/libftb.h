#ifndef LIBFTB_H
#define LIBFTB_H

#include "ftb_def.h"
#include "ftb_manager_lib.h"

#ifdef __cplusplus
extern "C" {
#endif 

int FTB_Init(FTB_comp_info_t *comp_info, FTB_client_handle_t *client_handle, char *error_msg);

int FTB_Create_mask(FTB_event_mask_t *event_mask, char *field_name, char *field_val, char *error_msg);

int FTB_Register_publishable_events(FTB_client_handle_t handle, FTB_event_info_t  *einfo, int num_events, char *error_msg);

int FTB_Subscribe(FTB_client_handle_t handle, FTB_event_mask_t *event_mask, FTB_subscribe_handle_t *shandle, char *error_msg, int (*callback)(FTB_catch_event_info_t *, void*), void *arg);

int FTB_Publish_event(FTB_client_handle_t handle, const char *event, FTB_event_data_t *datadetails, char *error_msg);

int FTB_Poll_for_event(FTB_subscribe_handle_t shandle, FTB_catch_event_info_t *event, char *error_msg);

int FTB_Finalize(FTB_client_handle_t handle);

/*
 *    FTB_Add_dynamic_tag, FTB_Remove_dynamic_tag, & FTB_Read_dynamic_tag
 *    Provide a simple mechanism to stamp some dynamic info, such as job id, onto the event thrown from a same client.
 *    
 *    FTB_Add_dynamic_tag tells the FTB client library to stamp a tag on any events it throws out later. It will affect events
 *    thrown by all components linked with that client library. 
 *    On success it returns FTB_SUCCESS
 *    If there is not enough space for the tag, FTB_ERR_TAG_NO_SPACE is returned.
 *    If the same tag already exists and it belongs to the same component (same client_handle), the tag data will get updated.
 *    If the same tag is added by another component, FTB_ERR_TAG_CONFLICT is returned.
 *
 *    FTB_Remove_dynamic_tag removes the previously added tags.
 *    On success it returns FTB_SUCCESS
 *    If the tag is not found, FTB_ERR_TAG_NOT_FOUND is returned.
 *    If the same tag is added by another component, FTB_ERR_TAG_CONFLICT is returned.
 *
 *    FTB_Read_dynamic_tag gets the value of a specific tag from an event.    
 *    On success it returns FTB_SUCCESS, and the data len will be updated to the actual size of tag data.
 *    If there is no such tag with the event, FTB_ERR_TAG_NOT_FOUND is returned.
 *    If the data_len passed in is smaller than the actual data, FTB_ERR_TAG_NO_SPACE is returned.
 */
int FTB_Add_tag(FTB_client_handle_t handle, FTB_tag_t tag, const char *tag_data, FTB_tag_len_t data_len, char *error_msg);

int FTB_Remove_tag(FTB_client_handle_t handle, FTB_tag_t tag, char *error_msg);

int FTB_Read_tag(const FTB_catch_event_info_t *event, FTB_tag_t tag, char *tag_data, FTB_tag_len_t *data_len, char *error_msg);

#ifdef __cplusplus
} /*extern "C"*/
#endif 

#endif
