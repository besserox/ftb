#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include "libftb.h"

static volatile int done = 0;

void Int_handler(int sig){
    if (sig == SIGINT)
        done = 1;
}

int main (int argc, char *argv[])
{
    FTB_client_t cinfo;
    FTB_client_handle_t handle;
    FTB_event_handle_t ehandle;
    FTB_event_mask_t mask;
    FTB_subscribe_handle_t shandle;

    int ret = 0;


    /* Create namespace and other attributes before calling FTB_Connect */
    strcpy(cinfo.event_space, "FTB.FTB_EXAMPLES.Watchdog");
    strcpy(cinfo.client_schema_ver, "0.5");
    strcpy(cinfo.client_name, "watchdog");
    strcpy(cinfo.client_jobid,"watchdog-111");
    strcpy(cinfo.client_subscription_style,"FTB_SUBSCRIPTION_POLLING");
    
    ret = FTB_Connect(&cinfo, &handle);
    if (ret != FTB_SUCCESS) {
        printf("FTB_Connect is not successful ret=%d\n", ret);
        exit(-1);
    }

     /* Create a subscription mask */
    ret = FTB_Create_mask(&mask, "all", "init");
    if (ret != FTB_SUCCESS) { 
        printf("Mask creation failed for field_name=all and field value=init \n"); exit(-1);
    }
    ret = FTB_Create_mask(&mask, "event_name", "WATCH_DOG_EVENT");
    if (ret != FTB_SUCCESS) { 
        printf("Setting Event name in mask failed\n"); exit(-1);
    }
    
    /* Subscribe the created mask using the polling mechanism*/
    ret = FTB_Subscribe(handle, &mask, &shandle, NULL, NULL);
    if (ret != FTB_SUCCESS) {
        printf("FTB_Subscribe failed ret=%d!\n", ret); exit(-1);
    }
   
    signal(SIGINT, Int_handler);

    
    while(1) {
        ret = 0;
        FTB_catch_event_info_t caught_event;

        /* Publish event with specified event name */
        ret = FTB_Publish(handle, "WATCH_DOG_EVENT", NULL, &ehandle);
        if (ret != FTB_SUCCESS) {
            printf("FTB_Publish_event failed\n");
            exit(-1);
        }
        sleep(1);
        
        /* Poll for a event which matches mask registered via shandle */
        ret = FTB_Poll_event(shandle, &caught_event);
        if (ret == FTB_CAUGHT_NO_EVENT) {
            fprintf(stderr,"Watchdog: No event caught!\n");
            break;
        }
        printf("Watchdog: Component name=%s Comp category=%s Severity=%s ",
                caught_event.comp, caught_event.comp_cat, caught_event.severity);
        printf("Event name=%s Region=%s Instance name=%s Hostname=%s, Jobid=%s, Seqnum=%d ",
                caught_event.event_name, caught_event.region, caught_event.client_name, 
                caught_event.hostname, caught_event.client_jobid, caught_event.seqnum);
        /*printf("User specified received data has watchdogid=%d and watchdog name=%s. Watchdog Tag is %s\n", 
                recv_info.watchdog_id, recv_info.watchdog_name, tag_data_recv);*/
        printf("Watchdog: Location is hostname=%s pid=%d\n", caught_event.incoming_src.hostname, 
                caught_event.incoming_src.pid);
        sleep(10);
        if (done)
            break;
    }
    FTB_Disconnect(handle);
    return 0;
}

