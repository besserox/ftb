#ifndef FTB_EVENT_DEF_H
#define FTB_EVENT_DEF_H

#include "ftb_def.h"

typedef struct FTBM_throw_event_entry{
    FTB_event_name_t event_name_key;
    FTB_event_name_t event_name;
    FTB_severity_t  severity;
    //FTB_event_cat_t event_cat;
    FTB_comp_cat_t comp_cat;
    FTB_comp_t comp;
}FTBM_throw_event_entry_t;


int FTBM_hash_init(void);

int FTBM_event_table_init(void);

int FTBM_get_event_by_name(const char *name, FTB_event_t *e);

#endif /*FTB_EVENT_DEF_H*/

