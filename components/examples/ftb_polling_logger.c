#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>
#include <string.h>

#include "libftb.h"

#define LOG_FILE "/tmp/ftb_log"

static volatile int done = 0;

void Int_handler(int sig){
    if (sig == SIGINT)
        done = 1;
}

int main (int argc, char *argv[])
{
    FILE *log_fp = NULL;
    FTB_client_handle_t handle;
    FTB_subscribe_handle_t shandle;
    FTB_client_t cinfo;
    FTB_event_mask_t mask;
    int ret;
    
    if (argc >= 2) {
        if (!strcmp("-", argv[1]))
            log_fp = stdout;
        else
            log_fp = fopen(argv[1],"w");
    }
    else {
        printf("use %s as log file",LOG_FILE);
        log_fp = fopen(LOG_FILE,"w");
    }

    if (log_fp == NULL) {
        fprintf(stderr,"failed to open file %s\n",argv[1]);
        return -1;
    }

    /* Create the namespace string, client_jobid and client_name before calling
     * FTB_Connect
     */
    strcpy(cinfo.event_space,"FTB.FTB_EXAMPLES.POLLING_LOGGER");
    strcpy(cinfo.client_schema_ver, "0.5");
    strcpy(cinfo.client_name,"abc");
    strcpy(cinfo.client_jobid,"1234");
    strcpy(cinfo.client_subscription_style,"FTB_SUBSCRIPTION_POLLING");
    ret = FTB_Connect(&cinfo, &handle);
    if (ret != FTB_SUCCESS) {
        printf("FTB_Connect failed \n");
        exit(-1);
    }

    ret = FTB_Create_mask(&mask, "all", "init");
    if (ret != FTB_SUCCESS) { 
        printf("FTB_Create_mask failed - 1\n"); 
        exit(-1);
    }
    printf("Catch Mask fields are event_name=%s severity=%s comp=%s comp_cat=%s hostname=%s client_name=%s client_jobid=%s region=%s\n", 
            mask.event.event_name, mask.event.severity, mask.event.comp, mask.event.comp_cat, 
            mask.event.hostname, mask.event.client_name, mask.event.client_jobid, mask.event.region);
    
    ret = FTB_Subscribe(handle, &mask, &shandle, NULL, NULL);
    if (ret != FTB_SUCCESS) {
        printf("FTB_Subscribe failed\n");
        exit(-1);
    }

    signal(SIGINT, Int_handler);
    while(1) {
        FTB_catch_event_info_t event;
        int ret = 0;
        ret = FTB_Poll_event(shandle, &event);
        if (ret == FTB_CAUGHT_NO_EVENT) {
            time_t current = time(NULL);
            fprintf(log_fp,"%s\t",asctime(localtime(&current)));
            fprintf(log_fp,"No event\n");
            fflush(log_fp);
            sleep(5);
        }
        else {
            time_t current = time(NULL);
            fprintf(log_fp,"%s\t",asctime(localtime(&current)));
            fprintf(log_fp, "Caught some event\n");
            fprintf(log_fp,"Caught event: comp_cat: %s, comp %s, severity: %s,  event_name %s, client_name %s hostname %s client_jobid %s region %s ",
                   event.comp_cat, event.comp, event.severity, event.event_name, event.client_name, 
                   event.hostname, event.client_jobid, event.region);
            fprintf(log_fp,"from host %s, pid %d \n",event.incoming_src.hostname, event.incoming_src.pid);
            
            fflush(log_fp);
        }
        if (done)
            break;
    }
    FTB_Disconnect(handle);
    fclose(log_fp);
    
    return 0;
}

