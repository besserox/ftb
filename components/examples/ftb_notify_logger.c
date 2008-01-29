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

int event_logger(FTB_receive_event_t *evt, void *arg)
{
    FILE* log_fp = (FILE*)arg;
    time_t current = time(NULL);
    fprintf(log_fp,"%s\t",asctime(localtime(&current)));
    fprintf(log_fp,"Caught event: event_space %s, severity: %s, event_name %s from hostname=%s and pid=%d ",
            evt->event_space, evt->severity, evt->event_name, evt->incoming_src.hostname, evt->incoming_src.pid);
    fflush(log_fp);
    return 0;
}

int main (int argc, char *argv[])
{
    FILE *log_fp = NULL;
    FTB_client_t cinfo;
    FTB_client_handle_t handle;
    FTB_subscribe_handle_t *shandle=(FTB_subscribe_handle_t *)malloc(sizeof(FTB_subscribe_handle_t));
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
    
    strcpy(cinfo.event_space,"FTB.FTB_EXAMPLES.NOTIFY_LOGGER");
    strcpy(cinfo.client_schema_ver, "0.5"); 
    strcpy(cinfo.client_name,"notify");
    strcpy(cinfo.client_jobid,"");
    strcpy(cinfo.client_subscription_style,"FTB_SUBSCRIPTION_NOTIFY");
    
    ret = FTB_Connect(&cinfo, &handle);
    if (ret != FTB_SUCCESS) {
        printf("FTB_Connect failed \n");
        exit(-1);
    }
    
    ret = FTB_Subscribe(shandle, handle, "", event_logger, (void*)log_fp);
    if (ret != FTB_SUCCESS) {
        printf("FTB_Subscribe failed\n");
        exit(-1);
    }
    
    signal(SIGINT, Int_handler);
    while(!done) {
        sleep(5);
    }
    FTB_Disconnect(handle);
    fclose(log_fp);
    
    return 0;
}

