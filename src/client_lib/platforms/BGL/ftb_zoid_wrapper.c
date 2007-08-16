/*Compute node libftb wrapper based on ZOID for BG/L*/

#include "libftb.h"
#include "ftb_zoid_client.h"

int FTB_Init(FTB_comp_ctgy_t category, FTB_comp_t component, const FTB_component_properties_t *properties, FTB_client_handle_t *client_handle)
{
    return ZOID_FTB_Init(category, component, properties, client_handle); 
}

int FTB_Reg_throw(FTB_client_handle_t handle, FTB_event_name_t event)
{
    return ZOID_FTB_Reg_throw(handle, event);
}

int FTB_Reg_catch_polling_event(FTB_client_handle_t handle, FTB_event_name_t event)
{
    return ZOID_FTB_Reg_catch_polling_event(handle, event);
}

int FTB_Reg_catch_polling_mask(FTB_client_handle_t handle, const FTB_event_t *event)
{
    if (event == NULL) {
        return FTB_ERR_NULL_POINTER;
    }
    return ZOID_FTB_Reg_catch_polling_mask(handle, event);
}

int FTB_Reg_catch_notify_event(FTB_client_handle_t handle, FTB_event_name_t event, int (*callback)(FTB_event_t *, void*), void *arg)
{
    return FTB_ERR_NOT_SUPPORTED;
}

int FTB_Reg_catch_notify_mask(FTB_client_handle_t handle, const FTB_event_t *event, int (*callback)(FTB_event_t *, void*), void *arg)
{
    return FTB_ERR_NOT_SUPPORTED;
}

int FTB_Throw(FTB_client_handle_t handle, FTB_event_name_t event)
{
    return ZOID_FTB_Throw(handle, event);
}

int FTB_Catch(FTB_client_handle_t handle, FTB_event_t *event)
{
    if (event == NULL) {
        return FTB_ERR_NULL_POINTER;
    }
    return ZOID_FTB_Catch(handle, event);
}

int FTB_Finalize(FTB_client_handle_t handle)
{
    return ZOID_FTB_Finalize(handle);
}

int FTB_Abort(FTB_client_handle_t handle)
{
    return ZOID_FTB_Abort(handle);
}

