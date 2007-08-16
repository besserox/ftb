#include <stdio.h>
#include <errno.h>
#include <error.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <assert.h>
#include <pthread.h>
#include <string.h>
#include <ctype.h>

#include "ftb_def.h"
#include "ftb_event_def.h"
#include "ftb_util.h"
#include "ftb_manager_lib.h"

extern FILE* FTBU_log_file_fp;

typedef struct FTBC_event_inst_list {
    struct FTBU_list_node *next;
    struct FTBU_list_node *prev;
    FTB_event_t *event_inst;
}FTBC_event_inst_list_t;

typedef struct FTBC_callback_entry{
    FTB_event_t *mask;
    int (*callback)(FTB_event_t *, void*);
    void *arg;
}FTBC_callback_entry_t;

typedef FTBU_map_node_t FTBC_map_mask_2_callback_entry_t;

typedef struct FTBC_comp_info{
    FTB_client_handle_t client_handle;
    FTB_id_t *id;
    FTB_component_properties_t properties;
    FTBU_list_node_t* event_queue; /* a event_inst_list*/
    int event_queue_size;
    pthread_mutex_t lock;
    pthread_cond_t cond;
    pthread_t callback;
    FTBU_list_node_t* callback_event_queue; /* a event_inst_list*/
    FTBC_map_mask_2_callback_entry_t *callback_map;
}FTBC_comp_info_t;

static pthread_mutex_t FTBC_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_t callback_thread;
typedef FTBU_map_node_t FTBC_map_client_handle_2_comp_info_t;
static FTBC_map_client_handle_2_comp_info_t* FTBC_comp_info_map = NULL;
static int enable_callback = 0;
static int num_components = 0;

static inline void lock_client()
{
    pthread_mutex_lock(&FTBC_lock);
}

static inline void unlock_client()
{
    pthread_mutex_unlock(&FTBC_lock);
}

static inline void lock_component(FTBC_comp_info_t * comp_info)
{
    pthread_mutex_lock(&comp_info->lock);
}

static inline void unlock_component(FTBC_comp_info_t * comp_info)
{
    pthread_mutex_unlock(&comp_info->lock);
}

int FTBC_util_is_equal_event(const void *lhs_void, const void* rhs_void)
{
    FTB_event_t *lhs = (FTB_event_t*)lhs_void;
    FTB_event_t *rhs = (FTB_event_t*)rhs_void;
    return FTBU_is_equal_event(lhs, rhs);
}

#define LOOKUP_COMP_INFO(handle, comp_info)  do {\
    FTBU_map_iterator_t iter;\
    if (FTBC_comp_info_map == NULL) {\
        FTB_WARNING("Not initialized");\
        return FTB_ERR_GENERAL;\
    }\
    lock_client();\
    iter = FTBU_map_find(FTBC_comp_info_map, FTBU_MAP_UINT_KEY(handle));\
    if (iter == FTBU_map_end(FTBC_comp_info_map)) {\
        FTB_WARNING("Not registered");\
        unlock_client();\
        return FTB_ERR_INVALID_PARAMETER;\
    }\
    comp_info = (FTBC_comp_info_t *)FTBU_map_get_data(iter); \
    unlock_client();\
}while(0)


static void util_push_to_comp_polling_list(FTBC_comp_info_t *comp_info, FTBC_event_inst_list_t *entry)
{/*Assumes it holds the lock of that comp*/
    FTBC_event_inst_list_t *temp;
    if (comp_info->properties.catching_type & FTB_EVENT_CATCHING_POLLING)  {
        if (comp_info->event_queue_size == comp_info->properties.max_event_queue_size) {
            /*Polling event queue full*/
            temp = (FTBC_event_inst_list_t *)comp_info->event_queue->next;
            FTBU_list_remove_entry((FTBU_list_node_t *)temp);
            free(temp->event_inst);
            free(temp);
            comp_info->event_queue_size --;
        }
        FTBU_list_add_back(comp_info->event_queue, (FTBU_list_node_t *)entry);
        comp_info->event_queue_size ++;
    }
    else {
        free(entry->event_inst);
        free(entry);
    }
}

