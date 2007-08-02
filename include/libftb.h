#ifndef LIBFTB_H
#define LIBFTB_H

#include "ftb.h"

#ifdef __cplusplus
extern "C" {
#endif 

#define FTB_DEFAULT_EVENT_QUEUE_SIZE  128

/* FTB_Init 
    Initialize ftb_client and connect to ftb server.
    Note if properties->polling_only != 0, no new thread will be created, otherwise a thread will be created for callback
*/
int FTB_Init(const FTB_component_properties_t *properties);

/* FTB_Reg_throw_id & FTB_Reg_throw
    Claim this component will throw a specific event in right condition. It is optional but it allows other components to cooperate better.
    
    Two veriations are specifying event_id (event_name macro, only for built-in events) or an event structure (for additional events).
    For additional events, developer can use FTB_get_<component name>_event_by_id to get the event structure.
*/
int FTB_Reg_throw_id(uint32_t event_id);

int FTB_Reg_throw(const FTB_event_t *event);

#ifndef FTB_CLIENT_POLLING_ONLY
/* FTB_Reg_catch_notify & FTB_Reg_catch_notify_single & FTB_Reg_catch_notify_all
    Register to catch events using notification mechanism. Event will be immediately sent to client and the callback function will be 
    called from another thread. These interfaces can only be used when linked with libftb.so, and the polling_only is set to 0. 
    If multiple catch conditions match, only one callback (the one registered last) will be called.

    Three variations are for single event, for a set of event specified by event_mask, and for all events.
*/
int FTB_Reg_catch_notify_single(uint32_t event_id, int (*callback)(FTB_event_inst_t *, void*), void *arg);

int FTB_Reg_catch_notify(const FTB_event_mask_t *event_mask, int (*callback)(FTB_event_inst_t *, void*), const void *arg);

int FTB_Reg_catch_notify_all(int (*callback)(FTB_event_inst_t *, void*), void *arg);
#endif

/* FTB_Reg_catch_polling & FTB_Reg_catch_polling_single & FTB_Reg_catch_polling_all
    Register to catch events using polling mechanism. Event will be buffered in server to polled by client. If the queue size (specified 
    by polling_queue_len) is full before client polls for event, the oldest events will be discard.

    Three variations are for single event, for a set of event specified by event_mask, and for all events.
*/
int FTB_Reg_catch_polling_single(uint32_t event_id);

int FTB_Reg_catch_polling(const FTB_event_mask_t *event_mask);

int FTB_Reg_catch_polling_all();

/* FTB_Throw_id & FTB_Throw
    Throw an event to ftb_server.
    
    Two veriations are specifying event_id (event_name macro, only for built-in events) or an event structure (for additional events).
    For additional events, developer can use FTB_get_<component name>_event_by_id to get the event structure.
*/
int FTB_Throw_id(uint32_t event_id, const char *data, uint32_t data_len);

int FTB_Throw(const FTB_event_t *event, const char *data, uint32_t data_len);

/* FTB_Catch
    Poll for an event instance from server.
    Parameter input: structure to hold the event instance
    Parameter output: filled structure with the polled event instance
    Return value: 0 when an event instance is caught. non-zero when there is no event in the queue or failed to catch.
*/
int FTB_Catch(FTB_event_inst_t *event_inst);

/* FTB_List_events
    List the events registered to be thrown by all components,

    Parameters input: an array of the events and the pointer to length of the array.
    Parameters output: filled array of the events and the updated length.
*/
int FTB_List_events(FTB_event_t *events, int *len);

/* FTB_Finalize
    Finalize ftb client
*/
int FTB_Finalize();

#ifdef __cplusplus
} /*extern "C"*/
#endif 

#endif
