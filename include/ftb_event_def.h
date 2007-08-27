#ifndef FTB_EVENT_DEF_H
#define FTB_EVENT_DEF_H

#include "ftb_def.h"

/*Definition of severity*/
#define FTB_EVENT_DEF_SEVERITY_INFO          (1<<1)
#define FTB_EVENT_DEF_SEVERITY_PERFORMANCE   (1<<2)
#define FTB_EVENT_DEF_SEVERITY_WARNING       (1<<3)
#define FTB_EVENT_DEF_SEVERITY_ERROR         (1<<4)
#define FTB_EVENT_DEF_SEVERITY_FATAL         (1<<5)

/*Definition of component category*/
#define FTB_EVENT_DEF_COMP_CTGY_MPI          (1<<1)

/*Definition of components*/
#define FTB_EVENT_DEF_COMP_MPICH2            (1<<1)

/*Definition of event category*/
#define FTB_EVENT_DEF_EVENT_CTGY_GENERAL     (1<<0)

/*Definition of event name identifier*/
#define WATCH_DOG_EVENT                      0x1
#define MPICH2_ERROR_ABORT                   0x2
#define SIMPLE_EVENT                         0x3
#define FTB_TEST_EVENT_1                     0x4
#define FTB_TEST_EVENT_2                     0x5 
#define FTB_PINGPONG_EVENT_CLI               0x6
#define FTB_PINGPONG_EVENT_SRV               0x7

/*Definition of event name string*/
#define MPICH2_ERROR_ABORT_STR               "MPICH2_ERROR_ABORT"

static inline int FTBM_get_event_by_name (FTB_event_name_t name, FTB_event_t *e)
{
    int ret=FTB_SUCCESS;
    switch (name) {
        case FTB_PINGPONG_EVENT_CLI:
            e->severity = FTB_EVENT_DEF_SEVERITY_INFO;
            e->comp_ctgy = FTB_COMP_CTGY_BACKPLANE;
            e->comp = FTB_COMP_SIMPLE;
            e->event_ctgy = FTB_EVENT_DEF_EVENT_CTGY_GENERAL;
            e->event_name = name;
            break;
        case FTB_PINGPONG_EVENT_SRV:
            e->severity = FTB_EVENT_DEF_SEVERITY_INFO;
            e->comp_ctgy = FTB_COMP_CTGY_BACKPLANE;
            e->comp = FTB_COMP_SIMPLE;
            e->event_ctgy = FTB_EVENT_DEF_EVENT_CTGY_GENERAL;
            e->event_name = name;
            break;
        case FTB_TEST_EVENT_1:
            e->severity = FTB_EVENT_DEF_SEVERITY_INFO;
            e->comp_ctgy = FTB_COMP_CTGY_BACKPLANE;
            e->comp = FTB_TEST_COMP_1;
            e->event_ctgy = FTB_EVENT_DEF_EVENT_CTGY_GENERAL;
            e->event_name = name;
            break;
        case FTB_TEST_EVENT_2:
            e->severity = FTB_EVENT_DEF_SEVERITY_FATAL;
            e->comp_ctgy = FTB_COMP_CTGY_BACKPLANE;
            e->comp = FTB_TEST_COMP_2;
            e->event_ctgy = FTB_EVENT_DEF_EVENT_CTGY_GENERAL;
            e->event_name = name;
            break;
        case SIMPLE_EVENT:
            e->severity = FTB_EVENT_DEF_SEVERITY_INFO;
            e->comp_ctgy = FTB_COMP_CTGY_BACKPLANE;
            e->comp = FTB_COMP_SIMPLE;
            e->event_ctgy = FTB_EVENT_DEF_EVENT_CTGY_GENERAL;
            e->event_name = name;
            break;
        case WATCH_DOG_EVENT:
            e->severity = FTB_EVENT_DEF_SEVERITY_INFO;
            e->comp_ctgy = FTB_COMP_CTGY_BACKPLANE;
            e->comp = FTB_COMP_WATCHDOG;
            e->event_ctgy = FTB_EVENT_DEF_EVENT_CTGY_GENERAL;
            e->event_name = name;
            break;
        case MPICH2_ERROR_ABORT:
            e->severity = FTB_EVENT_DEF_SEVERITY_FATAL;
            e->comp_ctgy = FTB_EVENT_DEF_COMP_CTGY_MPI;
            e->comp = FTB_EVENT_DEF_COMP_MPICH2;
            e->event_ctgy = FTB_EVENT_DEF_EVENT_CTGY_GENERAL;
            e->event_name = name;
            break;
        default:
            ret = FTB_ERR_EVENT_NOT_FOUND;
    }
    return ret;
}

#endif /*FTB_EVENT_DEF_H*/
