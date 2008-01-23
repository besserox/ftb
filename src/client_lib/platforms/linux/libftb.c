#include <stdio.h>
#include <string.h>
#include "libftb.h"
#include "ftb_client_lib.h"

/*Linux wrapper for client_lib*/
FILE* FTBU_log_file_fp;

int FTB_Connect(const FTB_client_t *client_info, FTB_client_handle_t *client_handle) {
    FTBU_log_file_fp = stderr;
    return FTBC_Connect(client_info, 0, client_handle); //Set extention to 0
}

int FTB_Publish(FTB_client_handle_t client_handle, const char *event_name,  const FTB_event_properties_t *event_properties, FTB_event_handle_t *event_handle) {
    return FTBC_Publish(client_handle, event_name, event_properties, event_handle);
}

int FTB_Subscribe(FTB_subscribe_handle_t *subscribe_handle, FTB_client_handle_t client_handle, const char *subscription_str, int (*callback)(FTB_receive_event_t *, void*), void *arg) {
    if (callback == NULL)
        return FTBC_Subscribe_with_polling(subscribe_handle, client_handle, subscription_str);
    else 
        return FTBC_Subscribe_with_callback(subscribe_handle, client_handle, subscription_str, callback, arg);
}

int FTB_Unsubscribe(FTB_subscribe_handle_t *subscribe_handle) {
    return FTBC_Unsubscribe(subscribe_handle);
}

int FTB_Declare_publishable_events(FTB_client_handle_t client_handle, int schema_file, const FTB_event_info_t  *einfo, int num_events)
{
    return FTBC_Declare_publishable_events(client_handle, schema_file, einfo, num_events);
}

int FTB_Poll_event(FTB_subscribe_handle_t subscribe_handle, FTB_receive_event_t *receive_event)
{
    if (receive_event == NULL) {
        return FTB_ERR_NULL_POINTER;
    }
    //else if (shandle.cmask.initialized == 0)
    //    return FTB_ERR_MASK_NOT_INITIALIZED;
    return FTBC_Poll_event(subscribe_handle, receive_event);
}

int FTB_Disconnect(FTB_client_handle_t client_handle) {
    return FTBC_Disconnect(client_handle);
}
/*
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

int FTB_Read_tag(const FTB_receive_event_t *event, FTB_tag_t tag, char *tag_data, FTB_tag_len_t *data_len, char *error_msg)
{
    *error_msg = 0;
    return FTBC_Read_dynamic_tag(event, tag, tag_data, data_len);
}*/
