#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/time.h>

#include "libftb.h"
#include "ftb_event_def.h"
#include "ftb_throw_events.h"

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

int pingpong_server(FTB_event_t *evt, FTB_id_t *src, void *arg)
{
    count++;
    FTB_client_handle_t *handle = (FTB_client_handle_t *)arg;
    FTB_Publish_event(*handle, "PINGPONG_EVENT_SRV", NULL, err_msg);
    return 0;
}

int pingpong_client(FTB_event_t *evt, FTB_id_t *src, void *arg)
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
    FTB_component_properties_t properties;
    FTB_client_handle_t handle;
    FTB_event_t mask;
    double event_latency;

    if (argc >=2 ) {
        is_server = 0;
        iter = atoi(argv[1]);
    }
    else {
        is_server = 1;
    }

    properties.catching_type = FTB_EVENT_CATCHING_NOTIFICATION;
    properties.err_handling = FTB_ERR_HANDLE_NONE;

    FTB_EVENT_SET_ALL(mask);
    FTB_Init(FTB_EXAMPLES, PINGPONG, &properties, &handle);

    if (is_server) {
        FTB_Reg_catch_notify_event(handle, "PINGPONG_EVENT_CLI", pingpong_server, (void*)&handle);
        signal(SIGINT, Sig_handler);
        signal(SIGTERM, Sig_handler);
    }
    else {
        FTB_Reg_catch_notify_event(handle, "PINGPONG_EVENT_SRV", pingpong_client, (void*)&handle);
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

