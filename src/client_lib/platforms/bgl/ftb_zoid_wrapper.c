/*Compute node libftb wrapper based on ZOID for BG/L*/
#include <string.h>
#include "libftb.h"
#include "ftb_zoid_client.h"

int FTB_Connect(FTB_client_t *comp_info, FTB_client_handle_t *client_handle)
{
    return ZOID_FTB_Connect(comp_info, client_handle); 
}

int FTB_Reg_throw(FTB_client_handle_t handle, const char *event)
{
    return ZOID_FTB_Reg_throw(handle, event);
}

int FTB_Reg_catch_polling_event(FTB_client_handle_t handle, const char *name)
{
    if (name == NULL) {
        return FTB_ERR_NULL_POINTER;
    }
    return ZOID_FTB_Reg_catch_polling_event(handle, name);
}

int FTB_Reg_catch_polling_mask(FTB_client_handle_t handle, const FTB_event_t *event)
{
    if (event == NULL) {
        return FTB_ERR_NULL_POINTER;
    }
    return ZOID_FTB_Reg_catch_polling_mask(handle, event);
}

int FTB_Reg_catch_notify_event(FTB_client_handle_t handle, const char *name, int (*callback)(FTB_event_t *, FTB_id_t *, void*), void *arg)
{
    if (name == NULL) {
        return FTB_ERR_NULL_POINTER;
    }
    return FTB_ERR_NOT_SUPPORTED;
}

int FTB_Reg_catch_notify_mask(FTB_client_handle_t handle, const FTB_event_t *event, int (*callback)(FTB_event_t *, FTB_id_t *, void*), void *arg)
{
    return FTB_ERR_NOT_SUPPORTED;
}

int FTB_Reg_all_predefined_catch(FTB_client_handle_t handle)
{
    return ZOID_FTB_Reg_all_predefined_catch(handle);
}

int FTB_Publish_event(FTB_client_handle_t handle, const char *event, FTB_event_data_t *datadetails, char *error_msg)
{
    return ZOID_FTB_Throw(handle, event, datadetails, error_msg);
}

int FTB_Catch(FTB_client_handle_t handle, FTB_event_t *event, FTB_id_t *src)
{
    if (event == NULL) {
        return FTB_ERR_NULL_POINTER;
    }
    return ZOID_FTB_Catch(handle, event, src);
}

int FTB_Disconnect(FTB_client_handle_t handle)
{
    return ZOID_FTB_Disconnect(handle);
}

int FTB_Abort(FTB_client_handle_t handle)
{
    return ZOID_FTB_Abort(handle);
}

int FTB_Add_dynamic_tag(FTB_client_handle_t handle, FTB_tag_t tag, const char *tag_data, FTB_tag_len_t data_len)
{
    if (tag_data == NULL) {
        return FTB_ERR_NULL_POINTER;
    }
    return ZOID_FTB_Add_dynamic_tag(handle, tag, tag_data, data_len);
}

int FTB_Remove_dynamic_tag(FTB_client_handle_t handle, FTB_tag_t tag)
{
    return ZOID_FTB_Remove_dynamic_tag(handle, tag);
}

int FTB_Read_dynamic_tag(const FTB_event_t *event, FTB_tag_t tag, char *tag_data, FTB_tag_len_t *data_len)
{
    uint8_t tag_count;
    uint8_t i;
    int offset;
    
    /*No need to forward it to zoid*/
    memcpy(&tag_count, event->dynamic_data, sizeof(tag_count));
    offset = 1;
    for (i=0;i<tag_count;i++) {
        FTB_tag_t temp_tag;
        FTB_tag_len_t temp_len;
        memcpy(&temp_tag, event->dynamic_data + offset, sizeof(FTB_tag_t));
        offset+=sizeof(FTB_tag_t);
        memcpy(&temp_len, event->dynamic_data + offset, sizeof(FTB_tag_len_t));
        offset+=sizeof(FTB_tag_len_t);
        if (tag == temp_tag) {
            if (*data_len < temp_len) {
                return FTB_ERR_TAG_NO_SPACE;
            }
            else {
                *data_len = temp_len;
                memcpy(tag_data, event->dynamic_data + offset, temp_len);
                return FTB_SUCCESS;
            }
        }
    }

    return FTB_ERR_TAG_NOT_FOUND;
}

