#include <stdio.h>
#include <search.h>
#include <string.h>
#include "ftb_def.h"
#include "ftb_event_def.h"
/* Note throw_events.h will be created by the parser (parser.pl) file */
#include "throw_events.h"

int FTBM_create_event_hash() {
   ENTRY event;
   ftb_comp_throw_event_t *info_ptr = throw_events;
   int i=0;
   hcreate(TOTAL_THROW_EVENTS);
   for (i=0; i<TOTAL_THROW_EVENTS; i++) {
        //printf("Adding event %s\n",  throw_events[i].event_name_char);
        event.key = throw_events[i].event_name_char;
        //printf("Severity is %d\n", throw_events[i].severity);
        event.data = info_ptr;
        info_ptr++;
        hsearch(event, ENTER);
   }
   return 0;
}

ENTRY* FTBM_search_event_hash(char *name) {
    ENTRY event;
    event.key= name;
    return (hsearch(event, FIND));
}

int FTBM_get_event_by_name(char *name, FTB_event_t *e)
{
    int ret=0;
    ENTRY *found_event;
    if ((found_event = FTBM_search_event_hash(name)) != NULL) {
            e->event_name = ((ftb_comp_throw_event_t *)found_event->data)->event_name;
            e->severity =   ((ftb_comp_throw_event_t *)found_event->data)->severity;
            e->event_ctgy = ((ftb_comp_throw_event_t *)found_event->data)->event_ctgy;
            e->comp_ctgy =  ((ftb_comp_throw_event_t *)found_event->data)->comp_ctgy;
            e->comp =       ((ftb_comp_throw_event_t *)found_event->data)->comp;
    } 
    else {
            printf("No such event %s\n", name);
    }
    return 0;
}

/*
  int main(int argc, char *argv[])
{ 
    FTB_event_t *e = (void *)malloc (sizeof(FTB_event_t));
    FTBM_create_event_hash();
    if (FTBM_get_event_by_name(argv[1], e) != 0) {
        printf("%s", "FTBM_get_event_by_name didnt return success\n");
    }
    return 0;
}
*/
