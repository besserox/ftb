#ifndef FTB_H
#define FTB_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif 

#define FTB_MAX_EVENT_VERSION_NAME     16

#ifdef FTB_BGL

/*Not support notification*/
#define FTB_CLIENT_POLLING_ONLY

#endif /*FTB_BGL*/

#define FTB_MAX_EVENT_NAME             64
#define FTB_MAX_HOST_NAME              64
#define FTB_MAX_COM_NAME               64
#define FTB_MAX_EVENT_IMMEDIATE_DATA   128

typedef uint32_t FTB_event_id_t;

typedef uint32_t FTB_component_id_t;
/*0 is reserved for invalid component ID*/

#define FTB_INVALID_COMPONENT_ID         0
#define FTB_INVALID_NAMESPACE               0

typedef struct FTB_config{
    uint32_t FTB_system_id;
}FTB_config_t;

typedef struct FTB_component_properties {
    uint32_t com_namespace;
    FTB_component_id_t id;
    char name[FTB_MAX_COM_NAME];
    int polling_only;
    int polling_queue_len;
}FTB_component_properties_t;

#define FTB_NAMESPACE_GENERAL                   0x01
#define FTB_NAMESPACE_BACKPLANE                 0x02
#define FTB_NAMESPACE_SYSTEM                    0x04
#define FTB_NAMESPACE_JOB                       0x08

#define FTB_SRC_ID_PREFIX_BACKPLANE_TEST        0x01000000
#define FTB_SRC_ID_PREFIX_BACKPLANE_LOGGER      0x02000000
#define FTB_SRC_ID_PREFIX_BACKPLANE_WATCHDOG    0x03000000
#define FTB_SRC_ID_PREFIX_BACKPLANE_NODE        0x04000000

#define FTB_EVENT_SEVERITY_INFO                 0x01
#define FTB_EVENT_SEVERITY_PERFORMANCE          0x02
#define FTB_EVENT_SEVERITY_WARNING              0x04
#define FTB_EVENT_SEVERITY_ERROR                0x08
#define FTB_EVENT_SEVERITY_FATAL                0x10

#define FTB_SUCCESS                             0
#define FTB_ERR_GENERAL                         (-1)
#define FTB_ERR_NULL_POINTER                    (-2 )
#define FTB_ERR_NOT_SUPPORTED                   (-3)
#define FTB_ERR_INVALID_PARAMETER               (-4)
#define FTB_ERR_NETWORK_GENERAL                 (-5)
#define FTB_ERR_NETWORK_NO_ROUTE                (-6)
#define FTB_ERR_EVENT_NOT_FOUND                 (-16)

#define FTB_CAUGHT_NO_EVENT                 0
#define FTB_CAUGHT_EVENT                       1

#define FTB_MAX_BUILD_IN_EVENT_ID               0xffff

/*event and event instance*/
typedef struct FTB_event{
    uint32_t event_id;
    char name[FTB_MAX_EVENT_NAME];
    uint32_t severity;
}FTB_event_t;

int FTB_get_event_by_id(uint32_t event_id, FTB_event_t *event);

typedef struct FTB_event_inst {
    FTB_event_id_t event_id;
    char name[FTB_MAX_EVENT_NAME];
    uint32_t severity;
    uint32_t src_namespace;
    uint32_t src_id;
    char immediate_data[FTB_MAX_EVENT_IMMEDIATE_DATA];
}FTB_event_inst_t;

/*event mask*/

typedef struct FTB_event_mask {
    FTB_event_id_t event_id;
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

#ifdef __cplusplus
} /*extern "C"*/
#endif 

#endif
