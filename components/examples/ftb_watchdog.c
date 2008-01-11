#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include "libftb.h"
#include "ftb_ftb_examples_watchdog_publishevents.h"

static volatile int done = 0;
char err_msg[FTB_MAX_ERRMSG_LEN];

void Int_handler(int sig){
    if (sig == SIGINT)
        done = 1;
}

struct watchdog_info {
    int watchdog_id ;
    char watchdog_name[16];
};

int main (int argc, char *argv[])
{
    FTB_comp_info_t cinfo;
    FTB_client_handle_t handle;
    FTB_event_mask_t mask;
    FTB_subscribe_handle_t shandle;
    int ret = 0;
    FTB_event_data_t *publish_event_data = (FTB_event_data_t *)malloc(sizeof(FTB_event_data_t));
    struct watchdog_info send_info, recv_info;
    char *tag_data1 = "sample_data";
    char *tag_data2 = "sample_data_rinku";
    FTB_tag_len_t data_len = 30;
    char tag_data_recv[256];


    /* Create namespace and other attributes before calling FTB_Connect */
    strcpy(cinfo.comp_namespace, "FTB.FTB_EXAMPLES.Watchdog");
    strcpy(cinfo.schema_ver, "0.5");
    strcpy(cinfo.inst_name, "watchdog");
    strcpy(cinfo.jobid,"watchdog-111");
    strcpy(cinfo.catch_style,"FTB_POLLING_CATCH");
    ret = FTB_Connect(&cinfo, &handle, err_msg);
    if (ret != FTB_SUCCESS) {
        printf("FTB_Connect is not successful ret=%d\n", ret);
        exit(-1);
    }
  
    FTB_Register_publishable_events(handle, ftb_ftb_examples_watchdog_events, FTB_FTB_EXAMPLES_WATCHDOG_TOTAL_EVENTS, err_msg);
    
     /* Create a subscription mask */
    ret = FTB_Create_mask(&mask, "all", "init", err_msg);
    if (ret != FTB_SUCCESS) { 
        printf("Mask creation failed for field_name=all and field value=init \n"); exit(-1);
    }
    ret = FTB_Create_mask(&mask, "event_name", "WATCH_DOG_EVENT", err_msg);
    if (ret != FTB_SUCCESS) { 
        printf("Setting Event name in mask failed\n"); exit(-1);
    }
    
    /* Subscribe the created mask using the polling mechanism*/
    ret = FTB_Subscribe(handle, &mask, &shandle, err_msg, NULL, NULL);
    if (ret != FTB_SUCCESS) {
        printf("FTB_Subscribe failed ret=%d!\n", ret); exit(-1);
    }
   
    /* Create a tag identified by "1" */ 
    ret = FTB_Add_tag(handle, 1, tag_data1, strlen(tag_data1)+1, err_msg);
    if (ret != FTB_SUCCESS) {
        printf("FTB_Add_tag with name 1 failed\n");
        exit (-1);
    }
    FTB_Add_tag(handle, 2, tag_data2, strlen(tag_data2)+1, err_msg);
    if (ret != FTB_SUCCESS) {
        printf("FTB_Add_tag with name 2 failed\n");
        exit (-1);
    }

    /* Copy data to send to with event to be published */
    send_info.watchdog_id = 5;
    strcpy(send_info.watchdog_name, "ftb_watchdog");
    memcpy(publish_event_data->data, &send_info, sizeof(struct watchdog_info));
    signal(SIGINT, Int_handler);

    
    while(1) {
        ret = 0;
        FTB_catch_event_info_t caught_event;

        /* Publish event with specified event name */
        ret = FTB_Publish_event(handle, "WATCH_DOG_EVENT", publish_event_data, err_msg);
        if (ret != FTB_SUCCESS) {
            printf("FTB_Publish_event failed\n");
            exit(-1);
        }
        sleep(1);
        
        /* Poll for a event which matches mask registered via shandle */
        ret = FTB_Poll_event(shandle, &caught_event, err_msg);
        if (ret == FTB_CAUGHT_NO_EVENT) {
            fprintf(stderr,"Watchdog: No event caught!\n");
            break;
        }
        
        memcpy(&recv_info, caught_event.event_data.data, sizeof(struct watchdog_info));
        ret = FTB_Read_tag(&caught_event, 2, tag_data_recv, &data_len, err_msg);
        if (ret != FTB_SUCCESS) {
            printf("FTB_Read_tag failed %d\n", ret);
            exit(-1);
        }
        printf("Watchdog: Component name=%s Comp category=%s Severity=%s ",
                caught_event.comp, caught_event.comp_cat, caught_event.severity);
        printf("Event name=%s Region=%s Instance name=%s Hostname=%s, Jobid=%s, Seqnum=%d ",
                caught_event.event_name, caught_event.region, caught_event.inst_name, 
                caught_event.hostname, caught_event.jobid, caught_event.seqnum);
        printf("User specified received data has watchdogid=%d and watchdog name=%s. Watchdog Tag is %s\n", 
                recv_info.watchdog_id, recv_info.watchdog_name, tag_data_recv);
        printf("Watchdog: Location is hostname=%s pid=%d\n", caught_event.incoming_src.hostname, 
                caught_event.incoming_src.pid);
        sleep(10);
        if (done)
            break;
    }
    FTB_Disconnect(handle);
    return 0;
}

