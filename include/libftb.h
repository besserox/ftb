#ifndef LIBFTB_H
#define LIBFTB_H

#include "ftb_def.h"
#include "ftb_manager_lib.h"

#ifdef __cplusplus
extern "C" {
#endif 
/*
Event mask macros
*/
#define FTB_EVENT_CLR_ALL(evt_mask)    FTBM_EVENT_CLR_ALL(evt_mask) 

#define FTB_EVENT_CLR_SEVERITY(evt_mask)  FTBM_EVENT_CLR_SEVERITY(evt_mask)

#define FTB_EVENT_CLR_COMP_CTGY(evt_mask)  FTBM_EVENT_CLR_COMP_CTGY(evt_mask) 

#define FTB_EVENT_CLR_COMP(evt_mask)  FTBM_EVENT_CLR_COMP(evt_mask) 

#define FTB_EVENT_CLR_EVENT_CTGY(evt_mask)  FTBM_EVENT_CLR_EVENT_CTGY(evt_mask) 

#define FTB_EVENT_CLR_EVENT_NAME(evt_mask) FTBM_EVENT_CLR_EVENT_NAME(evt_mask) 
    
#define FTB_EVENT_MASK_ALL(evt_mask)  FTBM_EVENT_MASK_ALL(evt_mask) 
    
#define FTB_EVENT_MARK_ALL_SEVERITY(evt_mask)  FTBM_EVENT_MARK_ALL_SEVERITY(evt_mask) 

#define FTB_EVENT_MARK_ALL_COMP_CTGY(evt_mask)  FTBM_EVENT_MARK_ALL_COMP_CTGY(evt_mask) 

#define FTB_EVENT_MARK_ALL_COMP(evt_mask)  FTBM_EVENT_MARK_ALL_COMP(evt_mask) 

#define FTB_EVENT_MARK_ALL_EVENT_CTGY(evt_mask)  FTBM_EVENT_MARK_ALL_EVENT_CTGY(evt_mask) 

#define FTB_EVENT_MARK_ALL_EVENT_NAME(evt_mask)  FTBM_EVENT_MARK_ALL_EVENT_NAME(evt_mask) 
    
#define FTB_EVENT_MARK_SEVERITY(evt_mask, val)  FTBM_EVENT_MARK_SEVERITY(evt_mask, val) 

#define FTB_EVENT_MARK_COMP_CTGY(evt_mask, val)  FTBM_EVENT_MARK_ALL_COMP_CTGY(evt_mask, val) 

#define FTB_EVENT_MARK_COMP(evt_mask, val)  FTBM_EVENT_MARK_ALL_COMP(evt_mask, val) 

#define FTB_EVENT_MARK_EVENT_CTGY(evt_mask, val)  FTBM_EVENT_MARK_ALL_EVENT_CTGY(evt_mask, val) 

#define FTB_EVENT_SET_EVENT_NAME(evt_mask, val)  FTBM_EVENT_SET_EVENT_NAME(evt_mask, val) 

/*
TOADD: Dynamic fields macros
*/

/* 
    FTB_Init 
    Initialize FTB functionality.
    If properties == NULL, it will use a lightweight default configuration: 
    FTB_ERR_HANDLE_NONE + FTB_EVENT_CATCHING_POLLING
*/
int FTB_Init(FTB_comp_ctgy_t category, FTB_comp_t component, const FTB_component_properties_t *properties, FTB_client_handle_t *client_handle);

/*
    FTB_Reg_throw
    Claim this component will throw a specific event in right condition. It is optional
 */
int FTB_Reg_throw(FTB_client_handle_t handle, FTB_event_name_t event);

/*
    FTB_Reg_catch
    Register for catching certain events using polling or callback function. 
    Note that callback funciton scheme only works when catching_type has FTB_EVENT_CATCHING_NOTIFICATION.
    User can choose to use an event name or an event mask. 
    Same event is only reported once: 
        If an event matches both polling and notification, the event will be notified and can't be polled later
        If an event matches more than one masks in notification, only the callback of the first matched mask will be called.
*/
int FTB_Reg_catch_polling_event(FTB_client_handle_t handle, FTB_event_name_t event);

int FTB_Reg_catch_polling_mask(FTB_client_handle_t handle, const FTB_event_t *event);

int FTB_Reg_catch_notify_event(FTB_client_handle_t handle, FTB_event_name_t event, int (*callback)(FTB_event_t *, void*), void *arg);

int FTB_Reg_catch_notify_mask(FTB_client_handle_t handle, const FTB_event_t *event, int (*callback)(FTB_event_t *, void*), void *arg);

/* 
    FTB_Throw
    Throw an event to FTB framework.
*/
int FTB_Throw(FTB_client_handle_t handle, FTB_event_name_t event);

/* 
    FTB_Catch
    Poll for an event instance from server.
    Return value: FTB_CAUGHT_NO_EVENT or FTB_CAUGHT_EVENT
*/
int FTB_Catch(FTB_client_handle_t handle, FTB_event_t *event);

/* 
    FTB_Finalize
    Finalize FTB in a normal way
*/
int FTB_Finalize(FTB_client_handle_t handle);

/* 
    FTB_Abort
    Abort FTB in a case of a failure or emergency
*/
int FTB_Abort(FTB_client_handle_t handle);

#ifdef __cplusplus
} /*extern "C"*/
#endif 

#endif
