#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/time.h>
#include <string.h>
#include "libftb.h"
#include "ftb_ftb_examples_pingpong_publishevents.h"

static volatile int done = 0;
static struct timeval begin, end;
static int is_server = 0;
static int count = 0; 
static int iter = 0;
char err_msg[FTB_MAX_ERRMSG_LEN];

void Sig_handler(int sig){
    if (sig == SIGINT || sig == SIGTERM)
        done = 1;
}

int pingpong_server(FTB_catch_event_info_t *evt, void *arg)
{
    count++;
    FTB_client_handle_t *handle = (FTB_client_handle_t *)arg;
    FTB_Publish_event(*handle, "PINGPONG_EVENT_SRV", NULL, err_msg);
    return 0;
}

int pingpong_client(FTB_catch_event_info_t *evt, void *arg)
{
    count++;
    if (count >= iter) {
        gettimeofday(&end,NULL);
        done = 1;
        return 0;
    }
    FTB_client_handle_t *handle = (FTB_client_handle_t *)arg;
    FTB_Publish_event(*handle, "PINGPONG_EVENT_CLI", NULL, err_msg);
    return 0;
}

int main (int argc, char *argv[])
{
    FTB_client_handle_t handle;
    FTB_event_mask_t mask;
    FTB_comp_info_t cinfo;
    FTB_subscribe_handle_t shandle;
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
    properties.catching_type = FTB_EVENT_CATCHING_NOTIFICATION;
    properties.err_handling = FTB_ERR_HANDLE_NONE;
    */

    strcpy(cinfo.comp_namespace, "FTB.FTB_EXAMPLES.Pingpong");
    strcpy(cinfo.schema_ver, "0.5"); 
    strcpy(cinfo.inst_name, "");
    strcpy(cinfo.jobid,"");
    ret = FTB_Init(&cinfo, &handle, err_msg);
    if (ret != FTB_SUCCESS) {
        printf("FTB_Init is not successful ret=%d\n", ret);
        exit(-1);
    }
    
    FTB_Register_publishable_events(handle, ftb_ftb_examples_pingpong_events, FTB_FTB_EXAMPLES_PINGPONG_TOTAL_EVENTS, err_msg);

    /* Create a subscription mask */
    ret = FTB_Create_mask(&mask, "all", "init", err_msg);
    if (ret != FTB_SUCCESS) {
        printf("Mask creation failed for field_name=all and field value=init \n");
        exit(-1);
    }
    
    if (is_server) {
        ret = FTB_Create_mask(&mask, "event_name", "PINGPONG_EVENT_CLI", err_msg);
        if (ret != FTB_SUCCESS) {
            printf("Create mask failure - 1\n");
            exit(-1);
        }
        ret = FTB_Subscribe(handle, &mask, &shandle, err_msg, pingpong_server, (void*)&handle);
        if (ret != FTB_SUCCESS) {
            printf("FTB_Subscribe failed!\n"); 
            exit(-1);
        }
        signal(SIGINT, Sig_handler);
        signal(SIGTERM, Sig_handler);
    }
    else {
        ret = FTB_Create_mask(&mask, "event_name", "PINGPONG_EVENT_SRV", err_msg);
        if (ret != FTB_SUCCESS) {
            printf("Create mask failure - 2\n");
            exit(-1);
        }
        ret = FTB_Subscribe(handle, &mask, &shandle, err_msg, pingpong_client, (void*)&handle);
        if (ret != FTB_SUCCESS) {
            printf("FTB_Subscribe failed!\n"); 
            exit(-1);
        }
        gettimeofday(&begin,NULL);
        FTB_Publish_event(handle, "PINGPONG_EVENT_CLI",NULL,err_msg);
    }

    while(!done) {
        sleep(1);
    }

    if (!is_server) {
        event_latency = (end.tv_sec-begin.tv_sec)*1000000 + (end.tv_usec-begin.tv_usec);
        event_latency/=iter;
        printf("Latency: %.3f\n",event_latency);
    }

    FTB_Finalize(handle);
    
    return 0;
}

