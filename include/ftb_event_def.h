#ifndef FTB_EVENT_DEF_H
#define FTB_EVENT_DEF_H

#include "ftb_def.h"


typedef struct FTBM_comp_cat_entry{
    char comp_cat_char[FTB_MAX_COMP_CAT];
    FTB_comp_cat_code_t comp_cat;
}FTBM_comp_cat_entry_t;

typedef struct FTBM_comp_entry{
    char comp_char[FTB_MAX_COMP];
    FTB_comp_code_t comp;
}FTBM_comp_entry_t;

typedef struct FTBM_severity_entry{
    char severity_char[FTB_MAX_SEVERITY];
    FTB_severity_code_t severity;
}FTBM_severity_entry_t;

typedef struct FTBM_throw_event_entry{
    char event_name_char[FTB_MAX_EVENT_NAME];
    FTB_event_name_code_t event_name;
    FTB_severity_code_t  severity;
    FTB_event_cat_code_t event_cat;
    FTB_comp_cat_code_t comp_cat;
    FTB_comp_code_t comp;
}FTBM_throw_event_entry_t;


int FTBM_hash_init(void);

int FTBM_compcat_table_init(void);

int FTBM_comp_table_init(void);

int FTBM_event_table_init(void);

int FTBM_severity_table_init(void);

int FTBM_get_string_by_id(int id, char *name, char *type);

int FTBM_get_compcat_by_name(const char *name, FTB_comp_cat_code_t *val);

int FTBM_get_comp_by_name(const char *name, FTB_comp_code_t *val);

int FTBM_get_event_by_name(const char *name, FTB_event_t *e);

int FTBM_get_severity_by_name(const char *name, FTB_severity_code_t *val);


/*
 FTBM_get_comp_catch_count
 Get the total number of the event masks a specific component wants to catch.
 The number will not change in runtime and will be used to allocate space for next interface
 */
int FTBM_get_comp_catch_count(FTB_comp_cat_code_t comp_cat, FTB_comp_code_t comp, int *count);

/*
 FTBM_get_comp_catch_masks
 Get an array of the masks a component wants to catch. 
 */
int FTBM_get_comp_catch_masks(FTB_comp_cat_code_t comp_cat, FTB_comp_code_t comp, FTB_event_t *events);


/*
static inline int FTBM_get_event_by_name (FTB_event_name_code_t name, FTB_event_t *e)
{
    int ret=FTB_SUCCESS;
    switch (name) {
        case FTB_PINGPONG_EVENT_CLI:
            e->severity = FTB_EVENT_DEF_SEVERITY_INFO;
            e->comp_cat = FTB_COMP_CAT_BACKPLANE;
            e->comp = FTB_COMP_SIMPLE;
            e->event_cat = FTB_EVENT_DEF_EVENT_CAT_GENERAL;
            e->event_name = name;
            break;
        case FTB_PINGPONG_EVENT_SRV:
            e->severity = FTB_EVENT_DEF_SEVERITY_INFO;
            e->comp_cat = FTB_COMP_CAT_BACKPLANE;
            e->comp = FTB_COMP_SIMPLE;
            e->event_cat = FTB_EVENT_DEF_EVENT_CAT_GENERAL;
            e->event_name = name;
            break;
        case FTB_TEST_EVENT_1:
            e->severity = FTB_EVENT_DEF_SEVERITY_INFO;
            e->comp_cat = FTB_COMP_CAT_BACKPLANE;
            e->comp = FTB_TEST_COMP_1;
            e->event_cat = FTB_EVENT_DEF_EVENT_CAT_GENERAL;
            e->event_name = name;
            break;
        case FTB_TEST_EVENT_2:
            e->severity = FTB_EVENT_DEF_SEVERITY_FATAL;
            e->comp_cat = FTB_COMP_CAT_BACKPLANE;
            e->comp = FTB_TEST_COMP_2;
            e->event_cat = FTB_EVENT_DEF_EVENT_CAT_GENERAL;
            e->event_name = name;
            break;
        case SIMPLE_EVENT:
            e->severity = FTB_EVENT_DEF_SEVERITY_INFO;
            e->comp_cat = FTB_COMP_CAT_BACKPLANE;
            e->comp = FTB_COMP_SIMPLE;
            e->event_cat = FTB_EVENT_DEF_EVENT_CAT_GENERAL;
            e->event_name = name;
            break;
        case WATCH_DOG_EVENT:
            e->severity = FTB_EVENT_DEF_SEVERITY_INFO;
            e->comp_cat = FTB_COMP_CAT_BACKPLANE;
            e->comp = FTB_COMP_WATCHDOG;
            e->event_cat = FTB_EVENT_DEF_EVENT_CAT_GENERAL;
            e->event_name = name;
            break;
        case MPICH2_ERROR_ABORT:
            e->severity = FTB_EVENT_DEF_SEVERITY_FATAL;
            e->comp_cat = FTB_EVENT_DEF_COMP_CAT_MPI;
            e->comp = FTB_EVENT_DEF_COMP_MPICH2;
            e->event_cat = FTB_EVENT_DEF_EVENT_CAT_GENERAL;
            e->event_name = name;
            break;
        default:
            ret = FTB_ERR_EVENT_NOT_FOUND;
    }
    return ret;
}

*/

#endif /*FTB_EVENT_DEF_H*/
