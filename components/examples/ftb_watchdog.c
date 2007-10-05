#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>

#include "libftb.h"
#include "ftb_event_def.h"
#include "ftb_throw_events.h"

static volatile int done = 0;
char err_msg[FTB_MAX_ERRMSG_LEN];

void Int_handler(int sig){
    if (sig == SIGINT)
        done = 1;
}

int main (int argc, char *argv[])
{
    FTB_comp_info_t cinfo;
    FTB_id_t src;
    char *tag_data = "my_tag";

    FTB_client_handle_t handle;
    char tag_data_recv[30];
    FTB_dynamic_len_t data_len = 30;
    
    strcpy(cinfo.comp_namespace,"FTB.FTB_EXAMPLES.FTB_WAtchdog");
    strcpy(cinfo.schema_ver, "0.5");
    strcpy(cinfo.inst_name,"abc");
    strcpy(cinfo.jobid,"1234");
    if (FTB_Init(&cinfo, &handle, err_msg)!=FTB_SUCCESS) {
        printf("Invalid namespace format %s\n",cinfo.comp_namespace);
        exit(-1);
    }
    FTB_Reg_throw(handle, "WATCH_DOG_EVENT");
    FTB_Reg_catch_polling_event(handle, "WATCH_DOG_EVENT");
    FTB_Add_dynamic_tag(handle, 1, tag_data, strlen(tag_data)+1);
    signal(SIGINT, Int_handler);
    while(1) {
        FTB_event_t evt;
        int ret = 0;
        FTB_Publish_event(handle, "WATCH_DOG_EVENT",NULL, err_msg);
        sleep(1);
        ret = FTB_Catch(handle, &evt, &src);
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