static void util_handle_FTBM_msg(FTBM_msg_t* msg)
{
    FTB_client_handle_t handle;
    FTBC_comp_info_t *comp_info;
    FTBU_map_iterator_t iter;
    FTBC_event_inst_list_t *entry;

    if (msg->msg_type != FTBM_MSG_TYPE_NOTIFY) {
        FTB_WARNING("unexpected message type %d",msg->msg_type);
        return;
    }
    handle = FTB_CLIENT_ID_TO_HANDLE(msg->dst.client_id);
    lock_client();
    iter = FTBU_map_find(FTBC_comp_info_map, FTBU_MAP_UINT_KEY(handle));
    if (iter == FTBU_map_end(FTBC_comp_info_map)) {
        FTB_WARNING("Message for unregistered component");
        unlock_client();
        return;
    }
    comp_info = (FTBC_comp_info_t *)FTBU_map_get_data(iter); 
    unlock_client();
    entry = (FTBC_event_inst_list_t *)malloc(sizeof(FTBC_event_inst_list_t));
    entry->event_inst = (FTB_event_t *)malloc(sizeof(FTB_event_t));
    memcpy(entry->event_inst, &msg->event, sizeof(FTB_event_t));
    lock_component(comp_info);
    if (comp_info->properties.catching_type & FTB_EVENT_CATCHING_NOTIFICATION) {
        /*has notification thread*/
        FTBU_list_add_back(comp_info->callback_event_queue, (FTBU_list_node_t *)entry);
        pthread_cond_signal(&comp_info->cond);
    }
    else {
        util_push_to_comp_polling_list(comp_info, entry);
    }
    unlock_component(comp_info);
}

static void *callback_thread_client(void *arg)
{
    FTBM_msg_t msg;
    FTB_location_id_t incoming_src;
    int ret;
    while (1) {
        ret = FTBM_Wait(&msg,&incoming_src);
        if (ret != FTB_SUCCESS) {
            FTB_WARNING("FTBM_Wait failed %d",ret);
            return NULL;
        }
        util_handle_FTBM_msg(&msg);
    }
    return NULL;
}

static void *callback_component(void *arg)
{
    FTBC_comp_info_t *comp_info = (FTBC_comp_info_t *)arg;
    FTBC_event_inst_list_t *entry;
    FTBU_map_iterator_t iter;
    FTBC_callback_entry_t *callback_entry;
    int callback_done;
    lock_component(comp_info);
    while (1) {
        while (comp_info->callback_event_queue->next == comp_info->callback_event_queue) {
            /*Callback event queue is empty*/
            pthread_cond_wait(&comp_info->cond, &comp_info->lock);
        }
        entry = (FTBC_event_inst_list_t *)comp_info->callback_event_queue->next;
        /*Try to match it with callback_map*/
        callback_done = 0;
        iter = FTBU_map_begin(comp_info->callback_map);
        while (iter != FTBU_map_end(comp_info->callback_map)) {
            callback_entry = (FTBC_callback_entry_t *)FTBU_map_get_data(iter);
            if (FTBU_match_mask(entry->event_inst, callback_entry->mask))  {
                /*Make callback*/
                (*callback_entry->callback)(entry->event_inst, callback_entry->arg);
                free(entry->event_inst);
                FTBU_list_remove_entry((FTBU_list_node_t *)entry);
                free(entry);
                callback_done = 1;
                break;
            }
            iter = FTBU_map_next_iterator(iter);
        }
        if (!callback_done) {
            /*Move to polling event queue*/
            FTBU_list_remove_entry((FTBU_list_node_t *)entry);
            util_push_to_comp_polling_list(comp_info,entry);
        }
    }
    unlock_component(comp_info);
    return NULL;
}

