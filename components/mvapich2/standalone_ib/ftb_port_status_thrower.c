#include "libftb.h"
#include <infiniband/verbs.h>

#define FTB_PORT_STATUS_THROWER_ID      0x0010000

#define DEFAULT_THROW_EVENT_NAME  "FTB_PORT_STATUS_THROWER"

#define FTB_EVENT_IB_PORT_ACTIVE 1
#define FTB_EVENT_IB_PORT_DOWN 2

typedef struct {

    struct ibv_device *nic;

    struct ibv_context *context;

    struct ibv_pd *ptag;

    struct ibv_qp **qp_hndl;

    struct ibv_cq *cq_hndl;

    pthread_t async_thread;

} ibv_info_t;

ibv_info_t ibv_dev;


static void open_hca(void)
{
    struct ibv_device **dev_list;
    int i = 0, num_adapters;

    dev_list = ibv_get_device_list(&num_adapters);

    ibv_dev.nic = dev_list[0];

    ibv_dev.context = ibv_open_device(dev_list[0]);

    if (!ibv_dev.context) {
        printf("Error getting HCA context\n");
    }

    ibv_dev.ptag = ibv_alloc_pd(ibv_dev.context);

    if (!ibv_dev.ptag) {
                printf("Error getting protection domain\n");
    }
}


int main (int argc, char *argv[])
{
    FTB_component_properties_t properties;
    uint32_t event_id;
    FTB_event_t event;
    struct ibv_port_attr port_attr;
       
 
    if (argc < 3) {
    	fprintf(stderr,"Usage: %s <event_id> <data> [event_name=<default name>] [event_severity=INFO] \n",argv[0]);
    	fprintf(stderr,"If event_id is a built-in event, the event_name and severity will be automatically filled\n");
    	fprintf(stderr,"If event_id is not a built-in event, the event_name and severity will be taken from command line\n");
    	exit(1);
    }

    properties.com_namespace = FTB_NAMESPACE_BACKPLANE;
    properties.id = FTB_SRC_ID_PREFIX_BACKPLANE_TEST | FTB_PORT_STATUS_THROWER_ID;
    strcpy(properties.name,"PORT_STATUS_THROWER");
    properties.polling_only = 1;
    properties.polling_queue_len = 0;

    FTB_Init(&properties);

    open_hca();

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
            strncpy(event.name, 
                    DEFAULT_THROW_EVENT_NAME, strlen(DEFAULT_THROW_EVENT_NAME)+1);
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

    while(1) {

        if (ibv_query_port(ibv_dev.context, 2,
                    &port_attr)) {
        }

        if (port_attr.state == IBV_PORT_ACTIVE) {

            event.severity = FTB_EVENT_SEVERITY_PERFORMANCE; 
            FTB_Throw_id(FTB_EVENT_IB_PORT_ACTIVE, argv[2], strlen(argv[2])+1);

            sleep(5);
        } else if (port_attr.state == IBV_PORT_DOWN) {

            event.severity = FTB_EVENT_SEVERITY_FATAL; 
            FTB_Throw(&event, argv[2], strlen(argv[2])+1);
            sleep(30);
        }


    }
    
    FTB_Finalize();
    
    return 0;
}
