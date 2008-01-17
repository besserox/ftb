#ifndef FTB_CLIENT_LIB_H
#define FTB_CLIENT_LIB_H

#include "ftb_manager_lib.h"
#include "ftb_def.h"
/*

*/
int FTBC_Connect(FTB_client_t *cinfo, uint8_t extension, FTB_client_handle_t *client_handle);

int FTBC_Create_mask(FTB_event_mask_t* event_mask, char *field_name, char *field_val);

int FTBC_Reg_throw(FTB_client_handle_t handle, const char *event);

int FTBC_Reg_catch_polling_mask(FTB_client_handle_t handle, const FTB_event_t *event);

int FTBC_Reg_catch_notify_mask(FTB_client_handle_t handle, const FTB_event_t *event, int (*callback)(FTB_catch_event_info_t *, void*), const void *arg);

int FTBC_Publish(FTB_client_handle_t handle, const char *event_name,  const FTB_event_properties_t *event_properties, FTB_event_handle_t *event_handle);

int FTBC_Poll_event(FTB_subscribe_handle_t  shandle, FTB_catch_event_info_t *event);

int FTBC_Disconnect(FTB_client_handle_t handle);

/*
int FTBC_Add_dynamic_tag(FTB_client_handle_t handle, FTB_tag_t tag, const char *tag_data, FTB_tag_len_t data_len);

int FTBC_Remove_dynamic_tag(FTB_client_handle_t handle, FTB_tag_t tag);

int FTBC_Read_dynamic_tag(const FTB_catch_event_info_t *event, FTB_tag_t tag, char *tag_data, FTB_tag_len_t *data_len);
*/

#endif
