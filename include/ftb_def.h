#ifndef FTB_DEF_H
#define FTB_DEF_H

#include <stdint.h>
#include <unistd.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif 
#define FTB_DEBUG   0

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
#define FTB_ERR_HANDLE_NOTIFICATION              0x1
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

#define FTB_MAX_SUBSCRIPTION_STYLE          32
typedef char FTB_subscription_style_t[FTB_MAX_SUBSCRIPTION_STYLE];     

/* If client will subscribe to any events */
#define FTB_SUBSCRIPTION_NONE               0x0                        
/* If client plans to poll - a polling queue is created */
#define FTB_SUBSCRIPTION_POLLING            0x1                        
/* If client plans to use callback handlers */
#define FTB_SUBSCRIPTION_NOTIFY             0x2           

typedef uint8_t FTB_tag_len_t;

#define FTB_MAX_REGION               16
#define FTB_MAX_COMP_CAT             32
#define FTB_MAX_COMP                 32
#define FTB_MAX_EVENTSPACE           84  /* FTB_MAX_REGION - 1 + FTB_MAX_COMP_CAT - 1 + FTB_MAX_COMP -1  + 2 */
#define FTB_MAX_SEVERITY             16
#define FTB_MAX_EVENT_NAME           32
#define FTB_MAX_CLIENT_JOBID         16
#define FTB_MAX_CLIENT_NAME          16
#define FTB_MAX_HOST_NAME            64
#define FTB_MAX_CLIENTSCHEMA_VER     8
#define FTB_MAX_PAYLOAD_DATA         128
#define FTB_EVENT_SIZE               512

typedef char FTB_eventspace_t[FTB_MAX_EVENTSPACE];
typedef char FTB_client_name_t[FTB_MAX_CLIENT_NAME];
typedef char FTB_client_schema_ver_t[FTB_MAX_CLIENTSCHEMA_VER];
typedef char FTB_client_jobid_t[FTB_MAX_CLIENT_JOBID];
typedef char FTB_region_t[FTB_MAX_REGION];
typedef char FTB_comp_cat_t[FTB_MAX_COMP_CAT];
typedef char FTB_comp_t[FTB_MAX_COMP];
typedef char FTB_severity_t[FTB_MAX_SEVERITY];
typedef char FTB_event_name_t[FTB_MAX_EVENT_NAME];
typedef char FTB_hostname_t[FTB_MAX_HOST_NAME];

#define FTB_MAX_DYNAMIC_DATA_SIZE   ((FTB_EVENT_SIZE)-sizeof(FTB_eventspace_t)-sizeof(FTB_severity_t)\
    -sizeof(FTB_event_name_t)-sizeof(FTB_client_jobid_t)\
    -sizeof(FTB_client_name_t)-sizeof(FTB_hostname_t)\
    -sizeof(FTB_client_schema_ver_t)-sizeof(FTB_tag_len_t)\
    -sizeof(FTB_event_properties_t))


typedef struct FTB_client {
    FTB_client_schema_ver_t client_schema_ver;
    FTB_eventspace_t event_space;
    FTB_client_name_t client_name;
    FTB_client_jobid_t client_jobid;
    FTB_subscription_style_t client_subscription_style; 
    unsigned int client_polling_queue_len;
}FTB_client_t;

typedef struct FTB_location_id {
    char hostname[FTB_MAX_HOST_NAME];
    pid_t pid;
}FTB_location_id_t;

typedef struct FTB_client_id {
    FTB_region_t region;
    FTB_comp_cat_t comp_cat;
    FTB_comp_t comp;
    FTB_client_name_t client_name;
    uint8_t ext;/*extenion field*/
}FTB_client_id_t;


typedef struct FTB_properties {
    char event_type;
    char event_payload[FTB_MAX_PAYLOAD_DATA];
}FTB_event_properties_t;

/* Reserved component category and component for use */
//#define FTB_COMP_MANAGER                  1 
//#define FTB_COMP_CAT_BACKPLANE            1

//typedef uint32_t FTB_client_handle_t;
typedef FTB_client_id_t FTB_client_handle_t;


