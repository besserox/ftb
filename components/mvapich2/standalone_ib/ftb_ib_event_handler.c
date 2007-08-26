#include "libftb.h"
#include <infiniband/verbs.h>
#include <pthread.h>

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

void async_thread(void *ctx);


static void open_hca(void)
{
    struct ibv_device **dev_list;
    int i = 0, num_adapters;

    /* Get the list of the adapters */
    dev_list = ibv_get_device_list(&num_adapters);

    /* Select the first Adapter for now, more changes can be
     * added later */
    ibv_dev.nic = dev_list[0];

    /* Get the context from the selected Adapter */
    ibv_dev.context = ibv_open_device(dev_list[0]);

    /* If the context is not found, report an error and exit */
    if (!ibv_dev.context) {
        fprintf(stderr, "Error Getting HCA Context, Aborting ..\n");
        exit(1);
    }

    /* Allocate the protection domain */
    ibv_dev.ptag = ibv_alloc_pd(ibv_dev.context);

    if (!ibv_dev.ptag) {
        fprintf(stderr, "Error Getting Protection Domain for HCA, Aborting ..\n");
    }

    pthread_create(&ibv_dev.async_thread, NULL,
            (void *) async_thread, (void *) ibv_dev.context);

}


int main (int argc, char *argv[])
{
    FTB_component_properties_t properties;
    uint32_t event_id;
    FTB_event_t event;
    struct ibv_port_attr port_attr;
       
 
    properties.com_namespace = FTB_NAMESPACE_BACKPLANE;
    properties.id = FTB_SRC_ID_PREFIX_BACKPLANE_TEST | FTB_PORT_STATUS_THROWER_ID;
    strcpy(properties.name,"PORT_STATUS_THROWER");
    properties.polling_only = 1;
    properties.polling_queue_len = 0;

    /* Initialize the properties of FTB */
    FTB_Init(&properties);

    /* Initialize the data structures for InfiniBand */
    open_hca();

    event_id = atoi(argv[1]);

    if (FTB_get_event_by_id(event_id, &event) == FTB_ERR_EVENT_NOT_FOUND) {
        int len = FTB_MAX_EVENT_NAME;
        if (argc >=4) {
            if (strlen(argv[3])+1 < len)
                len = strlen(argv[3])+1;
            strncpy(event.name,argv[3], len);
        } else {
            strncpy(event.name, 
                    DEFAULT_THROW_EVENT_NAME, strlen(DEFAULT_THROW_EVENT_NAME)+1);
        }
        
        if (argc >= 5) {
            if (strcasecmp(argv[4], "fatal") == 0) {
                event.severity = FTB_EVENT_SEVERITY_FATAL;
            } else if (strcasecmp(argv[4], "error") == 0) {
                event.severity = FTB_EVENT_SEVERITY_ERROR;
            } else if (strcasecmp(argv[4], "warning") == 0) {
                event.severity = FTB_EVENT_SEVERITY_WARNING;
            } else if (strcasecmp(argv[4], "performance") == 0) {
                event.severity = FTB_EVENT_SEVERITY_PERFORMANCE;
            } else {
                event.severity = FTB_EVENT_SEVERITY_INFO;
            }
        } else {
            event.severity = FTB_EVENT_SEVERITY_INFO;
        }
    }

    FTB_Reg_throw(&event);

    while(1) {
    /* The main thread does not do anything, the asynchronous event
     * handler handles the network events and throws an FTB event
     * accordingly */    
    }
    
    FTB_Finalize();
    
    return 0;
}


void async_thread(void *context)
{
    struct ibv_async_event event;
    int ret;

    /* This thread should be in a cancel enabled state */
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

    while (1) {

        do {
            ret = ibv_get_async_event((struct ibv_context *) context, &event);

            if (ret && errno != EINTR) {
                fprintf(stderr, "Error getting event!\n");
            }
        } while(ret && errno == EINTR);

        switch (event.event_type) {
            /* Fatal */
            case IBV_EVENT_CQ_ERR:
            case IBV_EVENT_QP_FATAL:
            case IBV_EVENT_QP_REQ_ERR:
            case IBV_EVENT_QP_ACCESS_ERR:

            case IBV_EVENT_PATH_MIG:
            case IBV_EVENT_PATH_MIG_ERR:
            case IBV_EVENT_DEVICE_FATAL:
            case IBV_EVENT_SRQ_ERR:
            case IBV_EVENT_QP_LAST_WQE_REACHED:
            case IBV_EVENT_PORT_ERR:
                break;

            case IBV_EVENT_COMM_EST:
            case IBV_EVENT_PORT_ACTIVE:
            case IBV_EVENT_SQ_DRAINED:
            case IBV_EVENT_LID_CHANGE:
            case IBV_EVENT_PKEY_CHANGE:
            case IBV_EVENT_SM_CHANGE:
            case IBV_EVENT_CLIENT_REREGISTER:
                break;

            case IBV_EVENT_SRQ_LIMIT_REACHED:


                break;
            default:
                break;
        }

        ibv_ack_async_event(&event);
    }
}

