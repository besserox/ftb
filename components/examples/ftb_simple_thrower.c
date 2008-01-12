#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>

#include "libftb.h"
#include "ftb_ftb_examples_simple_publishevents.h"


char err_msg[FTB_MAX_ERRMSG_LEN];

int main (int argc, char *argv[])
{
    FTB_client_t cinfo;
    FTB_client_handle_t handle;
    int ret=0;
    int i;

    printf("Begin\n");
    
    strcpy(cinfo.event_space,"FTB.FTB_EXAMPLES.SIMPLE");
    strcpy(cinfo.client_schema_ver, "0.5");
    strcpy(cinfo.client_name,"");
    strcpy(cinfo.client_jobid,"");
    strcpy(cinfo.client_subscription_style, "FTB_SUBSCRIPTION_NONE");

    ret = FTB_Connect(&cinfo, &handle);
    if (ret != FTB_SUCCESS) {
        printf("FTB_Connect did not return a success\n");
            exit(-1);
    }

    ret = FTB_Register_publishable_events(handle, ftb_ftb_examples_simple_events,
                    FTB_FTB_EXAMPLES_SIMPLE_TOTAL_EVENTS, err_msg);
    
    for (i=0;i<12;i++) {
        printf("FTB_Publish_event\n");
        ret = FTB_Publish_event(handle, "SIMPLE_EVENT", NULL, err_msg);
        if (ret != FTB_SUCCESS) {
            printf("FTB_Publish_event did not return a success\n");
            exit(-1);
        }
        printf("sleep(10)\n");
        sleep(10);
    }
    printf("FTB_Disconnect\n");
    FTB_Disconnect(handle);

    printf("End\n");
    return 0;
}
