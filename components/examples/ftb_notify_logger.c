#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>

#include "libftb.h"
#include "ftb_event_def.h"
#include "ftb_throw_events.h"

#define LOG_FILE "/tmp/ftb_log"

static volatile int done = 0;

void Int_handler(int sig){
    if (sig == SIGINT)
        done = 1;
}

int event_logger(FTB_event_t *evt, FTB_id_t *src, void *arg)
{
    FILE* log_fp = (FILE*)arg;
    time_t current = time(NULL);
    fprintf(log_fp,"%s\t",asctime(localtime(&current)));
    fprintf(log_fp,"Caught event: comp_cat: %d, comp %d, severity: %d, event_cat %d, event_name %d, ",
            evt->comp_cat, evt->comp, evt->severity, evt->event_cat, evt->event_name);
    fprintf(log_fp,"from host %s, pid %d, comp_cat: %d, comp %d, extension %d\n",
           src->location_id.hostname, src->location_id.pid, src->client_id.comp_cat,
           src->client_id.comp, src->client_id.ext);
    fflush(log_fp);
    return 0;
}

int main (int argc, char *argv[])
{
    FILE *log_fp = NULL;
    FTB_component_properties_t properties;
    FTB_client_handle_t handle;
    FTB_event_t mask;

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
    
    properties.catching_type = FTB_EVENT_CATCHING_NOTIFICATION;
    properties.err_handling = FTB_ERR_HANDLE_NONE;

    FTB_EVENT_SET_ALL(mask);
    FTB_Init(FTB_EVENT_DEF_COMP_CAT_FTB_EXAMPLES, FTB_EVENT_DEF_COMP_NOTIFY_LOGGER,  &properties, &handle);
    FTB_Reg_catch_notify_mask(handle, &mask, event_logger, (void*)log_fp);

    signal(SIGINT, Int_handler);
    while(!done) {
        sleep(5);
    }
    FTB_Finalize(handle);
    fclose(log_fp);
    
    return 0;
}

