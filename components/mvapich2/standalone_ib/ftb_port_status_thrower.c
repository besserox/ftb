#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>

#include "libftb.h"
#include "ftb_ftb_examples_simple_publishevents.h"
#include <infiniband/verbs.h>

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
    int num_adapters;
    
    /* Get the list of the adapters */
    dev_list = ibv_get_device_list(&num_adapters);
    
    /* Select the first Adapter for now, more changes can be
     *      * added later */
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
    
    if (!ibv_dev.ptag) {                                                                       fprintf(stderr, "Error Getting Protection Domain for HCA, Aborting ..\n");
    }                                                                                  }


char err_msg[FTB_MAX_ERRMSG_LEN];

int main (int argc, char *argv[])
{
    FTB_comp_info_t cinfo;
    FTB_client_handle_t handle;
    int i, ret = 0;
    struct ibv_port_attr port_attr;
        
    
    printf("Begin\n");
    
    strcpy(cinfo.comp_namespace,"FTB.FTB_EXAMPLES.SIMPLE");
    strcpy(cinfo.schema_ver, "0.5");
    strcpy(cinfo.inst_name,"");
    strcpy(cinfo.jobid,"");
    strcpy(cinfo.catch_style, "FTB_NO_CATCH");

    ret = FTB_Init(&cinfo, &handle, err_msg);
    if (ret != FTB_SUCCESS) {
        printf("FTB_Init did not return a success\n");
        exit(-1);
    }

    ret = FTB_Register_publishable_events(handle, ftb_ftb_examples_simple_events,
                    FTB_FTB_EXAMPLES_SIMPLE_TOTAL_EVENTS, err_msg);

    open_hca();
    
    for (i = 0 ; i < 12 ; i++) {
        if (ibv_query_port(ibv_dev.context, 2,
                    &port_attr)) {
            fprintf(stderr, "Error Querying the Port Status\n");
            exit(1);
        }

        /* If the status of the port is active, throw an FTB event
         * referring to port status as ACTIVE */

        if (IBV_PORT_ACTIVE == port_attr.state) {
            
            printf("FTB_Publish_event\n");
            
            ret = FTB_Publish_event(handle, "SIMPLE_EVENT", NULL, err_msg);
            
            if (ret != FTB_SUCCESS) {
                printf("FTB_Publish_event did not return a success\n");
                exit(-1);
            }
            printf("sleeping for a couple of seconds now ...\n");
            sleep(2);
        }
    }
    printf("FTB_Finalize\n");
    FTB_Finalize(handle);

    printf("End\n");
    return 0;
}
