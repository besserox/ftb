#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>

#include "libftb.h"

int main (int argc, char *argv[])
{
    FTB_client_handle_t handle;
    int i;
    FTB_client_t cinfo;
    FTB_subscribe_handle_t shandle;
    int ret=0;
    
    printf("Begin\n");
    strcpy(cinfo.event_space,"FTB.FTB_EXAMPLES.SIMPLE");
    strcpy(cinfo.client_schema_ver, "0.5");
    strcpy(cinfo.client_name,"");
    strcpy(cinfo.client_jobid,"");
    strcpy(cinfo.client_subscription_style,"FTB_SUBSCRIPTION_POLLING");
    
    ret = FTB_Connect(&cinfo, &handle);
    if (ret != FTB_SUCCESS) {
        printf("FTB_Connect was not successful\n");
        exit(-1);
    }
  
    
    ret = FTB_Subscribe(&shandle, handle, "event_name=SIMPLE_EVENT", NULL, NULL);
    if (ret != FTB_SUCCESS) {
        printf("FTB_Subscribe failed!\n"); 
        exit(-1);
    }
        
    for(i=0;i<12;i++) {
        int ret;
        FTB_receive_event_t event;
        printf("sleep(10)\n");
        sleep(10);
        while(1) {
            printf("FTB_Poll_event\n");
            ret = FTB_Poll_event(shandle, &event);
            if (ret == FTB_SUCCESS) {
                printf("Caught event: comp_cat: %s, comp: %s, severity: %s, event_name: %s\n",
                   event.comp_cat, event.comp, event.severity, event.event_name);
                printf("From host: %s, pid: %d \n",
                   event.incoming_src.hostname, event.incoming_src.pid);
            }
            else {
                printf("No event\n");
                break;
            }
        }
    }
    printf("FTB_Disconnect\n");
    FTB_Disconnect(handle);

    printf("End\n");
    return 0;
}
