#include <stdio.h>
#include <search.h>
#include <string.h>
#include "ftb_def.h"
#include "ftb_event_def.h"
#include "ftb_throw_events.h"

int FTBM_event_table_init() 
{
   ENTRY event;
   FTBM_throw_event_t *info_ptr = FTB_event_table;
   int i=0;
   hcreate(FTB_EVENT_DEF_TOTAL_THROW_EVENTS);
   for (i=0; i<FTB_EVENT_DEF_TOTAL_THROW_EVENTS; i++) {
        event.key = FTB_event_table[i].event_name_char;
        event.data = info_ptr;
        info_ptr++;
        hsearch(event, ENTER);
   }
   return 0;
}

static ENTRY* util_search_event_hash(const char *name) 
{
    ENTRY event;
    event.key= name;
    return (hsearch(event, FIND));
}

int FTBM_get_event_by_name(const char *name, FTB_event_t *e)
{
    ENTRY *found_event;
    if ((found_event = util_search_event_hash(name)) != NULL) {
            e->event_name = ((FTBM_throw_event_t *)found_event->data)->event_name;
            e->severity   = ((FTBM_throw_event_t *)found_event->data)->severity;
            e->event_ctgy = ((FTBM_throw_event_t *)found_event->data)->event_ctgy;
            e->comp_ctgy  = ((FTBM_throw_event_t *)found_event->data)->comp_ctgy;
            e->comp       = ((FTBM_throw_event_t *)found_event->data)->comp;
    } 
    else {
            return FTB_ERR_EVENT_NOT_FOUND;  
    }
    return FTB_SUCCESS;
}

int FTBM_get_comp_catch_count(FTB_comp_ctgy_t comp_ctgy, FTB_comp_t comp, int *count)
{
    *count = 0;
    return FTB_SUCCESS;
}

int FTBM_get_comp_catch_masks(FTB_comp_ctgy_t comp_ctgy, FTB_comp_t comp, FTB_event_t *events)
{
    return FTB_SUCCESS;
}
