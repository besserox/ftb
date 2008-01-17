#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/time.h>
#include <string.h>
#include "libftb.h"

static volatile int done = 0;
static struct timeval begin, end;
static int is_server = 0;
static int count = 0; 
static int iter = 0;

void Sig_handler(int sig){
    if (sig == SIGINT || sig == SIGTERM)
        done = 1;
}

int pingpong_server(FTB_catch_event_info_t *evt, void *arg)
{
    count++;
    FTB_event_handle_t ehandle;
    FTB_client_handle_t *handle = (FTB_client_handle_t *)arg;
    FTB_Publish(*handle, "PINGPONG_EVENT_SRV", NULL, &ehandle);
    return 0;
}

int pingpong_client(FTB_catch_event_info_t *evt, void *arg)
{
    FTB_event_handle_t ehandle;
    count++;
    if (count >= iter) {
        gettimeofday(&end,NULL);
        done = 1;
        return 0;
    }
    FTB_client_handle_t *handle = (FTB_client_handle_t *)arg;
    FTB_Publish(*handle, "PINGPONG_EVENT_CLI", NULL, &ehandle);
    return 0;
}

int main (int argc, char *argv[])
{
    FTB_client_handle_t handle;
    FTB_event_mask_t mask;
    FTB_client_t cinfo;
    FTB_subscribe_handle_t shandle;
    FTB_event_handle_t ehandle;
    double event_latency;
    int ret = 0;

    if (argc >=2 ) {
        is_server = 0;
        iter = atoi(argv[1]);
    }
    else {
        is_server = 1;
    }

    /*
    properties.catching_type = FTB_SUBSCRIPTION_NOTIFY;
    properties.err_handling = FTB_ERR_HANDLE_NONE;
    */

    strcpy(cinfo.event_space, "FTB.FTB_EXAMPLES.Pingpong");
    strcpy(cinfo.client_schema_ver, "0.5"); 
    strcpy(cinfo.client_name, "");
    strcpy(cinfo.client_jobid,"");
    strcpy(cinfo.client_subscription_style,"FTB_SUBSCRIPTION_NOTIFY");
    ret = FTB_Connect(&cinfo, &handle);
    if (ret != FTB_SUCCESS) {
        printf("FTB_Connect is not successful ret=%d\n", ret);
        exit(-1);
    }
    
    //FTB_Register_publishable_events(handle, ftb_ftb_examples_pingpong_events, FTB_FTB_EXAMPLES_PINGPONG_TOTAL_EVENTS);

    /* Create a subscription mask */
    ret = FTB_Create_mask(&mask, "all", "init");
    if (ret != FTB_SUCCESS) {
        printf("Mask creation failed for field_name=all and field value=init \n");
        exit(-1);
    }
    
    if (is_server) {
        ret = FTB_Create_mask(&mask, "event_name", "PINGPONG_EVENT_CLI");
        if (ret != FTB_SUCCESS) {
            printf("Create mask failure - 1\n");
            exit(-1);
        }
        ret = FTB_Subscribe(handle, &mask, &shandle, pingpong_server, (void*)&handle);
        if (ret != FTB_SUCCESS) {
            printf("FTB_Subscribe failed!\n"); 
            exit(-1);
        }
        signal(SIGINT, Sig_handler);
        signal(SIGTERM, Sig_handler);
    }
    else {
        ret = FTB_Create_mask(&mask, "event_name", "PINGPONG_EVENT_SRV");
        if (ret != FTB_SUCCESS) {
            printf("Create mask failure - 2\n");
            exit(-1);
        }
        ret = FTB_Subscribe(handle, &mask, &shandle, pingpong_client, (void*)&handle);
        if (ret != FTB_SUCCESS) {
            printf("FTB_Subscribe failed!\n"); 
            exit(-1);
        }
        gettimeofday(&begin,NULL);
        FTB_Publish(handle, "PINGPONG_EVENT_CLI",NULL, &ehandle);
    }

    while(!done) {
        sleep(1);
    }

    if (!is_server) {
        event_latency = (end.tv_sec-begin.tv_sec)*1000000 + (end.tv_usec-begin.tv_usec);
        event_latency/=iter;
        printf("Latency: %.3f\n",event_latency);
    }

    FTB_Disconnect(handle);
    
    return 0;
}

