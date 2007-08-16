#include <stdio.h>
#include "libftb.h"
#include "ftb_client_lib.h"

FILE* FTBU_log_file_fp;
/*Linux wrapper for client_lib*/

int FTB_Init(const FTB_comp_ctgy_t category, const FTB_comp_t component, const FTB_component_properties_t *properties, FTB_client_handle_t *client_handle)
{
    FTBU_log_file_fp = stderr;
    return FTBC_Init(category, component, 0, properties, client_handle); /*Set extention to 0*/
}

int FTB_Reg_throw(FTB_client_handle_t handle, const FTB_event_name_t event)
{
    return FTBC_Reg_throw(handle, event);
}

int FTB_Reg_catch_polling_event(FTB_client_handle_t handle, FTB_event_name_t event)
{
    return FTBC_Reg_catch_polling_event(handle, event);
}

int FTB_Reg_catch_polling_mask(FTB_client_handle_t handle, const FTB_event_t *event)
{
    return FTBC_Reg_catch_polling_mask(handle, event);
}

int FTB_Reg_catch_notify_event(FTB_client_handle_t handle, FTB_event_name_t event, int (*callback)(FTB_event_t *, void*), void *arg)
{
    return FTBC_Reg_catch_notify_event(handle, event, callback, arg);
}

int FTB_Reg_catch_notify_mask(FTB_client_handle_t handle, const FTB_event_t *event, int (*callback)(FTB_event_t *, void*), void *arg)
{
    return FTBC_Reg_catch_notify_mask(handle, event, callback, arg);
}

int FTB_Throw(FTB_client_handle_t handle, FTB_event_name_t event)
{
    return FTBC_Throw(handle, event);
}

int FTB_Catch(FTB_client_handle_t handle, FTB_event_t *event)
{
    return FTBC_Catch(handle, event);
}

int FTB_Finalize(FTB_client_handle_t handle)
{
    return FTBC_Finalize(handle);
}

int FTB_Abort(FTB_client_handle_t handle)
{
    return FTBC_Abort(handle);
}

