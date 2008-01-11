#ifndef FTB_CLIENT_LIB_H
#define FTB_CLIENT_LIB_H

#include "ftb_manager_lib.h"
#include "ftb_def.h"
/*

*/

int FTBC_Connect(const FTB_comp_info_t *comp_info, uint8_t extension, const FTB_component_properties_t *properties, FTB_client_handle_t *client_handle);

int FTBC_Create_mask(FTB_event_mask_t* event_mask, char *field_name, char *field_val);

int FTBC_Reg_throw(FTB_client_handle_t handle, const char *event);

int FTBC_Reg_catch_polling_event(FTB_client_handle_t handle, const char *event);

int FTBC_Reg_catch_polling_mask(FTB_client_handle_t handle, const FTB_event_t *event);

//int FTBC_Reg_catch_notify_event(FTB_client_handle_t handle, const char *event, int (*callback)(FTB_event_t *, FTB_id_t *, void*), void *arg);
int FTBC_Reg_catch_notify_event(FTB_client_handle_t handle, const char *event, int (*callback)(FTB_catch_event_info_t *, void*), void *arg);

//int FTBC_Reg_catch_notify_mask(FTB_client_handle_t handle, const FTB_event_t *event, int (*callback)(FTB_event_t *, FTB_id_t *, void*), const void *arg);
int FTBC_Reg_catch_notify_mask(FTB_client_handle_t handle, const FTB_event_t *event, int (*callback)(FTB_catch_event_info_t *, void*), const void *arg);

int FTBC_Reg_all_predefined_catch(FTB_client_handle_t handle);

int FTBC_Throw(FTB_client_handle_t handle, const char *event, FTB_event_data_t *datadetails);

//int FTBC_Catch(FTB_client_handle_t handle, FTB_event_t *event, FTB_id_t *src);
int FTBC_Poll_event(FTB_subscribe_handle_t  shandle, FTB_catch_event_info_t *event);

int FTBC_Disconnect(FTB_client_handle_t handle);

int FTBC_Abort(FTB_client_handle_t handle);

int FTBC_Add_dynamic_tag(FTB_client_handle_t handle, FTB_tag_t tag, const char *tag_data, FTB_tag_len_t data_len);

int FTBC_Remove_dynamic_tag(FTB_client_handle_t handle, FTB_tag_t tag);

int FTBC_Read_dynamic_tag(const FTB_catch_event_info_t *event, FTB_tag_t tag, char *tag_data, FTB_tag_len_t *data_len);

#endif
