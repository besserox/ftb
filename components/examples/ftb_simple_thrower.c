#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>

#include "libftb.h"



int main (int argc, char *argv[])
{
    FTB_client_t cinfo;
    FTB_client_handle_t handle;
    FTB_event_handle_t ehandle;
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

    for (i=0;i<12;i++) {
        printf("FTB_Publish\n");
        ret = FTB_Publish(handle, "SIMPLE_EVENT", NULL, &ehandle);
        if (ret != FTB_SUCCESS) {
            printf("FTB_Publish did not return a success\n");
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
