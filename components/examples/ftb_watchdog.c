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
    FTB_subscribe_handle_t shandle;
    FTB_event_handle_t ehandle;
    int ret = 0;
    FTB_event_info_t event_info[1] = {{"WATCH_DOG_EVENT", "INFO"}};

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

    ret = FTB_Subscribe(&shandle, handle, "", NULL, NULL);
    if (ret != FTB_SUCCESS) {
        printf("FTB_Subscribe failed ret=%d!\n", ret); exit(-1);
    }
   
    ret = FTB_Declare_publishable_events(handle, 0, event_info, 1);
    if (ret != FTB_SUCCESS) {
        printf("FTB_Declare_publishable_events failed ret=%d!\n", ret); exit(-1);
    }

    signal(SIGINT, Int_handler);
    int i =0;
    while(1) {
        i++;
        ret = 0;
        FTB_receive_event_t caught_event;

        /* Publish event with specified event name */
        ret = FTB_Publish(handle, "WATCH_DOG_EVENT", NULL, &ehandle);
        if (ret != FTB_SUCCESS) {
            printf("FTB_Publish failed\n");
            exit(-1);
        }
        sleep(1);
        
        ret = FTB_Poll_event(shandle, &caught_event);
        if (ret != FTB_SUCCESS) {
            fprintf(stderr,"Watchdog: No event caught Error code is %d!\n", ret);
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
        if (i == 100) {
            ret = FTB_Unsubscribe(&shandle);
            if (ret != FTB_SUCCESS) {
                printf("Didnt get a success\n");
            }
        }
        if (done)
            break;
    }
    FTB_Disconnect(handle);
    return 0;
}

