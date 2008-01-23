#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "libftb.h"

/*************  Component 3  ******************/

FTB_client_handle_t Comp3_ftb_handle;
FTB_subscribe_handle_t shandle3;

int Comp3_evt_handler(FTB_receive_event_t *evt, void *arg)
{
    printf("Comp3 caught event: comp_cat: %s, comp %s, severity: %s, event_name %s, ",
            evt->comp_cat, evt->comp, evt->severity, evt->event_name);
    printf("from host %s, pid %d\n",
           evt->incoming_src.hostname, evt->incoming_src.pid);
    return 0;
}

int Comp3_Init()
{
    int ret = 0;
    FTB_client_t cinfo;
    FTB_event_info_t event_info[1] = {{"TEST_EVENT_3", "INFO"}};
    
    strcpy(cinfo.event_space, "FTB.FTB_EXAMPLES.MULTICOMP_COMP3");
    strcpy(cinfo.client_schema_ver, "0.5");
    strcpy(cinfo.client_name, "");
    strcpy(cinfo.client_jobid,"");
    strcpy(cinfo.client_subscription_style,"FTB_SUBSCRIPTION_NOTIFY");
    
    printf("Comp3: FTB_Connect\n");
    ret = FTB_Connect(&cinfo, &Comp3_ftb_handle);
    if (ret != FTB_SUCCESS) {
        printf("FTB_Connect failed\n");
        exit(-1);
    }

    ret = FTB_Declare_publishable_events(Comp3_ftb_handle, 0, event_info, 1); 
    if (ret != FTB_SUCCESS) {
        printf("FTB_Declare_publishable_events failed ret=%d!\n", ret); exit(-1);
    }   

    printf("Comp3: FTB_Subscribe \n");
    ret = FTB_Subscribe(&shandle3, Comp3_ftb_handle, "severity=info", Comp3_evt_handler, NULL);
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
    printf("Comp3: FTB_Disconnect\n");
    FTB_Disconnect(Comp3_ftb_handle);

    return 0;
}

/*************  Component 3 End ***************/


/*************  Component 2  ******************/

FTB_client_handle_t Comp2_ftb_handle;
FTB_subscribe_handle_t shandle2;

int Comp2_Init()
{
    FTB_client_t cinfo;
    FTB_event_info_t event_info[1] = {{"TEST_EVENT_2", "INFO"}};
    int ret = 0;

    printf("Comp2: FTB_Connect\n");
    strcpy(cinfo.event_space, "FTB.FTB_EXAMPLES.MULTICOMP_COMP2");
    strcpy(cinfo.client_schema_ver, "0.5");
    strcpy(cinfo.client_name, "");
    strcpy(cinfo.client_jobid,"");
    strcpy(cinfo.client_subscription_style,"FTB_SUBSCRIPTION_POLLING");

    ret = FTB_Connect(&cinfo, &Comp2_ftb_handle);
    if (ret != FTB_SUCCESS) {
        printf("FTB_Connect failed\n");
        exit(-1);
    }
    
    ret = FTB_Declare_publishable_events(Comp2_ftb_handle, 0, event_info, 1); 
    if (ret != FTB_SUCCESS) {
        printf("FTB_Declare_publishable_events failed ret=%d!\n", ret); exit(-1);
    }   
    
    printf("Comp2: FTB_Subscribe via polling\n");
    ret = FTB_Subscribe(&shandle2, Comp2_ftb_handle, "event_name=TEST_EVENT_1", NULL, NULL);
    if (ret != FTB_SUCCESS) {
         printf("FTB_Subscribe failed!\n"); exit(-1);
    }

    Comp3_Init();

    return 0;
}

int Comp2_Func()
{
    FTB_receive_event_t evt;
    FTB_event_handle_t ehandle;
    int ret = 0;
    static int count=0;

    Comp3_Func();
    while(1) {
       ret = FTB_Poll_event(shandle2, &evt);
       if (ret == FTB_GOT_NO_EVENT) {
           break;
       }
       count++;
       printf("Comp2 caught event: comp_cat: %s, comp: %s, severity: %s, event_name %s, count=%d ++++++++++++++ ",
            evt.comp_cat, evt.comp, evt.severity, evt.event_name, count);
       printf("from host %s, pid %d \n",
           evt.incoming_src.hostname, evt.incoming_src.pid);
        if (count == 2) {
            ret = FTB_Unsubscribe(&shandle3);
            if (ret != FTB_SUCCESS) {
                printf("Return value is bnot success\n");
                exit(-1);
            }
            else printf("************************\n");
        }
    }

    printf("Comp2: FTB_Publish\n");
    FTB_Publish(Comp2_ftb_handle, "TEST_EVENT_2", NULL, &ehandle);
    return 0;
}

int Comp2_Finalize()
{
    Comp3_Finalize();

    printf("Comp2: FTB_Disconnect\n");
    FTB_Disconnect(Comp2_ftb_handle);
    
    return 0;
}

/*************  Component 2 End ***************/


/*************  Component 1  ******************/
FTB_client_handle_t Comp1_ftb_handle;
FTB_subscribe_handle_t shandle1;

int Comp1_evt_handler(FTB_receive_event_t *evt, void *arg)
{
    printf("Comp1 caught event: comp_cat: %s, comp %s, severity: %s, event_name %s, ",
            evt->comp_cat, evt->comp, evt->severity, evt->event_name);
    printf("from hostname %s, pid %d \n",
           evt->incoming_src.hostname, evt->incoming_src.pid);
    return 0;
}

int Comp1_Init()
{
    FTB_client_t cinfo;
    FTB_event_info_t event_info[1] = {{"TEST_EVENT_1", "INFO"}};
    int ret = 0;

    printf("Comp1: FTB_Connect\n");
    strcpy(cinfo.event_space, "FTB.FTB_EXAMPLES.MULTICOMP_COMP1");
    strcpy(cinfo.client_schema_ver, "0.5");
    strcpy(cinfo.client_name, "");
    strcpy(cinfo.client_jobid,"");
    strcpy(cinfo.client_subscription_style,"FTB_SUBSCRIPTION_NOTIFY");

    ret = FTB_Connect(&cinfo, &Comp1_ftb_handle);
    if (ret != FTB_SUCCESS) {
        printf("FTB_Connect failed\n");
        exit(-1);
    }
    
    ret = FTB_Declare_publishable_events(Comp1_ftb_handle, 0, event_info, 1); 
    if (ret != FTB_SUCCESS) {
        printf("FTB_Declare_publishable_events failed ret=%d!\n", ret); exit(-1);
    }   

    printf("Comp1: FTB_Subscribe \n");
    ret = FTB_Subscribe(&shandle1, Comp1_ftb_handle, "severity=fatal", Comp1_evt_handler, NULL);
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
    FTB_event_handle_t ehandle;
    i++;
    if (i%5 == 0) {
        printf("Comp1: FTB_Publish\n");
        FTB_Publish(Comp1_ftb_handle, "TEST_EVENT_1", NULL, &ehandle);
    }

    Comp2_Func();

    return 0;
}

int Comp1_Finalize()
{
    Comp2_Finalize();

    printf("Comp1: FTB_Disconnect\n");
    FTB_Disconnect(Comp1_ftb_handle);

    return 0;
}

/*************  Component 1 End ***************/


int main (int argc, char *argv[])
{
    int i;

    Comp1_Init();

    for (i=0;i<40;i++) {
        Comp1_Func();
    }

    Comp1_Finalize(); 

    return 0;
}

