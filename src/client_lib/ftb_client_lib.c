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
    FTB_event_t event_inst;
    FTB_id_t src;
}FTBC_event_inst_list_t;

typedef struct FTBC_callback_entry{
    FTB_event_t *mask;
    int (*callback)(FTB_catch_event_info_t *, void*);
    void *arg;
}FTBC_callback_entry_t;

typedef struct FTBC_tag_entry {
    FTB_tag_t tag;
    FTB_client_handle_t owner;
    char data[FTB_MAX_DYNAMIC_DATA_SIZE];
    FTB_tag_len_t data_len;
}FTBC_tag_entry_t;

typedef FTBU_map_node_t FTBC_map_mask_2_callback_entry_t;

typedef struct FTBC_comp_info{
    FTB_client_handle_t client_handle;
    FTB_id_t *id;
    FTB_component_properties_t properties;
    FTBU_list_node_t* event_queue; /* entry type: event_inst_list*/
    int event_queue_size;
    pthread_mutex_t lock;
    pthread_cond_t cond;
    pthread_t callback;
    volatile int finalizing;
    FTBU_list_node_t* callback_event_queue; /* entry type: event_inst_list*/
    FTBC_map_mask_2_callback_entry_t *callback_map;
    FTB_region_t comp_region; //This assumes a component can throw only in one region
    FTB_jobid_t comp_jobid;
    FTB_inst_name_t comp_inst_name;
    int seqnum;
}FTBC_comp_info_t;

static pthread_mutex_t FTBC_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_t callback_thread;
typedef FTBU_map_node_t FTBC_map_client_handle_2_comp_info_t;
static FTBC_map_client_handle_2_comp_info_t* FTBC_comp_info_map = NULL;
typedef FTBU_map_node_t FTBC_map_tag_2_tag_entry_t;
static FTBC_map_tag_2_tag_entry_t *FTBC_tag_map;
static char tag_string[FTB_MAX_DYNAMIC_DATA_SIZE];
static int tag_size = 1;
static uint8_t tag_count = 0;
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

