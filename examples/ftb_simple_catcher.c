#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

#include "libftb.h"
#include "ftb_event_def.h"

int main (int argc, char *argv[])
{
    FTB_component_properties_t properties;
    FTB_client_handle_t handle;
    int i;

    printf("Begin\n");
    properties.catching_type = FTB_EVENT_CATCHING_POLLING;
    properties.err_handling = FTB_ERR_HANDLE_NONE;

    printf("FTB_Init\n");
    FTB_Init(FTB_COMP_CTGY_BACKPLANE, FTB_COMP_SIMPLE, &properties, &handle);
    printf("FTB_Reg_catch_polling_event\n");
    FTB_Reg_catch_polling_event(handle, SIMPLE_EVENT);
    for(i=0;i<12;i++) {
        int ret;
        FTB_event_t event;
        FTB_id_t src;
        printf("sleep(10)\n");
        sleep(10);
        while(1) {
            printf("FTB_Catch\n");
            ret = FTB_Catch(handle, &event, &src);
            if (ret == FTB_CAUGHT_EVENT) {
                printf("Caught event: comp_ctgy: %d, comp %d, severity: %d, event_ctgy %d, event_name %d\n",
                   event.comp_ctgy, event.comp, event.severity, event.event_ctgy, event.event_name);
                printf("From host %s, pid %d, comp_ctgy: %d, comp %d, extension %d\n",
                   src.location_id.hostname, src.location_id.pid, src.client_id.comp_ctgy,
                   src.client_id.comp, src.client_id.ext);
            }
            else {
                printf("No event\n");
                break;
            }
        }
    }
    printf("FTB_Finalize\n");
    FTB_Finalize(handle);

    printf("End\n");
    return 0;
}
