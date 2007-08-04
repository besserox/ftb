#ifndef FTB_H
#define FTB_H

#include <stdio.h>
#include <errno.h>
#include <error.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <assert.h>
#include <pthread.h>
#include <string.h>
#include <netdb.h>
#include <netinet/tcp.h>
#include <ctype.h>

#define FTB_MAX_EVENT_NAME             128
#define FTB_MAX_HOST_NAME              128
#define FTB_MAX_COM_NAME               128

#define FTB_MAX_EVENT_VERSION_NAME           16

#define FTB_MAX_EVENT_IMMEDIATE_DATA   512

#define FTB_ERR_ABORT(args...)  do {\
    fprintf(stderr, "[FTB_ERROR][%s: line %d]", __FILE__, __LINE__); \
    fprintf(stderr, args); \
    fprintf(stderr, "\n"); \
    exit(-1);\
}while(0)

#define FTB_WARNING(args...)  do {\
    fprintf(stderr, "[FTB_WARNING][%s: line %d]", __FILE__, __LINE__); \
    fprintf(stderr, args); \
    fprintf(stderr, "\n"); \
}while(0)

#define FTB_DEBUG
#ifdef FTB_DEBUG
#define FTB_INFO(args...)  do {\
    fprintf(stderr, "[FTB_INFO][%s: line %d]", __FILE__, __LINE__); \
    fprintf(stderr, args); \
    fprintf(stderr, "\n"); \
}while(0)
#else
#define FTB_INFO(...)
#endif

typedef struct FTB_config{
    uint32_t FTB_id;
    char host_name[FTB_MAX_HOST_NAME];
    uint32_t port;
}FTB_config_t;

typedef struct FTB_component_properties {
    uint32_t com_namespace;
    uint32_t id;
    char name[FTB_MAX_COM_NAME];
    int polling_only;
    int polling_queue_len;
}FTB_component_properties_t;

#define FTB_MSG_TYPE_REG_THROW                       0x00
#define FTB_MSG_TYPE_REG_CATCH_NOTIFY          0x01
#define FTB_MSG_TYPE_REG_CATCH_POLLING        0x02
#define FTB_MSG_TYPE_THROW                               0x10
#define FTB_MSG_TYPE_CATCH                                0x11
#define FTB_MSG_TYPE_NOTIFY                               0x20
#define FTB_MSG_TYPE_LIST_EVENTS                     0x30
#define FTB_MSG_TYPE_FIN                                     0xFF

#define FTB_NAMESPACE_GENERAL          0x01
#define FTB_NAMESPACE_BACKPLANE      0x02
#define FTB_NAMESPACE_SYSTEM            0x04
#define FTB_NAMESPACE_JOB                   0x08

#define FTB_SRC_ID_PREFIX_BACKPLANE_TEST                  0x01000000
#define FTB_SRC_ID_PREFIX_BACKPLANE_LOGGER                0x02000000
#define FTB_SRC_ID_PREFIX_BACKPLANE_WATCHDOG              0x04000000

#define FTB_EVENT_SEVERITY_INFO              0x01
#define FTB_EVENT_SEVERITY_PERFORMANCE       0x02
#define FTB_EVENT_SEVERITY_WARNING           0x04
#define FTB_EVENT_SEVERITY_ERROR             0x08
#define FTB_EVENT_SEVERITY_FATAL             0x10

#define FTB_ERR_EVENT_NOT_FOUND             0x10

#define FTB_MAX_BUILD_IN_EVENT_ID           0xffff

/*event and event instance*/
typedef struct FTB_event{
    uint32_t event_id;
    char name[FTB_MAX_EVENT_NAME];
    uint32_t severity;
}FTB_event_t;

int FTB_get_event_by_id(uint32_t event_id, FTB_event_t *event);

typedef struct FTB_event_inst {
    uint32_t event_id;
    char name[FTB_MAX_EVENT_NAME];
    uint32_t severity;
    uint32_t src_namespace;
    uint32_t src_id;
    char immediate_data[FTB_MAX_EVENT_IMMEDIATE_DATA];
}FTB_event_inst_t;

/*event mask*/

typedef struct FTB_event_mask {
    uint32_t event_id;
    uint32_t severity;    
    uint32_t src_namespace;
    uint32_t src_id;
}FTB_event_mask_t;

#define FTB_EVENT_CLR_ALL(evt_mask) {\
    evt_mask.event_id = 0x0; \
    evt_mask.src_namespace = 0x0; \
    evt_mask.src_id = 0x0; \
    evt_mask.severity = 0x0; \
}
#define FTB_EVENT_CLR_EVENT_ID(evt_mask) {\
    evt_mask.event_id = 0x0; \
}
#define FTB_EVENT_CLR_SEVERITY(evt_mask) {\
    evt_mask.severity = 0x0; \
}
#define FTB_EVENT_CLR_SRC_NAMESPACE(evt_mask) {\
    evt_mask.src_namespace = 0x0; \
}
#define FTB_EVENT_CLR_SRC_ID(evt_mask) {\
    evt_mask.src_id = 0x0; \
}

#define FTB_EVENT_MARK_ALL_EVENT_ID(evt_mask) {\
    evt_mask.event_id = 0xFFFFFFFF; \
}
#define FTB_EVENT_MARK_ALL_SEVERITY(evt_mask) {\
    evt_mask.severity = 0xFFFFFFFF; \
}
#define FTB_EVENT_MARK_ALL_SRC_NAMESPACE(evt_mask) {\
    evt_mask.src_namespace = 0xFFFFFFFF; \
}
#define FTB_EVENT_MARK_ALL_SRC_ID(evt_mask) {\
    evt_mask.src_id = 0xFFFFFFFF; \
}

#define FTB_EVENT_SET_EVENT_ID(evt_mask, val) {\
    evt_mask.event_id = evt_mask.event_id | val; \
}
#define FTB_EVENT_SET_SEVERITY(evt_mask, val) {\
    evt_mask.severity = evt_mask.severity | val; \
}
#define FTB_EVENT_SET_SRC_NAMESPACE(evt_mask, val) {\
    evt_mask.src_namespace = evt_mask.src_namespace | val; \
}
#define FTB_EVENT_SET_SRC_ID(evt_mask, val) {\
    evt_mask.src_id = evt_mask.src_id | val; \
}

#endif
