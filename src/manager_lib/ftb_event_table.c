#include <stdio.h>
#include <search.h>
#include <string.h>
#include <stdlib.h>
#include "ftb_def.h"
#include "ftb_event_def.h"
#include "ftb_throw_events.h"

int FTBM_hash_init() 
{
     hcreate(FTB_TOTAL_PUBLISH_EVENTS);
     return 0;
}

int FTBM_event_table_init() 
{
   ENTRY event;
   FTBM_throw_event_entry_t *info_ptr = FTB_event_table;
   int i=0;
   for (i=0; i<FTB_TOTAL_PUBLISH_EVENTS; i++) {
        event.key = FTB_event_table[i].event_name_key;
        event.data = info_ptr;
        info_ptr++;
        hsearch(event, ENTER);
   }
   return 0;
}

static ENTRY* util_search_hash(const char *name) 
{
    ENTRY event;
    event.key= (char *)name;
    return (hsearch(event, FIND));
}

int FTBM_get_event_by_name(const char *name, FTB_event_t *e)
{
    ENTRY *found_event;
    if ((found_event = util_search_hash(name)) != NULL) {
            strcpy(e->event_name, ((FTBM_throw_event_entry_t *)found_event->data)->event_name);
            strcpy(e->severity, ((FTBM_throw_event_entry_t *)found_event->data)->severity);
            //strcpy(e->event_cat,((FTBM_throw_event_entry_t *)found_event->data)->event_cat);
            strcpy(e->comp_cat, ((FTBM_throw_event_entry_t *)found_event->data)->comp_cat);
            strcpy(e->comp, ((FTBM_throw_event_entry_t *)found_event->data)->comp);
    } 
    else {
            return FTB_ERR_FILTER_VALUE;
    }
    return FTB_SUCCESS;
}
