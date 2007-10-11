#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>
#include <string.h>

#include "libftb.h"
#include "ftb_ftb_examples_notify_logger_publishevents.h"

#define LOG_FILE "/tmp/ftb_log"

static volatile int done = 0;

void Int_handler(int sig){
    if (sig == SIGINT)
        done = 1;
}

int event_logger(FTB_catch_event_info_t *evt, void *arg)
{
    FILE* log_fp = (FILE*)arg;
    time_t current = time(NULL);
    fprintf(log_fp,"%s\t",asctime(localtime(&current)));
    fprintf(log_fp,"Caught event: region: %s comp_cat: %s, comp %s, severity: %s, event_name %s, ",
            evt->region, evt->comp_cat, evt->comp, evt->severity, evt->event_name);
    fflush(log_fp);
    return 0;
}

int main (int argc, char *argv[])
{
    FILE *log_fp = NULL;
    FTB_comp_info_t cinfo;
    FTB_client_handle_t handle;
    FTB_event_mask_t mask;
    FTB_subscribe_handle_t *shandle=(FTB_subscribe_handle_t *)malloc(sizeof(FTB_subscribe_handle_t));
    char err_msg[128];
    int ret = 0 ;

    if (argc >= 2) {
        log_fp = fopen(argv[1],"w");
    }
    else {
        fprintf(stderr,"use %s as log file\n",LOG_FILE);
        log_fp = fopen(LOG_FILE,"w");
    }

    if (log_fp == NULL) {
        fprintf(stderr,"failed to open file %s\n",argv[1]);
        return -1;
    }
    
    strcpy(cinfo.comp_namespace,"FTB.FTB_EXAMPLES.NOTIFY_LOGGER");
    strcpy(cinfo.schema_ver, "0.5"); 
    strcpy(cinfo.inst_name,"notify");
    strcpy(cinfo.jobid,"");
    
    ret = FTB_Init(&cinfo, &handle, err_msg);
    if (ret != FTB_SUCCESS) {
        printf("FTB_Init failed \n");
        exit(-1);
    }
    
    FTB_Register_publishable_events(handle, ftb_ftb_examples_notify_logger_events, 
                              FTB_FTB_EXAMPLES_NOTIFY_LOGGER_TOTAL_EVENTS, err_msg);
    
    ret = FTB_Create_mask(&mask, "all", "init", err_msg);
    if (ret != FTB_SUCCESS) { 
        printf("FTB_Create_mask failed - 1\n"); 
        exit(-1);
    }

    ret = FTB_Subscribe(handle, &mask, shandle, err_msg, event_logger, (void*)log_fp);
    if (ret != FTB_SUCCESS) {
        printf("FTB_Subscribe failed\n");
        exit(-1);
    }
    
    signal(SIGINT, Int_handler);
    while(!done) {
        sleep(5);
    }
    FTB_Finalize(handle);
    fclose(log_fp);
    
    return 0;
}

