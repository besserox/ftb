#include "libftb.h"

#define FTB_DEFAULT_WATCHDOG_ID         0x00000001

int main (int argc, char *argv[])
{
    FTB_component_properties_t properties;
    uint32_t watchdog_id = FTB_DEFAULT_WATCHDOG_ID;
    char event_data[FTB_MAX_EVENT_IMMEDIATE_DATA];
    int i = 0;

    if (argc >= 2) {
        watchdog_id = atoi(argv[1]);
    }

    properties.com_namespace = FTB_NAMESPACE_BACKPLANE;
    properties.id = FTB_SRC_ID_PREFIX_BACKPLANE_WATCHDOG | watchdog_id;
    strcpy(properties.name,"WATCHDOG");
    properties.polling_only = 1;
    properties.polling_queue_len = 10;

    FTB_Init(&properties);
    FTB_Reg_throw_id(BACKPLANE_WATCHDOG_HEARTBEAT);
    FTB_Reg_catch_polling_single(BACKPLANE_WATCHDOG_HEARTBEAT);

    while(1) {
        int ret = 0;
        FTB_event_inst_t evt;
        i++;
        sprintf(event_data,"%d",i);
        FTB_Throw_id(BACKPLANE_WATCHDOG_HEARTBEAT, event_data, FTB_MAX_EVENT_IMMEDIATE_DATA);
        sleep(1);
        ret = FTB_Catch(&evt);
        if (ret) {
            fprintf(stderr,"Watchdog: event lost!\n");
            break;
        }
        assert(evt.event_id == BACKPLANE_WATCHDOG_HEARTBEAT);
        printf("Watchdog: got watchdog event %s\n", evt.immediate_data);
        sleep(29);
    }
    FTB_Finalize();
    
    return 0;
}

