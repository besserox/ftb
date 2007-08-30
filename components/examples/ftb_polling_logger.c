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
        printf("use %s as log file",LOG_FILE);
        log_fp = fopen(LOG_FILE,"w");
    }

    if (log_fp == NULL) {
        fprintf(stderr,"failed to open file %s\n",argv[1]);
        return -1;
    }

    properties.catching_type = FTB_EVENT_CATCHING_POLLING;
    properties.err_handling = FTB_ERR_HANDLE_NONE;
    properties.max_event_queue_size = FTB_DEFAULT_EVENT_POLLING_Q_LEN;

    FTB_EVENT_MASK_ALL(mask);
    FTB_Init(FTB_EVENT_DEF_COMP_CTGY_FTB_EXAMPLES, FTB_EVENT_DEF_COMP_POLLING_LOGGER, &properties, &handle);
    FTB_Reg_catch_polling_mask(handle, &mask);

    signal(SIGINT, Int_handler);
    while(1) {
        FTB_event_t event;
        FTB_id_t src;
        int ret = 0;
        ret = FTB_Catch(handle, &event, &src);
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
            fprintf(log_fp,"Caught event: comp_ctgy: %d, comp %d, severity: %d, event_ctgy %d, event_name %d, ",
                   event.comp_ctgy, event.comp, event.severity, event.event_ctgy, event.event_name);
            fprintf(log_fp,"from host %s, pid %d, comp_ctgy: %d, comp %d, extension %d\n",
                   src.location_id.hostname, src.location_id.pid, src.client_id.comp_ctgy,
                   src.client_id.comp, src.client_id.ext);
            fflush(log_fp);
        }
        if (done)
            break;
    }
    FTB_Finalize(handle);
    fclose(log_fp);
    
    return 0;
}

