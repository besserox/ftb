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

int main (int argc, char *argv[])
{
    FTB_component_properties_t properties;
    FTB_client_handle_t handle;
    FTB_event_t mask;
    
    properties.catching_type = FTB_EVENT_CATCHING_POLLING;
    properties.err_handling = FTB_ERR_HANDLE_NONE;

    FTB_EVENT_MASK_ALL(mask);
    FTB_Init(FTB_COMP_CTGY_BACKPLANE, FTB_COMP_POLLING_LOGGER, &properties, &handle);
    FTB_Reg_catch_polling_mask(handle, &mask);

    signal(SIGINT, Int_handler);
    while(1) {
        FTB_event_t event;
        int ret = 0;
        ret = FTB_Catch(handle, &event);
        if (ret == FTB_CAUGHT_NO_EVENT) {
            printf("No event\n");
            sleep(5);
        }
        else {
            printf("Caught event: comp_ctgy: %d, comp %d, severity: %d, event_ctgy %d, event_name %d\n",
                   event.comp_ctgy, event.comp, event.severity, event.event_ctgy, event.event_name);
        }
        if (done)
            break;
    }
    FTB_Finalize(handle);
    
    return 0;
}

