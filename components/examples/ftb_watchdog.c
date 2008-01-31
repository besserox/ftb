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
    FTB_event_handle_t ehandle1, ehandle2, ehandle3;
    int ret = 0, num =0;
    FTB_event_info_t event_info[1] = {{"WATCH_DOG_EVENT", "INFO"}};

    printf("Sizeof FTB_event_handle_t=%d and FTB_event_t=%d\n", sizeof(FTB_event_handle_t), sizeof(FTB_event_t));
    /* Create eventspace and other attributes before calling FTB_Connect */
    strcpy(cinfo.event_space, "FTB.FTB_EXAMPLES.watchdog");
    strcpy(cinfo.client_schema_ver, "0.5");
    strcpy(cinfo.client_name, "watchdog");
    strcpy(cinfo.client_jobid,"watchdog-111");
    strcpy(cinfo.client_subscription_style,"FTB_SUBSCRIPTION_POLLING");
    
    ret = FTB_Connect(&cinfo, &handle);
    if (ret != FTB_SUCCESS) {
        printf("FTB_Connect is not successful ret=%d\n", ret);
        exit(-1);
    }
    ret = FTB_Declare_publishable_events(handle, "/homes/rgupta/ftb-code/trunk/components/examples/data.txt", event_info, num);
    if (ret != FTB_SUCCESS) {
        printf("FTB_Declare_Publishable_events is not successful ret=%d\n", ret);
        exit(-1);
    }

    /*
    ret = FTB_Declare_publishable_events(handle, 0, event_info, 1);
    if (ret != FTB_SUCCESS) {
        printf("FTB_Declare_publishable_events failed ret=%d!\n", ret); exit(-1);
    }
    */

    ret = FTB_Subscribe(&shandle, handle, "event_space=ftb.all.watchdog", NULL, NULL);
    if (ret != FTB_SUCCESS) {
        printf("FTB_Subscribe failed ret=%d!\n", ret); exit(-1);
    }

    signal(SIGINT, Int_handler);
    ret = FTB_Publish(handle, "WATCH_DOG_EVENT", NULL, &ehandle1);
    if (ret != FTB_SUCCESS) {
        printf("FTB_Publish failed\n");
        exit(-1);
    }

    sleep(2);
    FTB_receive_event_t caught_event;
    ret = FTB_Poll_event(shandle, &caught_event);
    if (ret != FTB_SUCCESS) {
        fprintf(stderr,"Watchdog: No event caught Error code is %d!\n", ret);
        exit(-1);
    }
    FTB_event_properties_t *eprop = (FTB_event_properties_t *)malloc(sizeof(FTB_event_properties_t));
    eprop->event_type = 2;
    ret = FTB_Get_event_handle(caught_event, &ehandle2);
    if (ret != FTB_SUCCESS) {
        fprintf(stderr,"FTB_Get_event_handle failed %d", ret);
        exit(-1);
    }
    printf("44444event_handle2.client_id.region=%s compcat=%s comp=%s client_name=%s extension=%d\n", ehandle2.client_id.region, ehandle2.client_id.comp_cat, ehandle2.client_id.comp, ehandle2.client_id.client_name, ehandle2.client_id.ext);
    memcpy(&eprop->event_payload, (char*)&ehandle2, sizeof(FTB_event_handle_t));

    int i =0;
    while(1) {
        i++;
        ret = 0;
        FTB_receive_event_t caught_event;

        /* Publish event with specified event name */
        ret = FTB_Publish(handle, "WATCH_DOG_EVENT", eprop, &ehandle3);
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

        FTB_event_handle_t ehandle4;
            printf("0000000000 event_type is %d\n", caught_event.event_type);
        if (caught_event.event_type == 2) {
            memcpy(&ehandle4, &caught_event.event_payload, sizeof(FTB_event_handle_t));
        }
        printf("1111111111 event_handle1.client_id.region=%s compcat=%s comp=%s client_name=%s extension=%d hostname=%s pid=%d starttime=%s event_name=%s severity=%s seqnum=%d\n", ehandle1.client_id.region, ehandle1.client_id.comp_cat, ehandle1.client_id.comp, ehandle1.client_id.client_name, ehandle1.client_id.ext, ehandle1.location_id.hostname, ehandle1.location_id.pid, ehandle1.location_id.pid_starttime, ehandle1.event_name, ehandle1.severity, ehandle1.seqnum);

        printf("4444444444 event_handle4.client_id.region=%s compcat=%s comp=%s client_name=%s extension=%d hostname=%s pid=%d starttime=%s event_name=%s severity=%s seqnum=%d\n", ehandle4.client_id.region, ehandle4.client_id.comp_cat, ehandle4.client_id.comp, ehandle4.client_id.client_name, ehandle4.client_id.ext, ehandle4.location_id.hostname, ehandle4.location_id.pid, ehandle4.location_id.pid_starttime, ehandle4.event_name, ehandle4.severity, ehandle4.seqnum);


        ret = FTB_Compare_event_handles(ehandle1, ehandle4);
        if (ret != FTB_SUCCESS) {
            fprintf(stderr,"FTB_Get_event_handle failed %d", ret);
            exit(-1);
        }
        printf("Handle is same as first publish\n");

        printf("Received event details: Event space=%s, Severity=%s, Event name=%s, Client name=%s, Hostname=%s, Jobid=%s, Seqnum=%d\n", caught_event.event_space, caught_event.severity,  caught_event.event_name, caught_event.client_name, caught_event.incoming_src.hostname, caught_event.client_jobid, caught_event.seqnum);
        printf("Received events: pid=%d and pid_starttime=%s\n", caught_event.incoming_src.pid,  caught_event.incoming_src.pid_starttime);
        
        if (done)
            break;
    }
    FTB_Disconnect(handle);
    return 0;
}

