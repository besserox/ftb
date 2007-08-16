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
    properties.catching_type = FTB_EVENT_CATCHING_POLLING;
    properties.err_handling = FTB_ERR_HANDLE_NONE;

    FTB_Init(FTB_COMP_CTGY_BACKPLANE, FTB_COMP_WATCHDOG, &properties, &handle);
    FTB_Reg_throw(handle, WATCH_DOG_EVENT);
    FTB_Reg_catch_polling_event(handle, WATCH_DOG_EVENT);

    signal(SIGINT, Int_handler);
    while(1) {
        FTB_event_t evt;
        int ret = 0;
        FTB_Throw(handle, WATCH_DOG_EVENT);
        sleep(1);
        ret = FTB_Catch(handle, &evt);
        if (ret == FTB_CAUGHT_NO_EVENT) {
            fprintf(stderr,"Watchdog: event lost!\n");
            break;
        }
        printf("Watchdog: got watchdog event\n");
        sleep(10);
        if (done)
            break;
    }
    FTB_Finalize(handle);
    
    return 0;
}