int FTBC_Init(FTB_comp_ctgy_t category, FTB_comp_t component, uint8_t extension, const FTB_component_properties_t *properties, FTB_client_handle_t *client_handle)
{
    FTBC_comp_info_t *comp_info;
    FTB_INFO("FTBC_Init In");
    comp_info = (FTBC_comp_info_t*)malloc(sizeof(FTBC_comp_info_t));
    if (properties != NULL) {
        memcpy((void*)&comp_info->properties, (void*)properties,sizeof(FTB_component_properties_t));
    }
    else {
        comp_info->properties.catching_type = FTB_EVENT_CATCHING_POLLING;
        comp_info->properties.err_handling = FTB_ERR_HANDLE_NONE;
        comp_info->properties.max_event_queue_size = FTB_DEFAULT_EVENT_POLLING_Q_LEN;
    }
    lock_client();
    if (num_components == 0) {
        FTBC_comp_info_map = FTBU_map_init(NULL); /*Since the key: client_handle is uint32_t*/
        FTBM_Init(1);
        num_components++;
    }
    unlock_client();
    comp_info->id = (FTB_id_t *)malloc(sizeof(FTB_id_t));
    comp_info->id->client_id.comp_ctgy = category;
    comp_info->id->client_id.comp = component;
    comp_info->id->client_id.ext = extension;
    FTBM_Get_location_id(&comp_info->id->location_id);
    comp_info->client_handle = FTB_CLIENT_ID_TO_HANDLE(comp_info->id->client_id);
    *client_handle = comp_info->client_handle;
    
    comp_info->event_queue_size = 0;
    if (comp_info->properties.catching_type & FTB_EVENT_CATCHING_POLLING) {
        comp_info->event_queue = (FTBU_list_node_t*)malloc(sizeof(FTBU_list_node_t));
        FTBU_list_init(comp_info->event_queue);
    }
    else {
        comp_info->event_queue = NULL;
    }

    pthread_mutex_init(&comp_info->lock, NULL);
    if (comp_info->properties.catching_type & FTB_EVENT_CATCHING_NOTIFICATION) {
        comp_info->callback_map = FTBU_map_init(FTBC_util_is_equal_event);
        comp_info->callback_event_queue = (FTBU_list_node_t*)malloc(sizeof(FTBU_list_node_t));
        FTBU_list_init(comp_info->callback_event_queue);
        /*TODO enable callback in client*/
        lock_client();
        if (enable_callback == 0) {
             pthread_create(&callback_thread,NULL, callback_thread_client, NULL);
             enable_callback++;
        }
        unlock_client();
        /*Create component level thread*/
        {
            pthread_attr_t attr;
            pthread_attr_init(&attr);
            pthread_attr_setstacksize(&attr,1024*1024);
            pthread_cond_init(&comp_info->cond, NULL);
            pthread_create(&comp_info->callback, &attr, callback_component, (void*)comp_info);
        }
    }
    else {
        comp_info->callback_map = NULL;
    }

    lock_client();
    if (FTBU_map_insert(FTBC_comp_info_map,FTBU_MAP_UINT_KEY(comp_info->client_handle), (void *)comp_info)==FTBU_EXIST) {
        FTB_WARNING("This component has already been registered");
        FTB_INFO("FTBC_Init Out");
        return FTB_ERR_INVALID_PARAMETER;
    }
    unlock_client();

    {
        int ret;
        FTBM_msg_t msg;
        memcpy(&msg.src,comp_info->id,sizeof(FTB_id_t));
        msg.msg_type = FTBM_MSG_TYPE_COMP_REG;
        FTBM_Get_parent_location_id(&msg.dst.location_id);
        FTB_INFO("parent: %s pid %d",msg.dst.location_id.hostname, msg.dst.location_id.pid);
        ret = FTBM_Send(&msg);
        if (ret != FTB_SUCCESS) {
            FTB_INFO("FTBC_Init Out");
            return ret;
        }
    }
    
    FTB_INFO("FTBC_Init Out");
    return FTB_SUCCESS;
}

static void util_finalize_component(FTBC_comp_info_t * comp_info)
{
    FTBM_Component_dereg(comp_info->id);
    
    free(comp_info->id);
    if (comp_info->event_queue) {
        FTBU_list_node_t *temp;
        FTBU_list_node_t *pos;
        FTBU_list_for_each(pos, comp_info->event_queue, temp){
            FTBC_event_inst_list_t *entry=(FTBC_event_inst_list_t*)pos;
            free(entry->event_inst);
            free(entry);
        }
        free(comp_info->event_queue);
    }

    if (comp_info->properties.catching_type & FTB_EVENT_CATCHING_NOTIFICATION) {
        FTBU_map_iterator_t iter;
        pthread_cancel(comp_info->callback);
        pthread_join(comp_info->callback, NULL);

        pthread_cond_destroy(&comp_info->cond);
        iter = FTBU_map_begin(comp_info->callback_map);
        while (iter!=FTBU_map_end(comp_info->callback_map)) {
            FTBC_callback_entry_t *entry = (FTBC_callback_entry_t*)FTBU_map_get_data(iter);
            free(entry->mask);
            iter = FTBU_map_next_iterator(iter);
        }
        FTBU_map_finalize(comp_info->callback_map);
    }
}

