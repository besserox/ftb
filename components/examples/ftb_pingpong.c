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

int pingpong_server(FTB_receive_event_t *evt, void *arg)
{
    count++;
    FTB_event_handle_t ehandle;
    FTB_client_handle_t *handle = (FTB_client_handle_t *)arg;
    printf("In pingpong_server handler, called because an event was received\n");
    FTB_Publish(*handle, "PINGPONG_EVENT_SRV", NULL, &ehandle);
    return 0;
}

int pingpong_client(FTB_receive_event_t *evt, void *arg)
{
    FTB_event_handle_t ehandle;
    count++;
    if (count >= iter) {
        gettimeofday(&end,NULL);
        done = 1;
        return 0;
    }
    FTB_client_handle_t *handle = (FTB_client_handle_t *)arg;
    printf("In pingpong_client handler, called because an event was received\n");
    FTB_Publish(*handle, "PINGPONG_EVENT_CLI", NULL, &ehandle);
    return 0;
}

int main (int argc, char *argv[])
{
    FTB_client_handle_t handle;
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
    
    FTB_event_info_t event_info[2] = {{"PINGPONG_EVENT_SRV", "INFO"}, 
                                        {"PINGPONG_EVENT_CLI", "INFO"}};
    ret = FTB_Declare_publishable_events(handle, 0, event_info, 2); 
    if (ret != FTB_SUCCESS) {
        printf("FTB_Declare_publishable_events failed ret=%d!\n", ret); exit(-1);
    }

    if (is_server) {

        ret = FTB_Subscribe(&shandle, handle, "event_name=PINGPONG_EVENT_CLI", pingpong_server, (void*)&handle);
        if (ret != FTB_SUCCESS) {
            printf("FTB_Subscribe failed!\n"); 
            exit(-1);
        }
        signal(SIGINT, Sig_handler);
        signal(SIGTERM, Sig_handler);
    }
    else {
        ret = FTB_Subscribe(&shandle, handle, "event_name=PINGPONG_EVENT_SRV", pingpong_client, (void*)&handle);
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

