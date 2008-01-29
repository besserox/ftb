#ifndef FTB_CLIENT_LIB_DEFS_H
#define FTB_CLIENT_LIB_DEFS_H

#include <stdint.h>
#include <unistd.h>
#include <sys/types.h>
#include "ftb_def.h"

#ifdef __cplusplus
extern "C" {
#endif 

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

/* region, comp_cat, comp will be of total size FTB_eventspace_t */
/* Total size of FTB_client_id_t is 101 bytes*/
typedef struct FTB_client_id {
    FTB_eventspace_t region;
    FTB_eventspace_t comp_cat;
    FTB_eventspace_t comp;
    FTB_client_name_t client_name;
    //uint8_t ext;
    int ext;
}FTB_client_id_t;

struct FTB_client_handle {
    int valid;
    FTB_client_id_t client_id;
};//FTB_client_handle_t;

typedef struct FTB_id {
    FTB_location_id_t location_id;
    FTB_client_id_t client_id;
}FTB_id_t;

typedef struct FTB_event {
    FTB_eventspace_t region;
    FTB_eventspace_t comp_cat;
    FTB_eventspace_t comp;
    FTB_event_name_t event_name;
    FTB_severity_t  severity;
    FTB_client_jobid_t client_jobid;
    FTB_client_name_t client_name;
    FTB_hostname_t hostname;
    //uint8_t ext;;
    int seqnum;
    FTB_tag_len_t len;
    FTB_event_properties_t event_properties;
    char dynamic_data[FTB_MAX_DYNAMIC_DATA_SIZE];
}FTB_event_t;

struct FTB_subscribe_handle {
    FTB_client_handle_t client_handle;
    FTB_event_t subscription_event;
    uint8_t subscription_type;
    uint8_t valid;
};//FTB_subscribe_handle_t;

struct FTB_event_handle {
    FTB_client_id_t client_id;      /* 101 bytes*/
    FTB_location_id_t location_id;  /* 64 for hostname + 4 for pid */
    FTB_event_name_t event_name;    /* 32 for event_name */
    FTB_severity_t  severity;       /* 16 for severity */
    int seqnum;                     /* 4 bytes*/
    /* Do we need the client_jobid?
     * FTB_client_jobid_t client_jobid;
     */
};                                  /* 217 bytes */


#ifdef __cplusplus
} /*extern "C"*/
#endif 

#endif /*FTB_CLIENT_LIB_DEFS_H*/