int FTBC_Finalize(FTB_client_handle_t handle)
{
    FTBC_comp_info_t *comp_info;
    LOOKUP_COMP_INFO(handle,comp_info);
    FTB_INFO("FTBC_Finalize In");
    
    {
        int ret;
        FTBM_msg_t msg;
        memcpy(&msg.src,comp_info->id,sizeof(FTB_id_t));
        msg.msg_type = FTBM_MSG_TYPE_COMP_DEREG;
        FTBM_Get_parent_location_id(&msg.dst.location_id);
        ret = FTBM_Send(&msg);
        if (ret != FTB_SUCCESS)
            return ret;
    }
    lock_component(comp_info);
    util_finalize_component(comp_info);
    unlock_component(comp_info);

    lock_client();
    pthread_mutex_destroy(&comp_info->lock);
    free(comp_info);
    FTBU_map_remove_key(FTBC_comp_info_map, FTBU_MAP_UINT_KEY(handle));
    if (comp_info->properties.catching_type & FTB_EVENT_CATCHING_NOTIFICATION) {
        enable_callback--;
        if (enable_callback == 0) {
            /*Last callback component finalized*/
            pthread_cancel(callback_thread);
            pthread_join(callback_thread, NULL);
        }
    }
    num_components--;
    if (num_components == 0) {
        FTBM_Finalize();
        FTBU_map_finalize(FTBC_comp_info_map);
        FTBC_comp_info_map = NULL;
    }
    unlock_client();
    
    FTB_INFO("FTBC_Finalize Out");
    return FTB_SUCCESS;
}

int FTBC_Abort(FTB_client_handle_t handle)
{  /*TODO: Abort function needs reconsideration*/
    FTBC_comp_info_t *comp_info;
    LOOKUP_COMP_INFO(handle,comp_info);
    FTB_INFO("FTBC_Abort In");
    {
        FTBM_msg_t msg;
        memcpy(&msg.src,comp_info->id,sizeof(FTB_id_t));
        msg.msg_type = FTBM_MSG_TYPE_COMP_DEREG;
        FTBM_Get_parent_location_id(&msg.dst.location_id);
        FTBM_Send(&msg);
    }

    util_finalize_component(comp_info);

    FTBU_map_remove_key(FTBC_comp_info_map, FTBU_MAP_UINT_KEY(handle));
    if (comp_info->properties.catching_type & FTB_EVENT_CATCHING_NOTIFICATION) {
        enable_callback--;
        if (enable_callback == 0) {
            /*Last callback component finalized*/
            pthread_cancel(callback_thread);
        }
    }

    num_components--;
    if (num_components == 0) {
        FTBM_Abort();
        FTBU_map_finalize(FTBC_comp_info_map);
        FTBC_comp_info_map = NULL;
    }
    
    FTB_INFO("FTBC_Abort Out");
    return FTB_SUCCESS;
}


int FTBC_Reg_throw(FTB_client_handle_t handle, FTB_event_name_t event_name)
{
    FTBM_msg_t msg;
    FTBC_comp_info_t *comp_info;
    int ret;
    LOOKUP_COMP_INFO(handle,comp_info);
    FTB_INFO("FTBC_Reg_throw In");

    FTBM_get_event_by_name(event_name, &msg.event);

    memcpy(&msg.src,comp_info->id,sizeof(FTB_id_t));
    msg.msg_type = FTBM_MSG_TYPE_REG_THROW;
    FTBM_Get_parent_location_id(&msg.dst.location_id);

    ret = FTBM_Send(&msg);
    FTB_INFO("FTBC_Reg_throw Out");
    return ret;
}

