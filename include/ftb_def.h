#ifndef FTB_DEF_H
#define FTB_DEF_H

#include <stdint.h>
#include <unistd.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif 

#define FTB_MAX_HOST_NAME              64

typedef uint8_t FTB_err_handling_t;
/*
The behavior in case therer is an internal FTB error,
defined as FTB_ERR_HANDLE_NONE, or | of the others
*/
#define FTB_ERR_HANDLE_NONE                      0x0
/*
In case of some error, that FTB client just stop functioning,
all subsequent FTB calls will take no effect and return 
FTB_ERR_GENERAL immediately 
*/
#define FTB_ERR_HANLDE_NOTIFICATION              0x1
/*
If some error happened, FTB will generate an event and next 
time when FTB_Catch is called, that event will be caught by 
client.
*/
#define FTB_ERR_HANDLE_RECOVER                   0x2
/*
If some error happened, FTB will try to recover, but this option
will cause more resource usage and an additional thread in client
process.
*/

typedef uint8_t FTB_event_catching_t;
/*
The mechanism to catch events required by component, 
defined as FTB_EVENT_CATCHING_NONE, or | of the others.
*/
#define FTB_EVENT_CATCHING_NONE                  0x0
/*
This component will not catch any events.
*/
#define FTB_EVENT_CATCHING_POLLING               0x1
/*
This component will catch events by polling, if specified 
a polling queue is constructed
*/
#define FTB_EVENT_CATCHING_NOTIFICATION          0x2
/*
This component will catch event by notification callback functions.
If specified, an additional thread is generated.
*/

typedef uint8_t FTB_severity_t;
typedef uint16_t FTB_comp_cat_t;
typedef uint8_t FTB_comp_t;
typedef uint8_t FTB_event_cat_t;
typedef uint16_t FTB_event_name_t;
typedef uint8_t FTB_dynamic_len_t;

#define FTB_EVENT_SIZE                   64
#define FTB_MAX_DYNAMIC_DATA_SIZE   ((FTB_EVENT_SIZE)-sizeof(FTB_severity_t)\
    -sizeof(FTB_comp_cat_t)-sizeof(FTB_comp_t)-sizeof(FTB_event_cat_t)\
    -sizeof(FTB_event_name_t)-sizeof(FTB_dynamic_len_t))

typedef struct FTB_location_id {
    char hostname[FTB_MAX_HOST_NAME];
    pid_t pid;
}FTB_location_id_t;

typedef struct FTB_client_id {
    FTB_comp_cat_t comp_cat;
    FTB_comp_t comp;
    uint8_t ext;/*extenion field*/
}FTB_client_id_t;

/* Reserved component category and component for use */
#define FTB_COMP_MANAGER                   (1<<0)
#define FTB_COMP_CAT_BACKPLANE            (1<<0)

typedef uint32_t FTB_client_handle_t;

#define FTB_CLIENT_ID_TO_HANDLE(id)  ((id).comp_cat<<16 | (id).comp<<8 | (id).ext)

typedef struct FTB_id {
    FTB_location_id_t location_id;
    FTB_client_id_t client_id;
}FTB_id_t;

#define FTB_DEFAULT_EVENT_POLLING_Q_LEN          64

typedef struct FTB_component_properties {
    FTB_event_catching_t catching_type; 
    FTB_err_handling_t err_handling;
    unsigned int max_event_queue_size;
}FTB_component_properties_t;

#define FTB_SUCCESS                             0
#define FTB_ERR_GENERAL                         (-1)
#define FTB_ERR_NULL_POINTER                    (-2)
#define FTB_ERR_NOT_SUPPORTED                   (-3)
#define FTB_ERR_INVALID_PARAMETER               (-4)
#define FTB_ERR_NETWORK_GENERAL                 (-5)
#define FTB_ERR_NETWORK_NO_ROUTE                (-6)
#define FTB_ERR_TAG_NO_SPACE                    (-7)
#define FTB_ERR_TAG_CONFLICT                    (-8)
#define FTB_ERR_TAG_NOT_FOUND                   (-9)
#define FTB_ERR_NOT_INITIALIZED                 (-2)
#define FTB_ERR_EVENT_NOT_FOUND                 (-16)

#define FTB_CAUGHT_NO_EVENT                     0
#define FTB_CAUGHT_EVENT                        1

typedef uint8_t FTB_tag_t;

/*event and event_mask using same structure*/
typedef struct FTB_event{
    FTB_severity_t  severity;
    FTB_comp_cat_t comp_cat;
    FTB_comp_t comp;
    FTB_event_cat_t event_cat;
    FTB_event_name_t event_name;
    char dynamic_data[FTB_MAX_DYNAMIC_DATA_SIZE];
    FTB_dynamic_len_t len;
}FTB_event_t;


#ifdef __cplusplus
} /*extern "C"*/
#endif 

#endif /*FTB_DEF_H*/
