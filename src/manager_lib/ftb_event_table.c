#include <stdio.h>
#include <search.h>
#include <string.h>
#include "ftb_def.h"
#include "ftb_event_def.h"
#include "ftb_throw_events.h"

#define FTB_TOTAL_HASH FTB_TOTAL_COMP_CAT+FTB_TOTAL_COMP+FTB_EVENT_DEF_TOTAL_THROW_EVENTS
int FTBM_hash_init() 
{
     hcreate(FTB_TOTAL_HASH);
     return 0;
         
}

int FTBM_compcat_table_init() 
{
   ENTRY comp_cat;
   FTBM_comp_cat_entry_t *info_ptr = FTB_comp_cat_table;
   int i=0;
   //hcreate(FTB_TOTAL_COMP_CAT);
   for (i=0; i<FTB_TOTAL_COMP_CAT; i++) {
        comp_cat.key = FTB_comp_cat_table[i].comp_cat_char;
        comp_cat.data = info_ptr;
        info_ptr++;
        hsearch(comp_cat, ENTER);
   }
   return 0;
}

int FTBM_comp_table_init() 
{
   ENTRY comp;
   FTBM_comp_entry_t *info_ptr = FTB_comp_table;
   int i=0;
  // hcreate(FTB_TOTAL_COMP);
   for (i=0; i<FTB_TOTAL_COMP; i++) {
        comp.key = FTB_comp_table[i].comp_char;
        comp.data = info_ptr;
        info_ptr++;
        hsearch(comp, ENTER);
   }
   return 0;
}

int FTBM_event_table_init() 
{
   ENTRY event;
   FTBM_throw_event_entry_t *info_ptr = FTB_event_table;
   int i=0;
   //hcreate(FTB_EVENT_DEF_TOTAL_THROW_EVENTS);
   for (i=0; i<FTB_EVENT_DEF_TOTAL_THROW_EVENTS; i++) {
        event.key = FTB_event_table[i].event_name_char;
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
            e->event_name = ((FTBM_throw_event_entry_t *)found_event->data)->event_name;
            e->severity   = ((FTBM_throw_event_entry_t *)found_event->data)->severity;
            e->event_cat = ((FTBM_throw_event_entry_t *)found_event->data)->event_cat;
            e->comp_cat  = ((FTBM_throw_event_entry_t *)found_event->data)->comp_cat;
            e->comp       = ((FTBM_throw_event_entry_t *)found_event->data)->comp;
    } 
    else {
            //return FTB_ERR_EVENT_NOT_FOUND;  
            return FTB_ERR_HASHKEY_NOT_FOUND;
    }
    return FTB_SUCCESS;
}


int FTBM_get_compcat_by_name(const char *name, FTB_comp_cat_t *val)
{
    ENTRY *found;
    if ((found = util_search_hash(name)) != NULL) {
            *val = ((FTBM_comp_cat_entry_t *)found->data)->comp_cat;
    }
    else {
        return FTB_ERR_HASHKEY_NOT_FOUND;
    }
    return FTB_SUCCESS;
}

int FTBM_get_comp_by_name(const char *name, FTB_comp_t *val)
{
    ENTRY *found;
    if ((found = util_search_hash(name)) != NULL) {
            *val = ((FTBM_comp_entry_t *)found->data)->comp;
    }
    else {
        return FTB_ERR_HASHKEY_NOT_FOUND;
    }
    return FTB_SUCCESS;
}


int FTBM_get_comp_catch_count(FTB_comp_cat_t comp_cat, FTB_comp_t comp, int *count)
{
    *count = 0;
    return FTB_SUCCESS;
}

int FTBM_get_comp_catch_masks(FTB_comp_cat_t comp_cat, FTB_comp_t comp, FTB_event_t *events)
{
    return FTB_SUCCESS;
}
