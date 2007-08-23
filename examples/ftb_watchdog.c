#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>

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
    char *tag_data = "my_tag";
    char tag_data_recv[30];
    FTB_dynamic_len_t data_len = 30;

    FTB_Init(FTB_COMP_CTGY_BACKPLANE, FTB_COMP_WATCHDOG, &properties, &handle);
    FTB_Reg_throw(handle, WATCH_DOG_EVENT);
    FTB_Reg_catch_polling_event(handle, WATCH_DOG_EVENT);
    FTB_Add_dynamic_tag(handle, 1, tag_data, strlen(tag_data)+1);
    signal(SIGINT, Int_handler);
    while(1) {
        FTB_event_t evt;
        int ret = 0;
        FTB_Throw(handle, WATCH_DOG_EVENT);
        sleep(1);
        ret = FTB_Catch(handle, &evt, NULL);
        if (ret == FTB_CAUGHT_NO_EVENT) {
            fprintf(stderr,"Watchdog: event lost!\n");
            break;
        }
        FTB_Read_dynamic_tag(&evt, 1, tag_data_recv, &data_len);
        tag_data_recv[data_len] = '\0';
        printf("Watchdog: got watchdog event, tag: %s\n",tag_data_recv);
        sleep(10);
        if (done)
            break;
    }
    FTB_Finalize(handle);
    
    return 0;
}

