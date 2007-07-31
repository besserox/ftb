#include "libftb.h"

#define FTB_EVENT_THROWER_ID      0x00000010

#define DEFAULT_THROW_EVENT_NAME  "EVENT_THROW_TEST"

int main (int argc, char *argv[])
{
    FTB_component_properties_t properties;
    uint32_t event_id;
    FTB_event_t event;
        
    if (argc < 3) {
    	fprintf(stderr,"Usage: %s <event_id> <data> [event_name=<default name>] [event_severity=INFO] \n",argv[0]);
    	fprintf(stderr,"    If event_id is a built-in event, the event_name and severity will be automatically filled\n");
    	fprintf(stderr,"    If event_id is not a built-in event, the event_name and severity will be taken from command line\n");
    	exit(1);
    }

    properties.com_namespace = FTB_NAMESPACE_BACKPLANE;
    properties.id = FTB_SRC_ID_PREFIX_BACKPLANE_TEST | FTB_EVENT_THROWER_ID;
    strcpy(properties.name,"EVENT_THROWER");
    properties.polling_only = 1;
    properties.polling_queue_len = 0;

    FTB_Init(&properties);

    event_id = atoi(argv[1]);
    if (FTB_get_event_by_id(event_id, &event)==FTB_ERR_EVENT_NOT_FOUND)
    {
        int len = FTB_MAX_EVENT_NAME;
        if (argc >=4) {
            if (strlen(argv[3])+1 < len)
                len = strlen(argv[3])+1;
            strncpy(event.name,argv[3], len);
        }
        else {
            strncpy(event.name, DEFAULT_THROW_EVENT_NAME, strlen(DEFAULT_THROW_EVENT_NAME)+1);
        }
        
        if (argc >=5) {
            if (strcasecmp(argv[4], "fatal") == 0) {
                event.severity = FTB_EVENT_SEVERITY_FATAL;
            }
            else if (strcasecmp(argv[4], "error") == 0) {
                event.severity = FTB_EVENT_SEVERITY_ERROR;
            }
            else if (strcasecmp(argv[4], "warning") == 0) {
                event.severity = FTB_EVENT_SEVERITY_WARNING;
            }
            else if (strcasecmp(argv[4], "performance") == 0) {
                event.severity = FTB_EVENT_SEVERITY_PERFORMANCE;
            }
            else {
                event.severity = FTB_EVENT_SEVERITY_INFO;
            }
        }
        else {
            event.severity = FTB_EVENT_SEVERITY_INFO;
        }
    }

    FTB_Reg_throw(&event);
    FTB_Throw(&event, argv[2], strlen(argv[2])+1);
    FTB_Finalize();
    
    return 0;
}
