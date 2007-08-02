#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <netdb.h>
#include <netinet/tcp.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <errno.h>

#include "ftb.h"
#include "ftb_agent.h"
#include "ftb_util.h"
#include "ftb_core.h"
#include "ftb_network.h"

/*Need to change to runtime later on*/
#define LOG_FILE_PREFIX   "/bgl/home1/qgao/ftb_bgl/zoid_output"

FILE *zoid_log_fp = NULL;

#define LOG_TO_FILE(args...)  do {\
    fprintf(zoid_log_fp, "[%s: line %d]", __FILE__, __LINE__); \
    fprintf(zoid_log_fp, args); \
    fprintf(zoid_log_fp, "\n"); \
}while(0)

static int nclient;
static FTB_component_properties_t *client_properties;
static pthread_t loop_thread;

void *progress_loop(void* arg)
{
    while (1) {
        FTBC_Progress();
    }
    return NULL;
}

void ftb_agent_init(int count)
{
    char log_file_name[128];
    int i;
    FTB_component_id_t component_id;
    FTB_node_properites_t properties;

    FTB_Bootstrap_local_config();
    FTB_Bootstrap_get_component_id(&component_id);

    sprintf(zoid_log_fp,"%s.%u", LOG_FILE_PREFIX, component_id);
    
    zoid_log_fp = fopen(LOG_FILE,"w");
    if (zoid_log_fp == NULL) {
       return;
    }
    LOG_TO_FILE("ZOID Plug-in Init:%d clients totally",count);
    nclient = count;
    client_properties = (FTB_component_properties_t*)malloc(sizeof(FTB_component_properties_t)*nclient);

    for (i=0;i<nclient;i++)  {
        client_properties[i].id = FTB_INVALID_COMPONENT_ID;
    }
    
    properties.id = component_id;
    properties.node_type = FTB_NODE_TYPE_LEAF;
    properties.multi_threading = 1;

    FTBC_Init(&properties);

    if (pthread_create(&loop_thread, NULL, progress_loop, NULL))
    {
        return FTB_ERR_GENERAL;
    }
    
}

void ftb_agent_fini()
{
    int i;
    LOG_TO_FILE("ZOID Plug-in Finalize");

    pthread_cancel(loop_thread);
    pthread_join(loop_thread,NULL);
    
    FTBC_Finalize();
    free(client_properties);
    fclose(zoid_log_fp);
}

int Agent_FTB_Init(const void *properties_in /* in:arr:size=+1 */,
                   size_t len /* in:obj */)
{
    FTB_component_properties_t *properties;
    int proc_id = __zoid_calling_process_id();

    properties = (FTB_component_properties_t *)properties_in;
    LOG_TO_FILE("%d:Agent_FTB_Init",proc_id);

    if (properties->com_namespace == FTB_INVALID_NAMESPACE
     || properties->id == FTB_INVALID_COMPONENT_ID) {
        LOG_TO_FILE("namespace or id cannot be 0");
        return FTB_ERR_INVALID_PARAMETER;
    }
    memcpy(&(client_properties[proc_id]), properties, sizeof(FTB_component_properties_t));

    return FTBC_Component_reg(properties);
}

int Agent_FTB_Finalize()
{
    int proc_id = __zoid_calling_process_id();
    FTB_component_id_t com_id = client_properties[proc_id].id;
    
    LOG_TO_FILE("%d:Agent_FTB_Reg_Finalize",proc_id);
    if (com_id == FTB_INVALID_COMPONENT_ID) {
        LOG_TO_FILE("not initialized");
        return FTB_ERR_GENERAL;
    }
    client_properties[proc_id].id = FTB_INVALID_COMPONENT_ID;

    return FTBC_Component_dereg(com_id);
}

int Agent_FTB_Reg_throw(const void *event_in /* in:arr:size=+1 */,
                        size_t len /* in:obj */)
{
    int proc_id = __zoid_calling_process_id();
    FTB_component_id_t com_id = client_properties[proc_id].id;
    FTB_event_t *event = (FTB_event_t *)event_in;
    LOG_TO_FILE("%d:Agent_FTB_Reg_throw",proc_id);

    if (com_id == FTB_INVALID_COMPONENT_ID) {
        LOG_TO_FILE("not initialized");
        return FTB_ERR_GENERAL;
    }

    return FTBC_Reg_throw(com_id, event);
}

int Agent_FTB_Reg_catch_polling(const void *event_mask /* in:arr:size=+1 */,
                                size_t len /* in:obj */)
{
    int proc_id = __zoid_calling_process_id();
    FTB_component_id_t com_id = client_properties[proc_id].id;
    FTB_event_mask_t *mask = (FTB_event_mask_t *)event_mask;
    LOG_TO_FILE("%d:Agent_FTB_Reg_catch_polling",proc_id);
    
    if (com_id == FTB_INVALID_COMPONENT_ID) {
        LOG_TO_FILE("not initialized");
        return FTB_ERR_GENERAL;
    }

    return FTBC_Reg_catch_cancel_polling(com_id, mask);
}

int Agent_FTB_Throw(const void *event_in /* in:arr:size=+1 */,
                    size_t len /* in:obj */,
                    const void *data /* in:arr:size=+1 */,
                    size_t data_len /* in:obj */)
{
    int proc_id = __zoid_calling_process_id();
    FTB_component_id_t com_id = client_properties[proc_id].id;
    FTB_event_t *event = (FTB_event_t *)event_in;
    FTB_event_inst_t event_inst;
    
    LOG_TO_FILE("%d:Agent_FTB_Throw",proc_id);
    if (com_id == FTB_INVALID_COMPONENT_ID) {
        LOG_TO_FILE("not initialized");
        return FTB_ERR_GENERAL;
    }
    event_inst.event_id = event->event_id;
    strncpy(event_inst.name, event->name, FTB_MAX_EVENT_NAME);
    event_inst.severity = event->severity;
    event_inst.src_id = com_id;
    event_inst.src_namespace = client_properties[proc_id].com_namespace;
    
    return FTBC_Throw(com_id, &event_inst);
}

int Agent_FTB_Catch(void *event_inst_out /* out:arr:size=+1 */,
                    size_t len /* in:obj */)
{
    int proc_id = __zoid_calling_process_id();
    FTB_component_id_t com_id = client_properties[proc_id].id;
    FTB_event_inst_t *event_inst = (FTB_event_inst_t *)event_inst_out;
    LOG_TO_FILE("%d:Agent_FTB_CATCH",proc_id);
    
    return FTBC_Catch(com_id, event_inst);
}

