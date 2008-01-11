#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>
#include <string.h>

#include "libftb.h"
#include "ftb_ftb_examples_polling_logger_publishevents.h"

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
    FTB_comp_info_t cinfo;
    FTB_event_mask_t mask;
    int ret;
    char err_msg[8];
    
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

    /* Create the namespace string, jobid and inst_name before calling
     * FTB_Connect
     */
    strcpy(cinfo.comp_namespace,"FTB.FTB_EXAMPLES.POLLING_LOGGER");
    strcpy(cinfo.schema_ver, "0.5");
    strcpy(cinfo.inst_name,"abc");
    strcpy(cinfo.jobid,"1234");
    strcpy(cinfo.catch_style,"FTB_POLLING_CATCH");
    ret = FTB_Connect(&cinfo, &handle, err_msg);
    if (ret != FTB_SUCCESS) {
        printf("FTB_Connect failed \n");
        exit(-1);
    }

    FTB_Register_publishable_events(handle, ftb_ftb_examples_polling_logger_events, 
            FTB_FTB_EXAMPLES_POLLING_LOGGER_TOTAL_EVENTS, err_msg);
    
    ret = FTB_Create_mask(&mask, "all", "init", err_msg);
    if (ret != FTB_SUCCESS) { 
        printf("FTB_Create_mask failed - 1\n"); 
        exit(-1);
    }
    printf("Mask fields are event_name=%d severity=%d comp=%d comp_cat=%d hostname=%s inst_name=%s jobid=%s region=%s\n", 
            mask.event.event_name, mask.event.severity, mask.event.comp, mask.event.comp_cat, 
            mask.event.hostname, mask.event.inst_name, mask.event.jobid, mask.event.region);
    
    ret = FTB_Subscribe(handle, &mask, &shandle, err_msg, NULL, NULL);
    if (ret != FTB_SUCCESS) {
        printf("FTB_Subscribe failed\n");
        exit(-1);
    }

    signal(SIGINT, Int_handler);
    while(1) {
        FTB_catch_event_info_t event;
        int ret = 0;
        ret = FTB_Poll_event(shandle, &event, err_msg);
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
            fprintf(log_fp,"Caught event: comp_cat: %s, comp %s, severity: %s,  event_name %s, inst_name %s hostname %s jobid %s region %s ",
                   event.comp_cat, event.comp, event.severity, event.event_name, event.inst_name, 
                   event.hostname, event.jobid, event.region);
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

