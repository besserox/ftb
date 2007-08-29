#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

#include "libftb.h"
#include "ftb_event_def.h"
#include "ftb_throw_events.h"

int main (int argc, char *argv[])
{
    FTB_component_properties_t properties;
    FTB_client_handle_t handle;
    int i;

    printf("Begin\n");
    properties.catching_type = FTB_EVENT_CATCHING_NONE;
    properties.err_handling = FTB_ERR_HANDLE_NONE;

    printf("FTB_Init\n");
    FTB_Init(FTB_EVENT_DEF_COMP_CTGY_FTB_EXAMPLES, FTB_EVENT_DEF_COMP_SIMPLE, &properties, &handle);
    printf("FTB_Reg_throw\n");
    FTB_Reg_throw(handle, "SIMPLE_EVENT");
    for (i=0;i<12;i++) {
        printf("FTB_Throw\n");
        FTB_Throw(handle, "SIMPLE_EVENT");
        printf("sleep(10)\n");
        sleep(10);
    }
    printf("FTB_Finalize\n");
    FTB_Finalize(handle);

    printf("End\n");
    return 0;
}
