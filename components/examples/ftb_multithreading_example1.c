#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>
#include <string.h>
#include <pthread.h>

#include "libftb.h"


FTB_client_handle_t handle;

/*
 * The  callback_handle_recovery() gets one event, when called and
 * unsubscribes the handle
 *
 * Deadlocks may occur if FTB_Disconnect is called before by the main thread
 * before the callback thread calls FTB_Unsubscribe or any other FTB
 * function. It is up to the user to make sure that this does not happen
 */

int callback_handle_recovery(FTB_receive_event_t *evt, void *arg)
{
    FTB_subscribe_handle_t *shandle =(FTB_subscribe_handle_t *)arg;
    int ret = 0;
    printf("In callback_handle_recovery function comp_cat=%s comp=%s event_name=%s\n", 
            shandle->client_handle.client_id.comp_cat, shandle->client_handle.client_id.comp, 
            shandle->subscription_event.event_name);
    ret = FTB_Unsubscribe(shandle);
    if (ret != FTB_SUCCESS) {
        printf("FTB_Unsubscribe failed with code %d\n", ret);
        return ret;
    }

    return 0;
}


int main (int argc, char *argv[])
{
    FTB_client_t cinfo;
    int ret = 0 ;
    FTB_subscribe_handle_t *shandle=(FTB_subscribe_handle_t *)malloc(sizeof(FTB_subscribe_handle_t));
    FTB_event_info_t einfo[1] = {{"Exevent_memory_errors", "warning"}};
    FTB_event_handle_t ehandle;
    int k=0 ;

    strcpy(cinfo.event_space,"FTB.FTB_EXAMPLES.MULTITHREAD_EXAMPLE");
    strcpy(cinfo.client_subscription_style,"FTB_SUBSCRIPTION_NOTIFY");
    
    ret = FTB_Connect(&cinfo, &handle);
    if (ret != FTB_SUCCESS) {
        printf("FTB_Connect failed with return code = %d\n", ret);
        FTB_Disconnect(handle);
    }

    ret = FTB_Declare_publishable_events(handle, 0, einfo, 1);
    if (ret != FTB_SUCCESS) {
        printf("FTB_Declare_Publishable_event failed with return code=%d \n", ret);
        FTB_Disconnect(handle);

    }

    ret = FTB_Subscribe(shandle, handle, "event_name=ExEVENT_memory_Errors", callback_handle_recovery, (void*)shandle);
    if (ret != FTB_SUCCESS) {
        printf("FTB_Subscribe failed with return code=%d\n", ret);
        FTB_Disconnect(handle);
    }

    for (k=0; k<3; k++) {
        ret = FTB_Publish(handle, "ExEVENT_memory_errors", NULL, &ehandle);
        if (ret != FTB_SUCCESS) {
            printf("FTB_Publish_event failed with return code=%d\n", ret);
        }
    }

    //The sleep is a quick hack to ensure that the callback function
    //completes before FTB_Disconnect is called
    sleep (10);
    FTB_Disconnect(handle);
    
    return 0;
}

