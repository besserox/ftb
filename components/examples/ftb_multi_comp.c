#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "libftb.h"
#include "ftb_ftb_examples_multicomp_comp1_publishevents.h"
#include "ftb_ftb_examples_multicomp_comp2_publishevents.h"
#include "ftb_ftb_examples_multicomp_comp3_publishevents.h"

char err_msg1[FTB_MAX_ERRMSG_LEN];
char err_msg2[FTB_MAX_ERRMSG_LEN];
char err_msg3[FTB_MAX_ERRMSG_LEN];
/*************  Component 3  ******************/

FTB_client_handle_t Comp3_ftb_handle;
FTB_subscribe_handle_t shandle3;

int Comp3_evt_handler(FTB_catch_event_info_t *evt, void *arg)
{
    printf("Comp3 caught event: comp_cat: %s, comp %s, severity: %s, event_name %s, ",
            evt->comp_cat, evt->comp, evt->severity, evt->event_name);
    printf("from host %s, pid %d\n",
           evt->incoming_src.hostname, evt->incoming_src.pid);
    return 0;
}

int Comp3_Init()
{
    FTB_event_mask_t mask;
    int ret = 0;
    FTB_comp_info_t cinfo;
    
    printf("Comp3: Create mask\n");
    ret = FTB_Create_mask(&mask, "all", "init", err_msg3);
    if (ret != FTB_SUCCESS) {
        printf("Mask creation failed for field_name=all and field value=init \n");
        exit(-1);
    }
    ret = FTB_Create_mask(&mask, "severity", "ftb_info", err_msg3);
    if (ret != FTB_SUCCESS) {
        printf("Mask creation failed for field val=ftb_info\n");
        exit(-1);
    }

    printf("Comp3: FTB_Init\n");
    strcpy(cinfo.comp_namespace, "FTB.FTB_EXAMPLES.MULTICOMP_COMP3");
    strcpy(cinfo.schema_ver, "0.5");
    strcpy(cinfo.inst_name, "");
    strcpy(cinfo.jobid,"");
    strcpy(cinfo.catch_style,"FTB_NOTIFY_CATCH");
    
    ret = FTB_Init(&cinfo, &Comp3_ftb_handle, err_msg3);
    if (ret != FTB_SUCCESS) {
        printf("FTB_Init failed\n");
        exit(-1);
    }

    printf("Comp3: Register_publishable_events\n");
    FTB_Register_publishable_events(Comp3_ftb_handle, ftb_ftb_examples_multicomp_comp3_events, FTB_FTB_EXAMPLES_MULTICOMP_COMP3_TOTAL_EVENTS, err_msg3);
    
    printf("Comp3: FTB_Subscribe \n");
    ret = FTB_Subscribe(Comp3_ftb_handle, &mask, &shandle3, err_msg3, Comp3_evt_handler, NULL);
    if (ret != FTB_SUCCESS) {
         printf("FTB_Subscribe failed!\n"); exit(-1);
    }
    
    return 0;
}

int Comp3_Func()
{
    sleep(1);
    return 0;
}

int Comp3_Finalize()
{
    printf("Comp3: FTB_Finalize\n");
    FTB_Finalize(Comp3_ftb_handle);

    return 0;
}

/*************  Component 3 End ***************/


/*************  Component 2  ******************/

FTB_client_handle_t Comp2_ftb_handle;
FTB_subscribe_handle_t shandle2;

int Comp2_Init()
{
    FTB_comp_info_t cinfo;
    FTB_event_mask_t mask;
    int ret = 0;

    printf("Comp2: FTB_Init\n");
    strcpy(cinfo.comp_namespace, "FTB.FTB_EXAMPLES.MULTICOMP_COMP2");
    strcpy(cinfo.schema_ver, "0.5");
    strcpy(cinfo.inst_name, "");
    strcpy(cinfo.jobid,"");
    strcpy(cinfo.catch_style,"FTB_POLLING_CATCH");
    ret = FTB_Init(&cinfo, &Comp2_ftb_handle, err_msg2);
    if (ret != FTB_SUCCESS) {
        printf("FTB_Init failed\n");
        exit(-1);
    }
   
    printf("Comp2: Register_publishable_events\n");
    FTB_Register_publishable_events(Comp2_ftb_handle, ftb_ftb_examples_multicomp_comp2_events, FTB_FTB_EXAMPLES_MULTICOMP_COMP2_TOTAL_EVENTS, err_msg2);
    
    printf("Comp2: Create mask\n");
    ret = FTB_Create_mask(&mask, "all", "init", err_msg2);
    if (ret != FTB_SUCCESS) {
        printf("Mask creation failed for field_name=all and field value=init \n");
        exit(-1);
    }
    ret = FTB_Create_mask(&mask, "event_name", "TEST_EVENT_1", err_msg2);
    if (ret != FTB_SUCCESS) {
        printf("Mask creation failed \n");
        exit(-1);
    }
    
    printf("Comp2: FTB_Subscribe via polling\n");
    ret = FTB_Subscribe(Comp2_ftb_handle, &mask, &shandle2, err_msg2, NULL, NULL);
    if (ret != FTB_SUCCESS) {
         printf("FTB_Subscribe failed!\n"); exit(-1);
    }

    Comp3_Init();

    return 0;
}

