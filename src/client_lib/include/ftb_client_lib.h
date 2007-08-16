#ifndef FTB_CLIENT_LIB_H
#define FTB_CLIENT_LIB_H

#include "ftb_manager_lib.h"
#include "ftb_def.h"
/*

*/

int FTBC_Init(const FTB_comp_ctgy_t category, const FTB_comp_t component, uint8_t extension, const FTB_component_properties_t *properties, FTB_client_handle_t *client_handle);

int FTBC_Reg_throw(FTB_client_handle_t handle, const FTB_event_name_t event);

int FTBC_Reg_catch_polling_event(FTB_client_handle_t handle, FTB_event_name_t event);

int FTBC_Reg_catch_polling_mask(FTB_client_handle_t handle, const FTB_event_t *event);

int FTBC_Reg_catch_notify_event(FTB_client_handle_t handle, FTB_event_name_t event, int (*callback)(FTB_event_t *, void*), void *arg);

int FTBC_Reg_catch_notify_mask(FTB_client_handle_t handle, const FTB_event_t *event, int (*callback)(FTB_event_t *, void*), const void *arg);

int FTBC_Throw(FTB_client_handle_t handle, FTB_event_name_t event);

int FTBC_Catch(FTB_client_handle_t handle, FTB_event_t *event);

int FTBC_Finalize(FTB_client_handle_t handle);

int FTBC_Abort(FTB_client_handle_t handle);


#endif
