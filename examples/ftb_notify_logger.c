#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

#include "libftb.h"
#include "ftb_event_def.h"

static volatile int done = 0;

void Int_handler(int sig){
    if (sig == SIGINT)
        done = 1;
}

int event_logger(FTB_event_t *evt, FTB_id_t *src, void *arg)
{
    printf("Caught event: comp_ctgy: %d, comp %d, severity: %d, event_ctgy %d, event_name %d\n",
        evt->comp_ctgy, evt->comp, evt->severity, evt->event_ctgy, evt->event_name);
    printf("From host %s, pid %d, comp_ctgy: %d, comp %d, extension %d\n",
        src->location_id.hostname, src->location_id.pid, src->client_id.comp_ctgy, 
        src->client_id.comp, src->client_id.ext);
    return 0;
}

int main (int argc, char *argv[])
{
    FTB_component_properties_t properties;
    FTB_client_handle_t handle;
    FTB_event_t mask;
    
    properties.catching_type = FTB_EVENT_CATCHING_NOTIFICATION;
    properties.err_handling = FTB_ERR_HANDLE_NONE;

    FTB_EVENT_MASK_ALL(mask);
    FTB_Init(FTB_COMP_CTGY_BACKPLANE, FTB_COMP_NOTIFY_LOGGER, &properties, &handle);
    FTB_Reg_catch_notify_mask(handle, &mask, event_logger, NULL);

    signal(SIGINT, Int_handler);
    while(!done) {
        sleep(5);
    }
    FTB_Finalize(handle);
    
    return 0;
}

