#include <stdio.h>
#include <string.h>
#include "libftb.h"
#include "ftb_client_lib.h"

FILE* FTBU_log_file_fp;
/*Linux wrapper for client_lib*/


int FTB_Connect(FTB_comp_info_t *comp_info, FTB_client_handle_t *client_handle, char *error_msg)
{
    FTB_component_properties_t properties;
    FTBU_log_file_fp = stderr;
    properties.err_handling = FTB_ERR_HANDLE_NONE;
    if (strcasecmp(comp_info->catch_style, "FTB_POLLING_CATCH") == 0) {
        properties.catching_type= FTB_EVENT_CATCHING_POLLING;
        properties.max_event_queue_size = FTB_DEFAULT_EVENT_POLLING_Q_LEN;
    }
    else if (strcasecmp(comp_info->catch_style, "FTB_NOTIFY_CATCH") == 0) {
        properties.catching_type= FTB_EVENT_CATCHING_NOTIFICATION;
        properties.max_event_queue_size = 0;
    }
    else if (strcasecmp(comp_info->catch_style, "FTB_NO_CATCH") == 0) {
        properties.catching_type= FTB_EVENT_CATCHING_NONE;
        properties.max_event_queue_size = 0;
    }
    else { /* user can also sepecify FTB_POLLING_NOTIFY_CATCH */
        properties.catching_type= FTB_EVENT_CATCHING_POLLING+FTB_EVENT_CATCHING_NOTIFICATION;
        properties.max_event_queue_size = FTB_DEFAULT_EVENT_POLLING_Q_LEN;
    }
    *error_msg=0;
    return FTBC_Connect(comp_info, 0, &properties, client_handle); /*Set extention to 0*/
}

int FTB_Create_mask(FTB_event_mask_t *event_mask, char *field_name, char *field_val, char *error_msg)
{
    *error_msg=0;
    return FTBC_Create_mask(event_mask, field_name, field_val);
}


int FTB_Register_publishable_events(FTB_client_handle_t handle, FTB_event_info_t  *einfo, int num_events, char *error_msg)
{
    *error_msg = 0;
    return FTB_SUCCESS;
}

int FTB_Subscribe(FTB_client_handle_t handle, FTB_event_mask_t *event_mask, FTB_subscribe_handle_t *shandle, char *error_msg, 
        int (*callback)(FTB_catch_event_info_t *, void*), void *arg)
{
    *error_msg=0;
    if ((event_mask == NULL) || (shandle == NULL)){ 
        return FTB_ERR_NULL_POINTER;
    }
    else if (!event_mask->initialized) 
        return FTB_ERR_MASK_NOT_INITIALIZED;
    memcpy(&shandle->chandle, &handle, sizeof(FTB_client_handle_t));
    memcpy(&shandle->cmask, event_mask, sizeof(FTB_event_mask_t));
    if (callback == NULL) { 
        return FTBC_Reg_catch_polling_mask(handle, &event_mask->event);
    }
    else {  
        return FTBC_Reg_catch_notify_mask(handle, &event_mask->event, callback, arg);
    }
}

int FTB_Publish_event(FTB_client_handle_t handle, const char *event, FTB_event_data_t *datadetails, char *error_msg)
{
    *error_msg = 0;
    return FTBC_Throw(handle, event, datadetails);
}

int FTB_Poll_event(FTB_subscribe_handle_t shandle, FTB_catch_event_info_t *event, char *error_msg)
{
    *error_msg=0;
    if (event == NULL) {
        return FTB_ERR_NULL_POINTER;
    }
    else if (shandle.cmask.initialized == 0)
        return FTB_ERR_MASK_NOT_INITIALIZED;
    return FTBC_Poll_event(shandle, event);
}

int FTB_Disconnect(FTB_client_handle_t handle)
{
    return FTBC_Disconnect(handle);
}

int FTB_Add_tag(FTB_client_handle_t handle, FTB_tag_t tag, const char *tag_data, FTB_tag_len_t data_len, char *error_msg)
{
    *error_msg = 0;
    return FTBC_Add_dynamic_tag(handle, tag, tag_data, data_len);
}

int FTB_Remove_tag(FTB_client_handle_t handle, FTB_tag_t tag, char *error_msg)
{
    *error_msg = 0;
    return FTBC_Remove_dynamic_tag(handle, tag);
}

int FTB_Read_tag(const FTB_catch_event_info_t *event, FTB_tag_t tag, char *tag_data, FTB_tag_len_t *data_len, char *error_msg)
{
    *error_msg = 0;
    return FTBC_Read_dynamic_tag(event, tag, tag_data, data_len);
}

/*
int FTB_Reg_throw(FTB_client_handle_t handle, const char *event)
{
    return FTBC_Reg_throw(handle, event);
}
*/

/*
int FTB_Reg_catch_polling_event(FTB_client_handle_t handle, const char *name)
{
    return FTBC_Reg_catch_polling_event(handle, name);
}
*/

/*
int FTB_Reg_catch_polling_mask(FTB_client_handle_t handle, const FTB_event_t *event)
{
    return FTBC_Reg_catch_polling_mask(handle, event);
}
*/
/*
//int FTB_Reg_catch_notify_event(FTB_client_handle_t handle, const char *name, int (*callback)(FTB_event_t *, FTB_id_t *, void*), void *arg)
int FTB_Reg_catch_notify_event(FTB_client_handle_t handle, const char *name, int (*callback)(FTB_catch_event_info_t *, void*), void *arg)
{
    return FTBC_Reg_catch_notify_event(handle, name, callback, arg);
}
*/
/*
//int FTB_Reg_catch_notify_mask(FTB_client_handle_t handle, const FTB_event_t *event, int (*callback)(FTB_event_t *, FTB_id_t *, void*), void *arg)
int FTB_Reg_catch_notify_mask(FTB_client_handle_t handle, const FTB_event_t *event, int (*callback)(FTB_catch_event_info_t *, void*), void *arg)
{
    return FTBC_Reg_catch_notify_mask(handle, event, callback, arg);
}
*/
/*
int FTB_Reg_all_predefined_catch(FTB_client_handle_t handle)
{
    return FTBC_Reg_all_predefined_catch(handle);
}
*/
/*
int FTB_Catch(FTB_client_handle_t handle, FTB_event_t *event, FTB_id_t *src)
{
    return FTBC_Poll_event(handle, event, src);
}
*/
/*
int FTB_Abort(FTB_client_handle_t handle)
{
    return FTBC_Abort(handle);
}
*/

