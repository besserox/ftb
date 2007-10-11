#ifndef FTB_DEF_H
#define FTB_DEF_H

#include <stdint.h>
#include <unistd.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif 
//#define FTB_DEBUG   0

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

typedef uint8_t FTB_event_catching_t;
typedef char FTB_catching_style_t[32];
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

typedef uint8_t FTB_severity_code_t;
typedef uint16_t FTB_comp_cat_code_t;
typedef uint8_t FTB_comp_code_t;
typedef uint8_t FTB_event_cat_code_t;
typedef uint16_t FTB_event_name_code_t;
typedef uint8_t FTB_tag_len_t;

#define FTB_MAX_REGION               16
#define FTB_MAX_COMP_CAT             32
#define FTB_MAX_COMP                 32
#define FTB_MAX_EVENT_NAME           32
#define FTB_MAX_SEVERITY             32
#define FTB_MAX_EVENT_DESC               1024
#define FTB_MAX_EVENT_DATA           64
#define FTB_MAX_JOBID                16
#define FTB_MAX_INST_NAME            16
#define FTB_MAX_NAMESPACE            78  /* FTB_MAX_REGION - 1 + FTB_MAX_COMP_CAT - 1 + FTB_MAX_COMP -1  + 2 */
#define FTB_MAX_SCHEMA_VER           16
#define FTB_MAX_HOST_NAME              64
#define FTB_MAX_ERRMSG_LEN           1024
#define FTB_EVENT_SIZE               256

typedef char FTB_schema_ver_t[FTB_MAX_SCHEMA_VER];
typedef char FTB_namespace_t[FTB_MAX_NAMESPACE];
typedef char FTB_inst_name_t[FTB_MAX_INST_NAME];
typedef char FTB_jobid_t[FTB_MAX_JOBID];
typedef char FTB_region_t[FTB_MAX_REGION];
typedef char FTB_comp_cat_t[FTB_MAX_COMP_CAT];
typedef char FTB_comp_t[FTB_MAX_COMP];
typedef char FTB_severity_t[FTB_MAX_SEVERITY];
typedef char FTB_event_name_t[FTB_MAX_EVENT_NAME];
typedef char FTB_hostname_t[FTB_MAX_HOST_NAME];
typedef char FTB_event_desc_t[FTB_MAX_EVENT_DESC];

#define FTB_MAX_DYNAMIC_DATA_SIZE   ((FTB_EVENT_SIZE)-sizeof(FTB_severity_code_t)\
    -sizeof(FTB_comp_cat_code_t)-sizeof(FTB_comp_code_t)-sizeof(FTB_event_cat_code_t)\
    -sizeof(FTB_event_name_code_t)-sizeof(FTB_region_t)-sizeof(FTB_jobid_t)\
    -sizeof(FTB_inst_name_t)-sizeof(FTB_hostname_t)-sizeof(int)-sizeof(FTB_tag_len_t)\
    -sizeof(FTB_event_data_t))


typedef struct FTB_comp_info {
    FTB_schema_ver_t schema_ver;
    FTB_namespace_t comp_namespace;
    FTB_inst_name_t inst_name;
    FTB_jobid_t jobid;
    FTB_catching_style_t catch_style; 
}FTB_comp_info_t;

typedef struct FTB_location_id {
    char hostname[FTB_MAX_HOST_NAME];
    pid_t pid;
}FTB_location_id_t;

typedef struct FTB_client_id {
    FTB_comp_cat_code_t comp_cat;
    FTB_comp_code_t comp;
    uint8_t ext;/*extenion field*/
}FTB_client_id_t;

typedef struct FTB_data {
    int datalen;
    char data[FTB_MAX_EVENT_DATA];
}FTB_event_data_t;

/* Reserved component category and component for use */
#define FTB_COMP_MANAGER                  1 
#define FTB_COMP_CAT_BACKPLANE            1

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
//#define FTB_ERR_NOT_INITIALIZED                 (-10)
#define FTB_ERR_MASK_NOT_INITIALIZED            (-12)
#define FTB_ERR_INVALID_VALUE                   (-13)
#define FTB_ERR_HASHKEY_NOT_FOUND               (-14)
#define FTB_ERR_NAMESPACE_FORMAT                (-15)
#define FTB_ERR_VALUE_NOT_FOUND                 (-16)
#define FTB_ERR_INVALID_FIELD                   (-17)


#define FTB_CAUGHT_NO_EVENT                     -31
#define FTB_CAUGHT_EVENT                        1

typedef uint8_t FTB_tag_t;

/*event and event_mask using same structure*/
typedef struct FTB_event{
    FTB_severity_code_t  severity;
    FTB_comp_cat_code_t comp_cat;
    FTB_comp_code_t comp;
    FTB_event_cat_code_t event_cat;
    FTB_event_name_code_t event_name;
    FTB_region_t region;
    FTB_jobid_t jobid;
    FTB_inst_name_t inst_name;
    FTB_hostname_t hostname;
    int seqnum;
    FTB_tag_len_t len;
    FTB_event_data_t event_data;
    char dynamic_data[FTB_MAX_DYNAMIC_DATA_SIZE];
}FTB_event_t;

//typedef FTB_event_t  FTB_catch_event_info_t;
typedef struct FTB_catch_event_info{
    FTB_severity_t  severity;
    FTB_comp_cat_t comp_cat;
    FTB_comp_t comp;
    //FTB_event_cat_t event_cat;
    FTB_event_name_t event_name;
    FTB_region_t region;
    FTB_jobid_t jobid;
    FTB_inst_name_t inst_name;
    FTB_hostname_t hostname;
    int seqnum;
    FTB_tag_len_t len;
    FTB_event_data_t event_data;
    FTB_location_id_t incoming_src;
    char dynamic_data[FTB_MAX_DYNAMIC_DATA_SIZE];
}FTB_catch_event_info_t;

typedef struct FTB_event_mask {
    FTB_event_t event;
    int initialized;
}FTB_event_mask_t;

typedef struct FTB_subscribe_handle {
    FTB_client_handle_t chandle;
    FTB_event_mask_t cmask;
}FTB_subscribe_handle_t;

typedef struct FTB_event_info {
    FTB_namespace_t comp_namespace;
    FTB_event_name_t event_name;
    FTB_severity_t severity;
    FTB_event_desc_t event_desc;    
}FTB_event_info_t;

#ifdef __cplusplus
} /*extern "C"*/
#endif 

#endif /*FTB_DEF_H*/
