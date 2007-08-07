#include "libftb.h"

#define FTB_EVENT_LISTER_ID      0x00000100

#define MAX_EVENTS          30

int main (int argc, char *argv[])
{
    FTB_component_properties_t properties;
    FTB_event_t events[MAX_EVENTS];
    int num_events = 30;
    int i;

    properties.com_namespace = FTB_NAMESPACE_BACKPLANE;
    properties.id = FTB_SRC_ID_PREFIX_BACKPLANE_TEST | FTB_EVENT_LISTER_ID;
    strcpy(properties.name,"EVENT_LISTER");
    properties.polling_only = 1;
    properties.polling_queue_len = 0;

    FTB_Init(&properties);
    FTB_List_events(events, &num_events);
    for (i=0;i<num_events;i++) {
        printf("Event: id: %d, name: %s, severity %d\n", 
        events[i].event_id, events[i].name, events[i].severity);
    }
    FTB_Finalize();
    
    return 0;
}