int FTBC_Reg_catch_polling_event(FTB_client_handle_t handle, FTB_event_name_t event_name)
{
    FTBM_msg_t msg;
    FTBC_comp_info_t *comp_info;
    int ret;
    LOOKUP_COMP_INFO(handle,comp_info);

    FTB_INFO("FTBC_Reg_catch_polling_event In");
    if (!(comp_info->properties.catching_type & FTB_EVENT_CATCHING_POLLING)){
        FTB_INFO("FTBC_Reg_catch_polling_event Out");
        return FTB_ERR_NOT_SUPPORTED;
    }

    FTBM_get_event_by_name(event_name, &msg.event);
    memcpy(&msg.src,comp_info->id,sizeof(FTB_id_t));
    msg.msg_type = FTBM_MSG_TYPE_REG_CATCH;
    FTBM_Get_parent_location_id(&msg.dst.location_id);

    ret = FTBM_Send(&msg);
    FTB_INFO("FTBC_Reg_catch_polling_event Out");
    return ret;
}

int FTBC_Reg_catch_polling_mask(FTB_client_handle_t handle, const FTB_event_t *event)
{
    FTBM_msg_t msg;
    FTBC_comp_info_t *comp_info;
    int ret;
    LOOKUP_COMP_INFO(handle,comp_info);

    FTB_INFO("FTBC_Reg_catch_polling_mask In");
    if (!(comp_info->properties.catching_type & FTB_EVENT_CATCHING_POLLING)){
        FTB_INFO("FTBC_Reg_catch_polling_mask Out");
        return FTB_ERR_NOT_SUPPORTED;
    }

    memcpy(&msg.event, event, sizeof(FTB_event_t));
    memcpy(&msg.src,comp_info->id,sizeof(FTB_id_t));
    msg.msg_type = FTBM_MSG_TYPE_REG_CATCH;
    FTBM_Get_parent_location_id(&msg.dst.location_id);

    ret = FTBM_Send(&msg);
    FTB_INFO("FTBC_Reg_catch_polling_mask Out");
    return ret;
}

static void util_add_to_callback_map(FTBC_comp_info_t *comp_info, const FTB_event_t *event, int (*callback)(FTB_event_t *, void*), void *arg)
{
    FTBC_callback_entry_t *entry = (FTBC_callback_entry_t *)malloc(sizeof(FTBC_callback_entry_t));
    entry->mask = (FTB_event_t *)malloc(sizeof(FTB_event_t));
    memcpy(entry->mask, event, sizeof(FTB_event_t));
    entry->callback = callback;
    entry->arg = arg;
    lock_component(comp_info);
    FTBU_map_insert(comp_info->callback_map, FTBU_MAP_PTR_KEY(entry->mask), (void *)entry);
    unlock_component(comp_info);
}

int FTBC_Reg_catch_notify_event(FTB_client_handle_t handle, FTB_event_name_t event_name, int (*callback)(FTB_event_t *, void*), void *arg)
{
    FTBM_msg_t msg;
    FTBC_comp_info_t *comp_info;
    int ret;
    LOOKUP_COMP_INFO(handle,comp_info);

    FTB_INFO("FTBC_Reg_catch_notify_event In");
    if (!(comp_info->properties.catching_type & FTB_EVENT_CATCHING_NOTIFICATION)){
        FTB_INFO("FTBC_Reg_catch_notify_event Out");
        return FTB_ERR_NOT_SUPPORTED;
    }
    
    FTBM_get_event_by_name(event_name, &msg.event);

    util_add_to_callback_map(comp_info, &msg.event, callback, arg);

    memcpy(&msg.src,comp_info->id,sizeof(FTB_id_t));
    msg.msg_type = FTBM_MSG_TYPE_REG_CATCH;
    FTBM_Get_parent_location_id(&msg.dst.location_id);

    ret = FTBM_Send(&msg);
    FTB_INFO("FTBC_Reg_catch_notify_event Out");
    return ret;
}

int FTBC_Reg_catch_notify_mask(FTB_client_handle_t handle, const FTB_event_t *event, int (*callback)(FTB_event_t *, void*), void *arg)
{
    FTBM_msg_t msg;
    FTBC_comp_info_t *comp_info;
    int ret;
    LOOKUP_COMP_INFO(handle,comp_info);

    FTB_INFO("FTBC_Reg_catch_notify_mask In");
    if (!(comp_info->properties.catching_type & FTB_EVENT_CATCHING_NOTIFICATION)){
        FTB_INFO("FTBC_Reg_catch_notify_mask Out");
        return FTB_ERR_NOT_SUPPORTED;
    }

    util_add_to_callback_map(comp_info, event, callback, arg);

    memcpy(&msg.event, event, sizeof(FTB_event_t));
    memcpy(&msg.src,comp_info->id,sizeof(FTB_id_t));
    msg.msg_type = FTBM_MSG_TYPE_REG_CATCH;
    FTBM_Get_parent_location_id(&msg.dst.location_id);

    ret = FTBM_Send(&msg);
    FTB_INFO("FTBC_Reg_catch_notify_mask Out");
    return ret;
}