//#define FTB_CLIENT_ID_TO_HANDLE(id)  ((id).comp_cat<<16 | (id).comp<<8 | (id).ext)

typedef struct FTB_id {
    FTB_location_id_t location_id;
    FTB_client_id_t client_id;
}FTB_id_t;

#define FTB_DEFAULT_POLLING_Q_LEN               64

#define FTB_SUCCESS                             0
#define FTB_ERR_EVENTSPACE_FORMAT               (-15)
#define FTB_ERR_SUBSCRIPTION_STYLE              (-18)
#define FTB_ERR_DUP_CALL                        (-19)
#define FTB_ERR_NULL_POINTER                    (-2)

#define FTB_ERR_INVALID_HANDLE                  (-20)
#define FTB_ERR_GENERAL                         (-1)
#define FTB_ERR_INVALID_EVENT_NAME              (-21)

#define FTB_ERR_NOT_SUPPORTED                   (-3)
#define FTB_ERR_SUBSCRIPTION_STR                (-22)
#define FTB_ERR_FILTER_ATTR                     (-23)
#define FTB_ERR_FILTER_VALUE                    (-24)


#define FTB_ERR_INVALID_PARAMETER               (-4)
#define FTB_ERR_NETWORK_GENERAL                 (-5)
#define FTB_ERR_NETWORK_NO_ROUTE                (-6)
#define FTB_ERR_TAG_NO_SPACE                    (-7)
#define FTB_ERR_TAG_CONFLICT                    (-8)
#define FTB_ERR_TAG_NOT_FOUND                   (-9)
//#define FTB_ERR_NOT_INITIALIZED                 (-10)
#define FTB_ERR_MASK_NOT_INITIALIZED            (-12)
#define FTB_ERR_INVALID_VALUE                   (-13)
#define FTB_ERR_HASHKEY_NOT_FOUND               (-14)
#define FTB_ERR_VALUE_NOT_FOUND                 (-16)
#define FTB_ERR_INVALID_FIELD                   (-17)


#define FTB_GOT_NO_EVENT                     -31
#define FTB_GOT_EVENT                        1

typedef uint8_t FTB_tag_t;

/*event and event_mask using same structure*/
typedef struct FTB_event{
    //FTB_eventspace_t event_space;
    FTB_region_t region;
    FTB_comp_cat_t comp_cat;
    FTB_comp_t comp;
    FTB_event_name_t event_name;
    FTB_severity_t  severity;
    FTB_client_jobid_t client_jobid;
    FTB_client_name_t client_name;
    FTB_hostname_t hostname;
    int seqnum;
    FTB_tag_len_t len;
    FTB_event_properties_t event_properties;
    char dynamic_data[FTB_MAX_DYNAMIC_DATA_SIZE];
}FTB_event_t;

//typedef FTB_event_t  FTB_receive_event_t;
typedef struct FTB_catch_event_info{
    FTB_severity_t  severity;
    FTB_comp_cat_t comp_cat;
    FTB_comp_t comp;
    FTB_region_t region;
    FTB_event_name_t event_name;
    FTB_client_jobid_t client_jobid;
    FTB_client_name_t client_name;
    FTB_hostname_t hostname;
    int seqnum;
    FTB_tag_len_t len;
    FTB_event_properties_t event_properties;
    FTB_location_id_t incoming_src;
    char dynamic_data[FTB_MAX_DYNAMIC_DATA_SIZE];
}FTB_receive_event_t;

typedef struct FTB_event_mask {
    FTB_event_t event;
    int initialized;
}FTB_event_mask_t;

typedef struct FTB_subscribe_handle {
    FTB_client_handle_t client_handle;
    FTB_event_t subscription_event;
}FTB_subscribe_handle_t;

typedef struct FTB_event_info {
    FTB_eventspace_t event_space;
    FTB_event_name_t event_name;
    FTB_severity_t severity;
    //FTB_event_desc_t event_desc;    
}FTB_event_info_t;

typedef struct FTB_event FTB_event_handle_t; 

#ifdef __cplusplus
} /*extern "C"*/
#endif 

#endif /*FTB_DEF_H*/
