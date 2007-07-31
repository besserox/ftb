#include "libftb.h"
#include "ftb_com1_event_list.h"

#define COM1_ID      0x101

int event_handler(FTB_event_inst_t *event, void *arg)
{
    assert(event->event_id==COM1_INFORMATION);
    printf("Received Event: id: %d, name: %s, severity %d, src namespace %d, src id %lu, data: %s\n",
        event->event_id, event->name, event->severity, event->src_namespace, event->src_id, event->immediate_data);
    return 0;
}

int main (int argc, char *argv[])
{
    FTB_component_properties_t properties;
    uint32_t event_id;
    
    properties.com_namespace = FTB_NAMESPACE_GENERAL;
    properties.id = COM1_ID;
    strcpy(properties.name,"COM1_CATCHER");
    properties.polling_only = 0;
    properties.polling_queue_len = 0;

    event_id = COM1_INFORMATION;

    FTB_Init(&properties);
    FTB_Reg_catch_notify_single(event_id, event_handler, NULL);

    while(1) {
        sleep(30);
    }
    FTB_Finalize();
    
    return 0;
}