int FTBC_Throw(FTB_client_handle_t handle, FTB_event_name_t event_name)
{
    FTBM_msg_t msg;
    FTBC_comp_info_t *comp_info;
    int ret;
    LOOKUP_COMP_INFO(handle,comp_info);

    FTB_INFO("FTBC_Throw In");
    FTBM_get_event_by_name(event_name, &msg.event);

    memcpy(&msg.src,comp_info->id,sizeof(FTB_id_t));
    msg.msg_type = FTBM_MSG_TYPE_NOTIFY;
    FTBM_Get_parent_location_id(&msg.dst.location_id);

    ret = FTBM_Send(&msg);
    FTB_INFO("FTBC_Throw In");
    return ret;
}

int FTBC_Catch(FTB_client_handle_t handle, FTB_event_t *event)
{
    FTBM_msg_t msg;
    FTB_location_id_t incoming_src;
    FTBC_comp_info_t *comp_info;
    FTBC_event_inst_list_t *entry;
    FTB_INFO("FTBC_Catch In");
    LOOKUP_COMP_INFO(handle,comp_info);

    if (!(comp_info->properties.catching_type & FTB_EVENT_CATCHING_POLLING)){
        FTB_INFO("FTBC_Catch Out");
        return FTB_ERR_NOT_SUPPORTED;
    }
    
    lock_component(comp_info);
    if (comp_info->event_queue_size > 0) {
        entry = (FTBC_event_inst_list_t *)comp_info->callback_event_queue->next;
        memcpy(event, entry->event_inst, sizeof(FTB_event_t));
        free(entry->event_inst);
        FTBU_list_remove_entry((FTBU_list_node_t *)entry);
        free(entry);
        comp_info->event_queue_size--;
        unlock_component(comp_info);
        FTB_INFO("FTBC_Catch Out");
        return FTB_CAUGHT_EVENT;
    }

    /*Then poll FTBM once*/
    while (FTBM_Poll(&msg,&incoming_src) == FTB_SUCCESS) {
        if (FTB_CLIENT_ID_TO_HANDLE(msg.dst.client_id)==handle) {
            /*message to myself*/
            int is_for_callback = 0;
            if (msg.msg_type != FTBM_MSG_TYPE_NOTIFY) {
                FTB_WARNING("unexpected message type %d",msg.msg_type);
                continue;
            }
            if (comp_info->properties.catching_type & FTB_EVENT_CATCHING_NOTIFICATION) {
                /*Test whether belonging to any callback mask*/
                FTBU_map_iterator_t iter;
                iter = FTBU_map_begin(comp_info->callback_map);
                while (iter != FTBU_map_end(comp_info->callback_map)) {
                    FTBC_callback_entry_t *callback_entry = (FTBC_callback_entry_t *)FTBU_map_get_data(iter);
                    if (FTBU_match_mask(&msg.event, callback_entry->mask))  {
                        entry = (FTBC_event_inst_list_t *)malloc(sizeof(FTBC_event_inst_list_t));
                        entry->event_inst = (FTB_event_t *)malloc(sizeof(FTB_event_t));
                        memcpy(entry->event_inst, &msg.event, sizeof(FTB_event_t));
                        FTBU_list_add_back(comp_info->callback_event_queue, (FTBU_list_node_t *)entry);
                        is_for_callback = 1;
                        break;
                    }
                    iter = FTBU_map_next_iterator(iter);
                }
            }
            if (!is_for_callback) {
                memcpy(event, &msg.event,sizeof(FTB_event_t));
                unlock_component(comp_info);
                FTB_INFO("FTBC_Catch Out");
                return FTB_CAUGHT_EVENT;
            }
        }
        else {
            /*message to others*/
            unlock_component(comp_info);
            util_handle_FTBM_msg(&msg);
            lock_component(comp_info);
        }
    }
    unlock_component(comp_info);

    FTB_INFO("FTBC_Catch Out");
    return FTB_CAUGHT_NO_EVENT;
}

