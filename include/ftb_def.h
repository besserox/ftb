#ifndef FTB_DEF_H
#define FTB_DEF_H

#include <stdint.h>
#include <unistd.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif 
#define FTB_DEBUG   0


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

#define FTB_ERR_DUP_EVENT                       (-25)

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

#define FTB_GOT_NO_EVENT                        (-31)
#define FTB_ERR_INVALID_SCHEMA_FILE             (-32)
#define FTB_FAILURE                             (-33)

/* If client will subscribe to any events */
#define FTB_SUBSCRIPTION_NONE               0x0                        
/* If client plans to poll - a polling queue is created */
#define FTB_SUBSCRIPTION_POLLING            0x1                        
/* If client plans to use callback handlers */
#define FTB_SUBSCRIPTION_NOTIFY             0x2           

#define FTB_DEFAULT_POLLING_Q_LEN               64
#define FTB_MAX_SUBSCRIPTION_STYLE   32
#define FTB_MAX_EVENTSPACE           84 
#define FTB_MAX_SEVERITY             16
#define FTB_MAX_EVENT_NAME           32
#define FTB_MAX_CLIENT_JOBID         16
#define FTB_MAX_CLIENT_NAME          16
#define FTB_MAX_HOST_NAME            64
#define FTB_MAX_CLIENTSCHEMA_VER     8
#define FTB_MAX_PAYLOAD_DATA         220
#define FTB_EVENT_SIZE               512
#define FTB_MAX_DYNAMIC_DATA_SIZE   ((FTB_EVENT_SIZE)-sizeof(FTB_eventspace_t)\
        -sizeof(FTB_event_name_t)\
        -sizeof(FTB_severity_t)\
        -sizeof(FTB_client_jobid_t)\
        -sizeof(FTB_client_name_t)\
        -sizeof(FTB_hostname_t)\
        -sizeof(int)\
        -sizeof(FTB_tag_len_t)\
        -sizeof(FTB_event_properties_t))

typedef char FTB_eventspace_t[FTB_MAX_EVENTSPACE];
typedef char FTB_client_name_t[FTB_MAX_CLIENT_NAME];
typedef char FTB_client_schema_ver_t[FTB_MAX_CLIENTSCHEMA_VER];
typedef char FTB_client_jobid_t[FTB_MAX_CLIENT_JOBID];
typedef char FTB_severity_t[FTB_MAX_SEVERITY];
typedef char FTB_event_name_t[FTB_MAX_EVENT_NAME];
typedef char FTB_hostname_t[FTB_MAX_HOST_NAME];
typedef char FTB_subscription_style_t[FTB_MAX_SUBSCRIPTION_STYLE];     
typedef uint8_t FTB_tag_t;
typedef uint8_t FTB_tag_len_t;

typedef struct FTB_client {
    FTB_client_schema_ver_t client_schema_ver;
    FTB_eventspace_t event_space;
    FTB_client_name_t client_name;
    FTB_client_jobid_t client_jobid;
    FTB_subscription_style_t client_subscription_style; 
    unsigned int client_polling_queue_len;
}FTB_client_t;

typedef struct FTB_event_info {
    FTB_event_name_t event_name;
    FTB_severity_t severity;
}FTB_event_info_t;

typedef struct FTB_event_properties {
    char event_type;
    char event_payload[FTB_MAX_PAYLOAD_DATA];
}FTB_event_properties_t;

typedef struct FTB_location_id {
    char hostname[FTB_MAX_HOST_NAME];
    pid_t pid;
}FTB_location_id_t;

typedef struct FTB_receive_event_info {
    FTB_eventspace_t event_space;
    FTB_event_name_t event_name;
    FTB_severity_t  severity;
    FTB_client_jobid_t client_jobid;
    FTB_client_name_t client_name;
    int client_extension;
    int seqnum;
    FTB_event_properties_t event_properties;
    FTB_location_id_t incoming_src;
    FTB_tag_len_t len;
    char dynamic_data[FTB_MAX_DYNAMIC_DATA_SIZE]; /*This will be about 32 bytes only*/
}FTB_receive_event_t;

typedef struct FTB_client_handle FTB_client_handle_t;
typedef struct FTB_subscribe_handle FTB_subscribe_handle_t;
typedef struct FTB_event_handle FTB_event_handle_t;

#ifdef __cplusplus
} /*extern "C"*/
#endif 

#endif /*FTB_DEF_H*/
