#include <stdio.h>
#include <stdlib.h>

#include "libftb.h"
#include "ftb_event_def.h"

/*************  Component 3  ******************/

FTB_client_handle_t Comp3_ftb_handle;

int Comp3_evt_handler(FTB_event_t *evt, FTB_id_t *src, void *arg)
{
    printf("Comp3 caught event: comp_ctgy: %d, comp %d, severity: %d, event_ctgy %d, event_name %d, ",
            evt->comp_ctgy, evt->comp, evt->severity, evt->event_ctgy, evt->event_name);
    printf("from host %s, pid %d, comp_ctgy: %d, comp %d, extension %d\n",
           src->location_id.hostname, src->location_id.pid, src->client_id.comp_ctgy,
           src->client_id.comp, src->client_id.ext);
    return 0;
}

int Comp3_Init()
{
    FTB_component_properties_t properties;
    FTB_event_t mask;

    properties.catching_type = FTB_EVENT_CATCHING_NOTIFICATION;
    properties.err_handling = FTB_ERR_HANDLE_NONE;

    FTB_EVENT_MASK_ALL(mask);
    FTB_EVENT_CLR_SEVERITY(mask);
    FTB_EVENT_MARK_SEVERITY(mask, FTB_EVENT_DEF_SEVERITY_INFO);

    printf("Comp3: FTB_Init\n");
    FTB_Init(FTB_COMP_CTGY_BACKPLANE, FTB_TEST_COMP_3, &properties, &Comp3_ftb_handle);
    printf("Comp3: FTB_Reg_catch_notify_mask\n");
    FTB_Reg_catch_notify_mask(Comp3_ftb_handle, &mask, Comp3_evt_handler, NULL);
    
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

int Comp2_Init()
{
    FTB_component_properties_t properties;

    properties.catching_type = FTB_EVENT_CATCHING_POLLING;
    properties.err_handling = FTB_ERR_HANDLE_NONE;

    printf("Comp2: FTB_Init\n");
    FTB_Init(FTB_COMP_CTGY_BACKPLANE, FTB_TEST_COMP_2, &properties, &Comp2_ftb_handle);
    printf("Comp2: FTB_Reg_catch_polling_event\n");
    FTB_Reg_catch_polling_event(Comp2_ftb_handle, FTB_TEST_EVENT_1);

    Comp3_Init();

    return 0;
}

int Comp2_Func()
{
    FTB_event_t evt;
    FTB_id_t src;

    Comp3_Func();
    while(1) {
       int ret = FTB_Catch(Comp2_ftb_handle, &evt, &src);
       if (ret == FTB_CAUGHT_NO_EVENT) {
           break;
       }
       printf("Comp2 caught event: comp_ctgy: %d, comp %d, severity: %d, event_ctgy %d, event_name %d, ",
            evt.comp_ctgy, evt.comp, evt.severity, evt.event_ctgy, evt.event_name);
       printf("from host %s, pid %d, comp_ctgy: %d, comp %d, extension %d\n",
           src.location_id.hostname, src.location_id.pid, src.client_id.comp_ctgy,
           src.client_id.comp, src.client_id.ext);
    }

    printf("Comp2: FTB_Throw\n");
    FTB_Throw(Comp2_ftb_handle, FTB_TEST_EVENT_2);

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

int Comp1_evt_handler(FTB_event_t *evt, FTB_id_t *src, void *arg)
{
    printf("Comp1 caught event: comp_ctgy: %d, comp %d, severity: %d, event_ctgy %d, event_name %d, ",
            evt->comp_ctgy, evt->comp, evt->severity, evt->event_ctgy, evt->event_name);
    printf("from host %s, pid %d, comp_ctgy: %d, comp %d, extension %d\n",
           src->location_id.hostname, src->location_id.pid, src->client_id.comp_ctgy,
           src->client_id.comp, src->client_id.ext);
    return 0;
}

int Comp1_Init()
{
    FTB_component_properties_t properties;
    FTB_event_t mask;

    properties.catching_type = FTB_EVENT_CATCHING_NOTIFICATION;
    properties.err_handling = FTB_ERR_HANDLE_NONE;

    FTB_EVENT_MASK_ALL(mask);
    FTB_EVENT_CLR_SEVERITY(mask);
    FTB_EVENT_MARK_SEVERITY(mask, FTB_EVENT_DEF_SEVERITY_FATAL);

    printf("Comp1: FTB_Init\n");
    FTB_Init(FTB_COMP_CTGY_BACKPLANE, FTB_TEST_COMP_1, &properties, &Comp1_ftb_handle);
    printf("Comp1: FTB_Reg_catch_notify_mask\n");
    FTB_Reg_catch_notify_mask(Comp1_ftb_handle, &mask, Comp1_evt_handler, NULL);

    Comp2_Init();    

    return 0;
}

int Comp1_Func()
{
    static int i=0;
    i++;
    if (i%5 == 0) {
        printf("Comp1: FTB_Throw\n");
        FTB_Throw(Comp1_ftb_handle, FTB_TEST_EVENT_1);
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