int Comp2_Func()
{
    FTB_catch_event_info_t evt;
    int ret = 0;

    Comp3_Func();
    while(1) {
       ret = FTB_Poll_for_event(shandle2, &evt, err_msg2);
       if (ret == FTB_CAUGHT_NO_EVENT) {
           break;
       }
       printf("Comp2 caught event: comp_cat: %s, comp %s, severity: %s, event_name %s, ",
            evt.comp_cat, evt.comp, evt.severity, evt.event_name);
       printf("from host %s, pid %d \n",
           evt.incoming_src.hostname, evt.incoming_src.pid);
    }

    printf("Comp2: FTB_Publish_event\n");
    FTB_Publish_event(Comp2_ftb_handle, "TEST_EVENT_2", NULL, err_msg2);

    return 0;
}

int Comp2_Finalize()
{
    Comp3_Finalize();

    printf("Comp2: FTB_Finalize\n");
    FTB_Finalize(Comp2_ftb_handle);
    
    return 0;
}

/*************  Component 2 End ***************/


/*************  Component 1  ******************/
FTB_client_handle_t Comp1_ftb_handle;
FTB_subscribe_handle_t shandle1;

int Comp1_evt_handler(FTB_catch_event_info_t *evt, void *arg)
{
    printf("Comp1 caught event: comp_cat: %s, comp %s, severity: %s, event_name %s, ",
            evt->comp_cat, evt->comp, evt->severity, evt->event_name);
    printf("from hostname %s, pid %d \n",
           evt->incoming_src.hostname, evt->incoming_src.pid);
    return 0;
}

int Comp1_Init()
{
    FTB_event_mask_t mask;
    FTB_comp_info_t cinfo;
    int ret = 0;

    printf("Comp1: FTB_Init\n");
    strcpy(cinfo.comp_namespace, "FTB.FTB_EXAMPLES.MULTICOMP_COMP1");
    strcpy(cinfo.schema_ver, "0.5");
    strcpy(cinfo.inst_name, "");
    strcpy(cinfo.jobid,"");
    strcpy(cinfo.catch_style,"FTB_NOTIFY_CATCH");
    ret = FTB_Init(&cinfo, &Comp1_ftb_handle, err_msg1);
    if (ret != FTB_SUCCESS) {
        printf("FTB_Init failed\n");
        exit(-1);
    }
    
    printf("Comp1: Register_publishable_events\n");
    FTB_Register_publishable_events(Comp1_ftb_handle, ftb_ftb_examples_multicomp_comp1_events, FTB_FTB_EXAMPLES_MULTICOMP_COMP1_TOTAL_EVENTS, err_msg1);
    
    ret = FTB_Create_mask(&mask, "all", "init", err_msg1);
    if (ret != FTB_SUCCESS) {
        printf("Mask creation failed for field_name=all and field value=init \n");
        exit(-1);
    }
    ret = FTB_Create_mask(&mask, "severity", "ftb_fatal", err_msg1);
    if (ret != FTB_SUCCESS) {
        printf("Mask creation failed for field val=ftb_fatal\n");
        exit(-1);
    }
    
    printf("Comp1: FTB_Subscribe \n");
    ret = FTB_Subscribe(Comp1_ftb_handle, &mask, &shandle1, err_msg1, Comp1_evt_handler, NULL);
    if (ret != FTB_SUCCESS) {
         printf("FTB_Subscribe failed!\n"); 
         exit(-1);
    }

    Comp2_Init();    

    return 0;
}

int Comp1_Func()
{
    static int i=0;
    i++;
    if (i%5 == 0) {
        printf("Comp1: FTB_Publish_event\n");
        FTB_Publish_event(Comp1_ftb_handle, "TEST_EVENT_1", NULL, err_msg1);
    }

    Comp2_Func();

    return 0;
}

int Comp1_Finalize()
{
    Comp2_Finalize();

    printf("Comp1: FTB_Finalize\n");
    FTB_Finalize(Comp1_ftb_handle);

    return 0;
}

/*************  Component 1 End ***************/


int main (int argc, char *argv[])
{
    int i;

    Comp1_Init();

    for (i=0;i<20;i++) {
        Comp1_Func();
    }

    Comp1_Finalize(); 

    return 0;
}

