#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>

#include "libftb.h"
#include "ftb_event_def.h"
#include "ftb_throw_events.h"

int main (int argc, char *argv[])
{
    FTB_component_properties_t properties;
    FTB_client_handle_t handle;
    int i;
    char err_msg[1024];

    printf("Begin\n");
    properties.catching_type = FTB_EVENT_CATCHING_NONE;
    properties.err_handling = FTB_ERR_HANDLE_NONE;
    FTB_comp_info_t cinfo;
    strcpy(cinfo.comp_namespace,"FTB.FTB_EXAMPLES.SIMPLE");
    strcpy(cinfo.schema_ver, "0.5");
    strcpy(cinfo.inst_name,"abc");
    strcpy(cinfo.jobid,"1234");

    printf("cinfo.comp_namespace=%s cinfo.schema_ver=%s cinfo.inst_name=%s cinfo.jobid=%s\n",cinfo.comp_namespace,
                                            cinfo.schema_ver,cinfo.inst_name,cinfo.jobid);
    int ret=0;
    ret=FTB_Init(&cinfo, &handle,err_msg);
    printf("FTB handle is %d\n",(int)handle);
    //FTB_Init(FTB_EXAMPLES, SIMPLE, &properties, &handle);
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
