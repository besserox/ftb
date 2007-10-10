#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>
#include <string.h>

#include "libftb.h"
#include "ftb_event_def.h"
#include "ftb_throw_events.h"

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
     * FTB_Init
     */
    strcpy(cinfo.comp_namespace,"FTB.FTB_EXAMPLES.POLLING_LOGGER");
    strcpy(cinfo.schema_ver, "0.5");
    strcpy(cinfo.inst_name,"abc");
    strcpy(cinfo.jobid,"1234");
    ret = FTB_Init(&cinfo, &handle, err_msg);

    
    ret = FTB_Create_mask(&mask, "all", "init", err_msg);
    if (ret != FTB_SUCCESS) { printf("Something is wrong\n"); exit(-1);}
    ret = FTB_Create_mask(&mask, "namespace", "FTB.ALL.ALL", err_msg);
    if (ret != FTB_SUCCESS) { printf("Something is wrong\n"); exit(-1);}
    FTB_Create_mask(&mask, "event_name", "WATCH_DOG_EVENT", err_msg);
    if (ret != FTB_SUCCESS) { printf("Something is wrong\n"); exit(-1);}
    ret = FTB_Create_mask(&mask,"hostname","all", err_msg);
    if (ret != FTB_SUCCESS) { printf("Something is wrong\n"); exit(-1);}
    ret = FTB_Create_mask(&mask,"inst_name", "all", err_msg);
    if (ret != FTB_SUCCESS) { printf("Something is wrong\n"); exit(-1);}
    ret = FTB_Create_mask(&mask, "jobid", "all", err_msg);
    if (ret != FTB_SUCCESS) { printf("Something is wrong\n"); exit(-1);}
    printf("Mask fields are event_name=%d severity=%d comp=%d comp_cat=%d hostname=%s inst_name=%s jobid=%s region=%s\n", mask.event.event_name,
            mask.event.severity, mask.event.comp, mask.event.comp_cat, mask.event.hostname, mask.event.inst_name, mask.event.jobid, mask.event.region);
    
    FTB_Subscribe(handle, &mask, &shandle, err_msg, NULL, NULL);

    signal(SIGINT, Int_handler);
    while(1) {
        FTB_catch_event_info_t event;
        FTB_id_t src;
        int ret = 0;
        ret = FTB_Poll_for_event(shandle, &event, err_msg);
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
                   event.comp_cat, event.comp, event.severity, event.event_name, event.inst_name, event.hostname, event.jobid, event.region);
            fprintf(log_fp,"from host %s, pid %d \n",event.incoming_src.hostname, event.incoming_src.pid);
            
            fflush(log_fp);
        }
        if (done)
            break;
    }
    FTB_Finalize(handle);
    fclose(log_fp);
    
    return 0;
}