int FTBC_util_is_equal_tag(const void *lhs_void, const void* rhs_void)
{
    FTB_tag_t *lhs = (FTB_tag_t *)lhs_void;
    FTB_tag_t *rhs = (FTB_tag_t *)rhs_void;
    return (*lhs == *rhs);
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

static int util_split_namespace(FTB_namespace_t ns, char *region_name, char *category_name, char *component_name)
{
    char *tempstr=(char*)malloc(sizeof(FTB_namespace_t)+1);
    char *str=(char*)malloc(sizeof(FTB_namespace_t)+1);
    int i=0; 
    for (i=0; i<=strlen(ns); i++) tempstr[i]=toupper(ns[i]);
    str=strsep(&tempstr,".");
    strcpy(region_name,str);
    str=strsep(&tempstr,".");
    strcpy(category_name,str);
    str=strsep(&tempstr,".");
    strcpy(component_name,str);
    if ((strcmp(region_name,"\0")==0) || (strcmp(category_name,"\0")==0) || (strcmp(component_name,"\0")==0) 
            || (tempstr != NULL) 
            || (strlen(region_name)> (FTB_MAX_REGION-1)) 
            || (strlen(category_name) > (FTB_MAX_COMP_CAT-1)) 
            || (strlen(component_name) > (FTB_MAX_COMP-1))
            || (strcasecmp(region_name, "FTB")!=0)) {
        return FTB_ERR_NAMESPACE_FORMAT;
    }
    return FTB_SUCCESS;
}

static void util_push_to_comp_polling_list(FTBC_comp_info_t *comp_info, FTBC_event_inst_list_t *entry)
{/*Assumes it holds the lock of that comp*/
    FTBC_event_inst_list_t *temp;
    if (comp_info->properties.catching_type & FTB_EVENT_CATCHING_POLLING)  {
        if (comp_info->event_queue_size == comp_info->properties.max_event_queue_size) {
            FTB_WARNING("Event queue is full");
            temp = (FTBC_event_inst_list_t *)comp_info->event_queue->next;
            FTBU_list_remove_entry((FTBU_list_node_t *)temp);
            free(temp);
            comp_info->event_queue_size--;
        }
        FTBU_list_add_back(comp_info->event_queue, (FTBU_list_node_t *)entry);
        comp_info->event_queue_size++;
    }
    else {
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
    memcpy(&entry->event_inst, &msg->event, sizeof(FTB_event_t));
    memcpy(&entry->src, &msg->src, sizeof(FTB_id_t));
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

    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
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
            if (comp_info->finalizing) {
                FTB_INFO("Finalizing");
                unlock_component(comp_info);
                return NULL;
            }
        }
        entry = (FTBC_event_inst_list_t *)comp_info->callback_event_queue->next;
        /*Try to match it with callback_map*/
        callback_done = 0;
        iter = FTBU_map_begin(comp_info->callback_map);
        while (iter != FTBU_map_end(comp_info->callback_map)) {
            callback_entry = (FTBC_callback_entry_t *)FTBU_map_get_data(iter);
            if (FTBU_match_mask(&entry->event_inst, callback_entry->mask))  {
                /*Make callback*/
                //(*callback_entry->callback)(&entry->event_inst, &entry->src, callback_entry->arg);
                FTB_catch_event_info_t ce_event;
                char name[128];
                int ret = 0;
                ret = FTBM_get_string_by_id(entry->event_inst.comp_cat, name, "comp_cat");
                if (ret != FTB_SUCCESS) {
                    FTB_WARNING("Incoming event matches mask but is from unknown component category\n");
                }
                else 
                    strcpy(ce_event.comp_cat, name);
                
                ret = FTBM_get_string_by_id(entry->event_inst.comp, name, "comp");
                if (ret != FTB_SUCCESS) {
                    FTB_WARNING("Incoming event matches mask but is from unknown component\n");
                }
                else 
                    strcpy(ce_event.comp, name);
                
                ret = FTBM_get_string_by_id(entry->event_inst.severity, name, "severity");
                if (ret != FTB_SUCCESS) {
                    FTB_WARNING("Incoming event matches mask but is of unknown severity\n");
                }
                else 
                    strcpy(ce_event.severity, name);
                
                ret = FTBM_get_string_by_id(entry->event_inst.event_name, name, "event_name");
                if (ret != FTB_SUCCESS) {
                    FTB_WARNING("Incoming event matches mask but the event name is unknown to FTB\n");
                }
                    strcpy(ce_event.event_name, name);
                strcpy(ce_event.region, entry->event_inst.region);
                strcpy(ce_event.jobid, entry->event_inst.jobid);
                strcpy(ce_event.inst_name, entry->event_inst.inst_name);
                strcpy(ce_event.hostname, entry->event_inst.hostname);
                memcpy(&ce_event.seqnum, &entry->event_inst.seqnum, sizeof(int));
                memcpy(&ce_event.len, &entry->event_inst.len, sizeof(FTB_tag_len_t));
                memcpy(&ce_event.event_data, &entry->event_inst.event_data, sizeof(FTB_event_data_t));
                memcpy(&ce_event.incoming_src, &entry->src.location_id, sizeof(FTB_location_id_t));
                memcpy(ce_event.dynamic_data, entry->event_inst.dynamic_data,  FTB_MAX_DYNAMIC_DATA_SIZE);
                
                (*callback_entry->callback)(&ce_event, callback_entry->arg);
                //(*callback_entry->callback)((FTB_catch_event_info_t *)&entry->event_inst, callback_entry->arg);
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

int FTBC_Init(FTB_comp_info_t *cinfo, uint8_t extension, const FTB_component_properties_t *properties, FTB_client_handle_t *client_handle)
{
    FTBC_comp_info_t *comp_info;

    FTB_comp_cat_code_t category;
    FTB_comp_code_t component;
    char category_name[FTB_MAX_COMP_CAT+1];
    char component_name[FTB_MAX_COMP+1];
    char region_name[FTB_MAX_REGION+1];
    
    if (util_split_namespace(cinfo->comp_namespace, region_name, category_name, component_name)) {
        FTB_WARNING("Invalid namespace format");
        FTB_INFO("FTBC_Init Out");
        return FTB_ERR_NAMESPACE_FORMAT;
    }
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
        FTBC_tag_map = FTBU_map_init(FTBC_util_is_equal_tag);
        memset(tag_string,0,FTB_MAX_DYNAMIC_DATA_SIZE);
        FTBM_hash_init();
        FTBM_compcat_table_init();
        FTBM_comp_table_init();
        FTBM_event_table_init();
        FTBM_severity_table_init();
        FTBM_Init(1);
    }
    num_components++;
    unlock_client();
    if (FTBM_get_compcat_by_name(category_name, &category) == FTB_ERR_HASHKEY_NOT_FOUND)
        return FTB_ERR_NAMESPACE_FORMAT;
    if (FTBM_get_comp_by_name(component_name, &component)  == FTB_ERR_HASHKEY_NOT_FOUND)
            return FTB_ERR_NAMESPACE_FORMAT;
    comp_info->id = (FTB_id_t *)malloc(sizeof(FTB_id_t));
    comp_info->id->client_id.comp_cat = category;
    comp_info->id->client_id.comp = component;
    comp_info->id->client_id.ext = extension;
    FTBM_Get_location_id(&comp_info->id->location_id);
    comp_info->client_handle = FTB_CLIENT_ID_TO_HANDLE(comp_info->id->client_id);
    *client_handle = comp_info->client_handle;
    
    comp_info->finalizing = 0;
    comp_info->event_queue_size = 0;
    comp_info->seqnum = 0;
    strcpy(comp_info->comp_region, region_name);
    if (strlen(cinfo->inst_name) > (FTB_MAX_INST_NAME-1)) 
        return FTB_ERR_INVALID_VALUE;
    else
        strcpy(comp_info->comp_inst_name, cinfo->inst_name);
    if (strlen(cinfo->jobid) > (FTB_MAX_JOBID-1)) 
        return FTB_ERR_INVALID_VALUE;
    else
    strcpy(comp_info->comp_jobid, cinfo->jobid);
    if (comp_info->properties.catching_type & FTB_EVENT_CATCHING_POLLING) {
        if (comp_info->properties.max_event_queue_size == 0) {
            FTB_WARNING("Polling queue size equal to 0, changing to default value");
            comp_info->properties.max_event_queue_size = FTB_DEFAULT_EVENT_POLLING_Q_LEN;
        }
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

int FTBC_Create_mask(FTB_event_mask_t * event_mask, char *field_name, char *field_val)
{
    if (strlen(field_val) <= 0) 
        return FTB_ERR_INVALID_VALUE;

    if (event_mask->initialized != 1) {
        if ((strcasecmp(field_name, "all") != 0) && (strcasecmp(field_val, "init") !=0 ))
                return FTB_ERR_MASK_NOT_INITIALIZED;
    }
    if ((strcasecmp(field_name, "all") == 0) && (strcasecmp(field_val, "init") == 0 )) {
                event_mask->initialized = 1;
                strcpy(event_mask->event.region, "ALL");
                strcpy(event_mask->event.hostname, "ALL");
                strcpy(event_mask->event.inst_name, "ALL");
                strcpy(event_mask->event.jobid, "ALL");
                event_mask->event.severity = 0-1;
                event_mask->event.comp_cat = 0-1; 
                event_mask->event.comp = 0-1; 
                event_mask->event.event_cat = 0-1; 
                event_mask->event.event_name = 0-1;
    }
    else if (strcasecmp(field_name, "hostname") == 0) {
        if (strlen(field_val) > (FTB_MAX_HOST_NAME-1)) 
            return FTB_ERR_INVALID_VALUE;
        strcpy(event_mask->event.hostname, field_val);
    }
    else if (strcasecmp(field_name, "jobid") == 0) {
        if (strlen(field_val) > (FTB_MAX_JOBID-1)) 
            return FTB_ERR_INVALID_VALUE; 
        strcpy(event_mask->event.jobid, field_val);
    }
    else if (strcasecmp(field_name, "inst_name") == 0) {
        if (strlen(field_val) > (FTB_MAX_INST_NAME-1)) 
            return FTB_ERR_INVALID_VALUE; 
        strcpy(event_mask->event.inst_name, field_val);
    }
    else if (strcasecmp(field_name, "namespace") == 0) {
        char region_name[FTB_MAX_REGION+1];
        char comp_name[FTB_MAX_COMP+1];
        char comp_cat[FTB_MAX_COMP_CAT+1];
        FTB_comp_cat_code_t comp_cat_code = 0;
        FTB_comp_code_t comp_code = 0;
        int ret = 0;
        
        if (sizeof(field_val) > (FTB_MAX_NAMESPACE-1)) 
            return FTB_ERR_INVALID_VALUE; 
        
        ret = util_split_namespace(field_val, region_name, comp_cat, comp_name);
        if (ret != FTB_SUCCESS) {
            return FTB_ERR_NAMESPACE_FORMAT;
        }
        if ((strlen(region_name) > (FTB_MAX_REGION-1)) || 
                (strlen(comp_name) > (FTB_MAX_COMP-1)) || 
                (strlen(comp_cat) > (FTB_MAX_COMP_CAT-1))) {
            return FTB_ERR_INVALID_VALUE; 
        }
        strcpy(event_mask->event.region, region_name);
       
        if (strcasecmp(comp_cat, "ALL") == 0) 
            comp_cat_code = 0-1;
        else {
            ret = FTBM_get_compcat_by_name(comp_cat, &comp_cat_code);
            if (ret == FTB_ERR_HASHKEY_NOT_FOUND) 
                return FTB_ERR_INVALID_VALUE;
        }
        
        if (strcasecmp(comp_name, "ALL") == 0) 
            comp_code = 0-1;
        else {
            ret = FTBM_get_comp_by_name(comp_name, &comp_code);
            if (ret == FTB_ERR_HASHKEY_NOT_FOUND) 
                return FTB_ERR_INVALID_VALUE;
        }
        
        event_mask->event.comp_cat = comp_cat_code;
        event_mask->event.comp = comp_code;
        event_mask->event.event_cat = 0-1;
        
    }
    else if (strcasecmp(field_name, "severity") == 0) {
        int ret = 0, i = 0;
        char tempstr[FTB_MAX_SEVERITY+1];
        FTB_severity_code_t severity_code = 0;
        
        if (sizeof(field_val) > (FTB_MAX_SEVERITY-1)) 
            return FTB_ERR_VALUE_NOT_FOUND; 
        
        for (i=0; i<=strlen(field_val); i++) 
            tempstr[i]=toupper(field_val[i]);
        ret = FTBM_get_severity_by_name(tempstr, &severity_code);
        if (ret == FTB_ERR_HASHKEY_NOT_FOUND) 
            return FTB_ERR_VALUE_NOT_FOUND;
        event_mask->event.severity=severity_code;
    }
    else if (strcasecmp(field_name, "event_name") == 0){
        FTB_event_t evt;
        int ret = 0, i = 0;
        char tempstr[FTB_MAX_EVENT_NAME+1];

        if (sizeof(field_val) > (FTB_MAX_EVENT_NAME-1)) 
            return FTB_ERR_INVALID_VALUE; 
        
        for (i=0; i<strlen(field_val)+1; i++) 
            tempstr[i]=toupper(field_val[i]);
        ret = FTBM_get_event_by_name(tempstr, &evt);
        if (ret == FTB_ERR_HASHKEY_NOT_FOUND) 
            return FTB_ERR_INVALID_VALUE;
        event_mask->event.event_name = evt.event_name;
        event_mask->event.severity   = evt.severity;
        event_mask->event.comp       = evt.comp;
        event_mask->event.comp_cat   = evt.comp_cat;
        event_mask->event.event_cat  = evt.event_cat;
    }
    else 
        return FTB_ERR_INVALID_FIELD;
    return FTB_SUCCESS;
}

static void util_finalize_component(FTBC_comp_info_t * comp_info)
{
    FTBM_Component_dereg(comp_info->id);
    free(comp_info->id);
    FTB_INFO("in util_finalize_component -- after FTBM_Component_dereg\n");
    if (comp_info->event_queue) {
        FTBU_list_node_t *temp;
        FTBU_list_node_t *pos;
        FTBU_list_for_each(pos, comp_info->event_queue, temp){
            FTBC_event_inst_list_t *entry=(FTBC_event_inst_list_t*)pos;
            free(entry);
        }
        free(comp_info->event_queue);
    }

    FTB_INFO("In util_finalize_component: Free comp_info->event_queue\n");

    if (comp_info->properties.catching_type & FTB_EVENT_CATCHING_NOTIFICATION) {
        FTBU_map_iterator_t iter;
        comp_info->finalizing = 1;
        FTB_INFO("signal the callback"); 
        pthread_cond_signal(&comp_info->cond);
        unlock_component(comp_info);
        pthread_join(comp_info->callback, NULL);
        pthread_cond_destroy(&comp_info->cond);
        lock_component(comp_info);
        iter = FTBU_map_begin(comp_info->callback_map);
        while (iter!=FTBU_map_end(comp_info->callback_map)) {
            FTBC_callback_entry_t *entry = (FTBC_callback_entry_t*)FTBU_map_get_data(iter);
            free(entry->mask);
            iter = FTBU_map_next_iterator(iter);
        }
        FTBU_map_finalize(comp_info->callback_map);
    }
    FTB_INFO("in util_finalize_component -- done\n");
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
    comp_info->finalizing = 1;
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
        FTBU_map_finalize(FTBC_tag_map);
        FTBC_comp_info_map = NULL;
    }
    unlock_client();
    
    FTB_INFO("FTBC_Finalize Out");
    return FTB_SUCCESS;
}

int FTBC_Abort(FTB_client_handle_t handle)
{   
    FTBC_comp_info_t *comp_info;
    FTBU_map_iterator_t iter;
    if (FTBC_comp_info_map == NULL) {
        FTB_WARNING("Not initialized");
        return FTB_ERR_GENERAL;
    }
    iter = FTBU_map_find(FTBC_comp_info_map, FTBU_MAP_UINT_KEY(handle));
    if (iter == FTBU_map_end(FTBC_comp_info_map)) {
        FTB_WARNING("Not registered");
        return FTB_ERR_INVALID_PARAMETER;
    }
    comp_info = (FTBC_comp_info_t *)FTBU_map_get_data(iter); 
    FTB_INFO("FTBC_Abort In");

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


int FTBC_Reg_throw(FTB_client_handle_t handle, const char *event_name)
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

int FTBC_Reg_catch_polling_event(FTB_client_handle_t handle, const char* event_name)
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
    strcpy(msg.event.region, "ALL");
    strcpy(msg.event.jobid, "ALL");
    strcpy(msg.event.inst_name, "ALL");
    strcpy(msg.event.hostname, "ALL");
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

int FTBC_Reg_all_predefined_catch(FTB_client_handle_t handle)
{
    FTBM_msg_t msg;
    FTBC_comp_info_t *comp_info;
    int i, count;
    FTB_event_t *events;
    int ret;
    LOOKUP_COMP_INFO(handle,comp_info);

    FTB_INFO("FTBC_Reg_catch_polling_mask In");
    if (!(comp_info->properties.catching_type & FTB_EVENT_CATCHING_POLLING)){
        FTB_INFO("FTBC_Reg_catch_polling_mask Out");
        return FTB_ERR_NOT_SUPPORTED;
    }

    if (FTBM_get_comp_catch_count(comp_info->id->client_id.comp_cat, comp_info->id->client_id.comp, &count) != FTB_SUCCESS) {
        FTB_WARNING("FTBM_get_comp_catch_count failed");
        return FTB_ERR_GENERAL;
    }

    if (count == 0) {
        FTB_INFO("FTBC_Reg_catch_polling_mask Out");
        return FTB_SUCCESS;
    }

    events = (FTB_event_t *)malloc(sizeof(FTB_event_t)*count);
    if (FTBM_get_comp_catch_masks(comp_info->id->client_id.comp_cat, comp_info->id->client_id.comp, events) != FTB_SUCCESS) {
        FTB_WARNING("FTBM_get_comp_catch_masks failed");
        return FTB_ERR_GENERAL;
    }

    memcpy(&msg.src,comp_info->id,sizeof(FTB_id_t));
    msg.msg_type = FTBM_MSG_TYPE_REG_CATCH;
    FTBM_Get_parent_location_id(&msg.dst.location_id);
    for (i=0;i<count;i++) {
        memcpy(&msg.event, &(events[i]), sizeof(FTB_event_t));
        ret = FTBM_Send(&msg);
        if (ret != FTB_SUCCESS) {
            FTB_WARNING("FTBM_Send failed");
            break;
        }
    }

    return ret;
}

//static void util_add_to_callback_map(FTBC_comp_info_t *comp_info, const FTB_event_t *event, int (*callback)(FTB_event_t *, FTB_id_t *, void*), void *arg)
static void util_add_to_callback_map(FTBC_comp_info_t *comp_info, const FTB_event_t *event, int (*callback)(FTB_catch_event_info_t *, void*), void *arg)
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

//int FTBC_Reg_catch_notify_event(FTB_client_handle_t handle, const char* event_name, int (*callback)(FTB_event_t *, FTB_id_t *, void*), void *arg)
int FTBC_Reg_catch_notify_event(FTB_client_handle_t handle, const char* event_name, int (*callback)(FTB_catch_event_info_t *, void*), void *arg)
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

//int FTBC_Reg_catch_notify_mask(FTB_client_handle_t handle, const FTB_event_t *event, int (*callback)(FTB_event_t *, FTB_id_t *, void*), void *arg)
int FTBC_Reg_catch_notify_mask(FTB_client_handle_t handle, const FTB_event_t *event, int (*callback)(FTB_catch_event_info_t *, void*), void *arg)
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

int FTBC_Throw(FTB_client_handle_t handle, const char *event_name,  FTB_event_data_t *datadetails)
{
    FTBM_msg_t msg;
    FTBC_comp_info_t *comp_info;
    int ret;
    LOOKUP_COMP_INFO(handle,comp_info);
    
    FTB_INFO("FTBC_Throw In");
    ret = FTBM_get_event_by_name(event_name, &msg.event);
    if (ret == FTB_ERR_HASHKEY_NOT_FOUND) 
        return FTB_ERR_VALUE_NOT_FOUND; 
    lock_client();
    memcpy(&msg.event.dynamic_data, tag_string, FTB_MAX_DYNAMIC_DATA_SIZE);
    unlock_client();
    comp_info->seqnum +=1; 
    memcpy(&msg.src,comp_info->id,sizeof(FTB_id_t));
    if (datadetails == NULL) {
        datadetails=(FTB_event_data_t*)malloc(sizeof(FTB_event_data_t));
    }
    memcpy(&msg.event.event_data, datadetails, sizeof(FTB_event_data_t));
    strcpy(msg.event.hostname, msg.src.location_id.hostname);
    strcpy(msg.event.region, comp_info->comp_region);
    strcpy(msg.event.inst_name, comp_info->comp_inst_name);
    strcpy(msg.event.jobid, comp_info->comp_jobid);
    msg.event.seqnum=comp_info->seqnum;
    msg.msg_type = FTBM_MSG_TYPE_NOTIFY;
    FTBM_Get_parent_location_id(&msg.dst.location_id);
    ret = FTBM_Send(&msg);
    FTB_INFO("FTBC_Throw Out");
    return ret;
}

//int FTBC_Catch(FTB_client_handle_t handle, FTB_event_t *event, FTB_id_t *src)
int FTBC_Catch(FTB_subscribe_handle_t shandle, FTB_catch_event_info_t *ce_event)
{
    FTBM_msg_t msg;
    FTB_location_id_t incoming_src;
    FTBC_comp_info_t *comp_info;
    FTBC_event_inst_list_t *entry;
    FTB_INFO("FTBC_Catch In");
    FTB_client_handle_t handle;
    handle = shandle.chandle;
    LOOKUP_COMP_INFO(handle,comp_info);

    //*error_msg = 0;
    if (!(comp_info->properties.catching_type & FTB_EVENT_CATCHING_POLLING)){
        FTB_INFO("FTBC_Catch Out");
        return FTB_ERR_NOT_SUPPORTED;
    }

   
    lock_component(comp_info);
    if (comp_info->event_queue_size > 0) {
        int event_found = 0;
        FTBC_event_inst_list_t *start = (FTBC_event_inst_list_t *)comp_info->event_queue->next;
        entry = (FTBC_event_inst_list_t *)comp_info->event_queue->next;
        do { 
            if (FTBU_match_mask(&entry->event_inst, &shandle.cmask.event))  {
                event_found = 1;
                char name[128];
                int ret = 0;
                ret = FTBM_get_string_by_id(entry->event_inst.comp_cat, name, "comp_cat");
                if (ret != FTB_SUCCESS) {
                    FTB_WARNING("Incoming event matches mask but is from unknown component category\n");
                }
                else 
                    strcpy(ce_event->comp_cat, name);
                
                ret = FTBM_get_string_by_id(entry->event_inst.comp, name, "comp");
                if (ret != FTB_SUCCESS) {
                    FTB_WARNING("Incoming event matches mask but is from unknown component\n");
                }
                else 
                    strcpy(ce_event->comp, name);
                
                ret = FTBM_get_string_by_id(entry->event_inst.severity, name, "severity");
                if (ret != FTB_SUCCESS) {
                    FTB_WARNING("Incoming event matches mask but is of unknown severity\n");
                }
                else 
                    strcpy(ce_event->severity, name);
                
                ret = FTBM_get_string_by_id(entry->event_inst.event_name, name, "event_name");
                if (ret != FTB_SUCCESS) {
                    FTB_WARNING("Incoming event matches mask but the event name is unknown to FTB\n");
                }
                else 
                    strcpy(ce_event->event_name, name);
                strcpy(ce_event->region, entry->event_inst.region);
                strcpy(ce_event->jobid, entry->event_inst.jobid);
                strcpy(ce_event->inst_name, entry->event_inst.inst_name);
                strcpy(ce_event->hostname, entry->event_inst.hostname);
                memcpy(&ce_event->seqnum, &entry->event_inst.seqnum, sizeof(int));
                memcpy(&ce_event->len, &entry->event_inst.len, sizeof(FTB_tag_len_t));
                memcpy(&ce_event->event_data, &entry->event_inst.event_data, sizeof(FTB_event_data_t));
                memcpy(&ce_event->incoming_src, &entry->src.location_id, sizeof(FTB_location_id_t));
                //memcpy(ce_event->dynamic_data, entry->event_inst.dynamic_data, entry->event_inst.len);
                memcpy(ce_event->dynamic_data, entry->event_inst.dynamic_data,  FTB_MAX_DYNAMIC_DATA_SIZE);
                break;
            }
            else {
                entry = (FTBC_event_inst_list_t *)entry->next;
            }
        } while (entry != start);
        if (event_found) {
            FTBU_list_remove_entry((FTBU_list_node_t *)entry);
            comp_info->event_queue_size--;
            unlock_component(comp_info);
        }
        free(entry);
        if (event_found) {
            FTB_INFO("FTBC_Catch Out");
            return FTB_CAUGHT_EVENT;
        }
    }
    
    /*Then poll FTBM once*/
    while (FTBM_Poll(&msg,&incoming_src) == FTB_SUCCESS) {
        if (FTB_CLIENT_ID_TO_HANDLE(msg.dst.client_id)==handle) {
            FTB_INFO("Polled event for myself");
            int is_for_callback = 0;
            if (msg.msg_type != FTBM_MSG_TYPE_NOTIFY) {
                FTB_WARNING("unexpected message type %d",msg.msg_type);
                continue;
            }
            if (comp_info->properties.catching_type & FTB_EVENT_CATCHING_NOTIFICATION) {
                FTB_INFO("Test whether belonging to any callback mask");
                FTBU_map_iterator_t iter;
                iter = FTBU_map_begin(comp_info->callback_map);
                while (iter != FTBU_map_end(comp_info->callback_map)) {
                    FTBC_callback_entry_t *callback_entry = (FTBC_callback_entry_t *)FTBU_map_get_data(iter);
                    if (FTBU_match_mask(&msg.event, callback_entry->mask))  {
                        entry = (FTBC_event_inst_list_t *)malloc(sizeof(FTBC_event_inst_list_t));
                        memcpy(&entry->event_inst, &msg.event, sizeof(FTB_event_t));
                        memcpy(&entry->src, &msg.src, sizeof(FTB_id_t));
                        FTBU_list_add_back(comp_info->callback_event_queue, (FTBU_list_node_t *)entry);
                        pthread_cond_signal(&comp_info->cond);
                        FTB_INFO("The event belongs to my callback");
                        is_for_callback = 1;
                        break;
                    }
                    iter = FTBU_map_next_iterator(iter);
                }
            }
            if (!is_for_callback) {
                FTB_INFO("Not for callback");
                char name[128];
                int ret = 0;
                if (FTBU_match_mask(&msg.event, &shandle.cmask.event))  {
                    
                    ret = FTBM_get_string_by_id(msg.event.comp_cat, name, "comp_cat");
                    if (ret != FTB_SUCCESS) {
                        FTB_WARNING("Incoming event matches mask but is from unknown component category\n");
                    }
                    else 
                        strcpy(ce_event->comp_cat, name);
                    
                    ret = FTBM_get_string_by_id(msg.event.comp, name, "comp");
                    if (ret != FTB_SUCCESS) {
                        FTB_WARNING("Incoming event matches mask but is from unknown component\n");
                    }
                    else 
                        strcpy(ce_event->comp, name);
                    
                    ret = FTBM_get_string_by_id(msg.event.severity, name, "severity");
                    if (ret != FTB_SUCCESS) {
                        FTB_WARNING("Incoming event matches mask but is of unknown severity\n");
                    }
                    else 
                        strcpy(ce_event->severity, name);
                    
                    ret = FTBM_get_string_by_id(msg.event.event_name, name, "event_name");
                    if (ret != FTB_SUCCESS) {
                        FTB_WARNING("Incoming event matches mask but the event name is unknown to FTB\n");
                    }
                    else 
                        strcpy(ce_event->event_name, name);
                    strcpy(ce_event->region, msg.event.region);
                    strcpy(ce_event->jobid,  msg.event.jobid);
                    strcpy(ce_event->inst_name, msg.event.inst_name);
                    strcpy(ce_event->hostname, msg.event.hostname);
                    memcpy(&ce_event->seqnum, &msg.event.seqnum, sizeof(int));
                    memcpy(&ce_event->len, &msg.event.len, sizeof(FTB_tag_len_t));
                    memcpy(&ce_event->event_data, &msg.event.event_data, sizeof(FTB_event_data_t));
                    memcpy(&ce_event->incoming_src, &msg.src.location_id, sizeof(FTB_location_id_t));
                    memcpy(ce_event->dynamic_data, msg.event.dynamic_data,  FTB_MAX_DYNAMIC_DATA_SIZE);
                    //memcpy(temp_src, &entry->src, sizeof(FTB_id_t));
                    unlock_component(comp_info);
                    FTB_INFO("FTBC_Catch Out");
                    return FTB_CAUGHT_EVENT;
                }
                else {
                    unlock_component(comp_info);
                    util_handle_FTBM_msg(&msg);
                    FTB_INFO("FTBC_Catch Out");
                    return FTB_CAUGHT_NO_EVENT;
                }
            }
        }
        else {
            unlock_component(comp_info);
            FTB_INFO("Polled event for someone else");
            util_handle_FTBM_msg(&msg);
            lock_component(comp_info);
            FTB_INFO("Testing whether someone else got my events");
            int event_found = 0;
            if (comp_info->event_queue_size > 0) {
                entry = (FTBC_event_inst_list_t *)comp_info->event_queue->next;
                FTBC_event_inst_list_t * start = (FTBC_event_inst_list_t *)comp_info->event_queue->next;
                char name[128];
                int ret = 0;
                do { 
                    if (FTBU_match_mask(&entry->event_inst, &shandle.cmask.event))  {
                        ret = FTBM_get_string_by_id(entry->event_inst.comp_cat, name, "comp_cat");
                        if (ret != FTB_SUCCESS) {
                            FTB_WARNING("Incoming event matches mask but is from unknown component category\n");
                        }
                        else 
                            strcpy(ce_event->comp_cat, name);
                    
                        ret = FTBM_get_string_by_id(entry->event_inst.comp, name, "comp");
                        if (ret != FTB_SUCCESS) {
                            FTB_WARNING("Incoming event matches mask but is from unknown component\n");
                        }
                        else 
                            strcpy(ce_event->comp, name);
                    
                        ret = FTBM_get_string_by_id(entry->event_inst.severity, name, "severity");
                        if (ret != FTB_SUCCESS) {
                            FTB_WARNING("Incoming event matches mask but is of unknown severity\n");
                        }
                        else 
                            strcpy(ce_event->severity, name);
                    
                        ret = FTBM_get_string_by_id(entry->event_inst.event_name, name, "event_name");
                        if (ret != FTB_SUCCESS) {
                            FTB_WARNING("Incoming event matches mask but the event name is unknown to FTB\n");
                        }
                        else 
                            strcpy(ce_event->event_name, name);
                        strcpy(ce_event->region, entry->event_inst.region);
                        strcpy(ce_event->jobid, entry->event_inst.jobid);
                        strcpy(ce_event->inst_name, entry->event_inst.inst_name);
                        strcpy(ce_event->hostname, entry->event_inst.hostname);
                        memcpy(&ce_event->seqnum, &entry->event_inst.seqnum, sizeof(int));
                        memcpy(&ce_event->len, &entry->event_inst.len, sizeof(FTB_tag_len_t));
                        memcpy(&ce_event->event_data, &entry->event_inst.event_data, sizeof(FTB_event_data_t));
                        memcpy(&ce_event->incoming_src, &entry->src.location_id, sizeof(FTB_location_id_t));
                        memcpy(ce_event->dynamic_data, entry->event_inst.dynamic_data,  FTB_MAX_DYNAMIC_DATA_SIZE);
                        event_found = 1;
                        break;
                    }
                    else {
                        entry = (FTBC_event_inst_list_t *)entry->next;
                    }
                } while (entry != start);
                if (event_found) {
                    FTBU_list_remove_entry((FTBU_list_node_t *)entry);
                    comp_info->event_queue_size--;
                }
                free(entry);
                unlock_component(comp_info);
                if (event_found) {
                    FTB_INFO("FTBC_Catch Out");
                 return FTB_CAUGHT_EVENT;
                }
            }
            FTB_INFO("No events put in my queue, keep polling");
        }
    }
    unlock_component(comp_info);
    FTB_INFO("FTBC_Catch Out");
    return FTB_CAUGHT_NO_EVENT;
}

static void util_update_tag_string()
{
    int offset = 0;
    FTBU_map_iterator_t iter;
    /*Format: 1 byte tag count (M) + M*{tag, len(N), N byte data}  */
    memcpy(tag_string, &tag_count, sizeof(tag_count));
    offset+=sizeof(tag_count);
    iter = FTBU_map_begin(FTBC_tag_map);
    while (iter != FTBU_map_end(FTBC_tag_map)) {
        FTBC_tag_entry_t *entry = (FTBC_tag_entry_t *)FTBU_map_get_data(iter);
        memcpy(tag_string+offset, &entry->tag, sizeof(FTB_tag_t));
        offset+=sizeof(FTB_tag_t);
        memcpy(tag_string+offset, &entry->data_len, sizeof(FTB_tag_len_t));
        offset+=sizeof(FTB_tag_len_t);
        memcpy(tag_string+offset, &entry->data, entry->data_len);
        offset+=entry->data_len;
        iter = FTBU_map_next_iterator(iter);
    }
    tag_size = offset;
}

int FTBC_Add_dynamic_tag(FTB_client_handle_t handle, FTB_tag_t tag, const char *tag_data, FTB_tag_len_t data_len)
{
    FTBU_map_iterator_t iter;

    FTB_INFO("FTBC_Add_dynamic_tag In");

    lock_client();
    if (FTB_MAX_DYNAMIC_DATA_SIZE - tag_size < data_len + sizeof(FTB_tag_len_t) + sizeof(FTB_tag_t)) {
        FTB_INFO("FTBC_Add_dynamic_tag Out");
        return FTB_ERR_TAG_NO_SPACE;
    }
    
    iter = FTBU_map_find(FTBC_tag_map, FTBU_MAP_PTR_KEY(&tag));
    if (iter == FTBU_map_end(FTBC_tag_map)) {
        FTBC_tag_entry_t *entry = (FTBC_tag_entry_t *)malloc(sizeof(FTBC_tag_entry_t));
        FTB_INFO("create a new tag");
        entry->tag = tag;
        entry->owner = handle;
        entry->data_len = data_len;
        memcpy(entry->data, tag_data, data_len);
        FTBU_map_insert(FTBC_tag_map, FTBU_MAP_PTR_KEY(&entry->tag), (void*)entry);
        tag_count++;
    }
    else {
        FTBC_tag_entry_t *entry = (FTBC_tag_entry_t *)FTBU_map_get_data(iter);
        if (entry->owner == handle) {
            FTB_INFO("update tag");
            entry->data_len = data_len;
            memcpy(entry->data, tag_data, data_len);
        }
        else {
            FTB_INFO("FTBC_Add_dynamic_tag Out");
            return FTB_ERR_TAG_CONFLICT;
        }
    }

    util_update_tag_string();
    unlock_client();
    FTB_INFO("FTBC_Add_dynamic_tag Out");
    return FTB_SUCCESS;
}

int FTBC_Remove_dynamic_tag(FTB_client_handle_t handle, FTB_tag_t tag)
{
    FTBU_map_iterator_t iter;
    FTBC_tag_entry_t *entry;

    FTB_INFO("FTBC_Remove_dynamic_tag In");
    lock_client();
    iter = FTBU_map_find(FTBC_tag_map, FTBU_MAP_PTR_KEY(&tag));
    if (iter == FTBU_map_end(FTBC_tag_map)) {
        FTB_INFO("FTBC_Remove_dynamic_tag Out");
        return FTB_ERR_TAG_NOT_FOUND;
    }

    entry = (FTBC_tag_entry_t *)FTBU_map_get_data(iter);
    if (entry->owner != handle) {
        FTB_INFO("FTBC_Remove_dynamic_tag Out");
        return FTB_ERR_TAG_CONFLICT;
    }

    FTBU_map_remove_iter(iter);
    tag_count--;
    free(entry);
    util_update_tag_string();
    unlock_client();
    FTB_INFO("FTBC_Remove_dynamic_tag Out");
    return FTB_SUCCESS;
}

//int FTBC_Read_dynamic_tag(const FTB_event_t *event, FTB_tag_t tag, char *tag_data, FTB_tag_len_t *data_len)
int FTBC_Read_dynamic_tag(const FTB_catch_event_info_t *event, FTB_tag_t tag, char *tag_data, FTB_tag_len_t *data_len)
{
    uint8_t tag_count;
    uint8_t i;
    int offset;

    FTB_INFO("FTBC_Read_dynamic_tag In");
    memcpy(&tag_count, event->dynamic_data, sizeof(tag_count));
    offset = 1;
    for (i=0;i<tag_count;i++) {
        FTB_tag_t temp_tag;
        FTB_tag_len_t temp_len;
        memcpy(&temp_tag, event->dynamic_data + offset, sizeof(FTB_tag_t));
        offset+=sizeof(FTB_tag_t);
        memcpy(&temp_len, event->dynamic_data + offset, sizeof(FTB_tag_len_t));
        offset+=sizeof(FTB_tag_len_t);
        if (tag == temp_tag) {
            if (*data_len < temp_len) {
                FTB_INFO("FTBC_Read_dynamic_tag  Out");
                return FTB_ERR_TAG_NO_SPACE;
            }
            else {
                *data_len = temp_len;
                memcpy(tag_data, event->dynamic_data + offset, temp_len);
                FTB_INFO("FTBC_Read_dynamic_tag Out");
                return FTB_SUCCESS;
            }
        }
    }

    FTB_INFO("FTBC_Read_dynamic_tag Out");
    return FTB_ERR_TAG_NOT_FOUND;
}

