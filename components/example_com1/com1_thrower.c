#include "libftb.h"
#include "ftb_com1_event_list.h"

#define COM1_ID      0x100

int main (int argc, char *argv[])
{
    FTB_component_properties_t properties;
    uint32_t event_id;
    FTB_event_t event;
        
    if (argc < 2) {
    	fprintf(stderr,"Usage: %s <data>\n",argv[0]);
    	exit(1);
    }

    properties.com_namespace = FTB_NAMESPACE_GENERAL;
    properties.id = COM1_ID;
    strcpy(properties.name,"COM1_THROWER");
    properties.polling_only = 1;
    properties.polling_queue_len = 0;

    FTB_Init(&properties);

    event_id = COM1_INFORMATION;
    if (FTB_get_com1_event_by_id(event_id, &event)==FTB_ERR_EVENT_NOT_FOUND)
    {
        fprintf(stderr,"event not found\n");
        return -1;
    }

    FTB_Reg_throw(&event);
    FTB_Throw(&event, argv[1], strlen(argv[1])+1);
    FTB_Finalize();
    
    return 0;
}
