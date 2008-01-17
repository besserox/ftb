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
    uint8_t subscription_type;
    uint8_t err_handling;
    unsigned int max_polling_queue_len;
    FTBU_list_node_t* event_queue; /* entry type: event_inst_list*/
    int event_queue_size;
    pthread_mutex_t lock;
    pthread_cond_t cond;
    pthread_t callback;
    volatile int finalizing;
    FTBU_list_node_t* callback_event_queue; /* entry type: event_inst_list*/
    FTBC_map_mask_2_callback_entry_t *callback_map;
    FTB_region_t comp_region; //This assumes a component can throw only in one region
    FTB_client_jobid_t comp_client_jobid;
    FTB_client_name_t comp_client_name;
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

int FTBC_util_is_equal_clienthandle(const void *lhs_void, const void* rhs_void)
{
    FTB_client_handle_t *lhs = (FTB_client_handle_t *)lhs_void;
    FTB_client_handle_t *rhs = (FTB_client_handle_t *)rhs_void;
    return FTBU_is_equal_clienthandle(lhs, rhs);
}


#define LOOKUP_COMP_INFO(handle, comp_info)  do {\
    FTBU_map_iterator_t iter;\
    if (FTBC_comp_info_map == NULL) {\
        FTB_WARNING("Not initialized");\
        return FTB_ERR_GENERAL;\
    }\
    lock_client();\
    iter = FTBU_map_find(FTBC_comp_info_map, FTBU_MAP_PTR_KEY(&handle));\
    if (iter == FTBU_map_end(FTBC_comp_info_map)) {\
        FTB_WARNING("Not registered");\
        unlock_client();\
        return FTB_ERR_INVALID_PARAMETER;\
    }\
    comp_info = (FTBC_comp_info_t *)FTBU_map_get_data(iter); \
    unlock_client();\
}while(0)

static int convert_clientid_to_clienthandle (const FTB_client_id_t client_id, FTB_client_handle_t *client_handle) {
    memcpy(client_handle, &client_id, sizeof(FTB_client_handle_t));
    return 1;
}

static int util_split_namespace(const FTB_eventspace_t ns, char *region_name, char *category_name, char *component_name)
{
    char *tempstr=(char*)malloc(sizeof(FTB_eventspace_t)+1);
    char *str=(char*)malloc(sizeof(FTB_eventspace_t)+1);
    int i=0; 
    if (strlen(ns) >= FTB_MAX_EVENTSPACE) {
        FTB_INFO("In util_split_namespace-1");
        return FTB_ERR_EVENTSPACE_FORMAT;
    }
    for (i=0; i<=strlen(ns); i++) tempstr[i]=toupper(ns[i]);
    str=strsep(&tempstr,"."); strcpy(region_name,str);
    str=strsep(&tempstr,"."); strcpy(category_name,str);
    str=strsep(&tempstr,"."); strcpy(component_name,str);
    if ((strcmp(region_name,"\0")==0) || (strcmp(category_name,"\0")==0) 
            || (strcmp(component_name,"\0")==0)  || (tempstr != NULL)) { 
        FTB_INFO("In util_split_namespace-2");
        return FTB_ERR_EVENTSPACE_FORMAT;
    }
    return FTB_SUCCESS;
}

static void util_push_to_comp_polling_list(FTBC_comp_info_t *comp_info, FTBC_event_inst_list_t *entry)
{/*Assumes it holds the lock of that comp*/
    FTBC_event_inst_list_t *temp;
    if (comp_info->subscription_type & FTB_SUBSCRIPTION_POLLING)  {
        if (comp_info->event_queue_size == comp_info->max_polling_queue_len) {
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
    convert_clientid_to_clienthandle(msg->dst.client_id, &handle);
    //handle = FTB_CLIENT_ID_TO_HANDLE(msg->dst.client_id);
    lock_client();
    iter = FTBU_map_find(FTBC_comp_info_map, FTBU_MAP_PTR_KEY(&handle));
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
    if (comp_info->subscription_type & FTB_SUBSCRIPTION_NOTIFY) {
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
                strcpy(ce_event.comp_cat, entry->event_inst.comp_cat);
                strcpy(ce_event.comp, entry->event_inst.comp);
                strcpy(ce_event.client_name, entry->event_inst.client_name);
                strcpy(ce_event.severity, entry->event_inst.severity);
                strcpy(ce_event.event_name, entry->event_inst.event_name);
                strcpy(ce_event.region, entry->event_inst.region);
                strcpy(ce_event.client_jobid, entry->event_inst.client_jobid);
                strcpy(ce_event.hostname, entry->event_inst.hostname);
                memcpy(&ce_event.seqnum, &entry->event_inst.seqnum, sizeof(int));
                memcpy(&ce_event.len, &entry->event_inst.len, sizeof(FTB_tag_len_t));
                memcpy(&ce_event.event_properties, &entry->event_inst.event_properties, sizeof(FTB_event_properties_t));
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

int FTBC_Connect(const FTB_client_t *cinfo, uint8_t extension, FTB_client_handle_t *client_handle)
{
    FTBC_comp_info_t *comp_info;
    /*
    FTB_comp_cat_code_t category;
    FTB_comp_code_t component;
    */
    char category_name[FTB_MAX_EVENTSPACE];
    char component_name[FTB_MAX_EVENTSPACE];
    char region_name[FTB_MAX_EVENTSPACE];
    
    FTB_INFO("FTBC_Connect In");
    comp_info = (FTBC_comp_info_t*)malloc(sizeof(FTBC_comp_info_t));

    /* For BGL - set appropriate subscription_style */
    if (strcasecmp(cinfo->client_subscription_style, "FTB_SUBSCRIPTION_POLLING") == 0) {    
        FTB_INFO("FTBC_Connect In - 2");
        comp_info->subscription_type = FTB_SUBSCRIPTION_POLLING;
        FTB_INFO("FTBC_Connect In - 3");
        if (cinfo->client_polling_queue_len > 0) {
            comp_info->max_polling_queue_len = cinfo->client_polling_queue_len;
            FTB_INFO("FTBC_Connect In - 4");
        }
        else {
            comp_info->max_polling_queue_len = FTB_DEFAULT_POLLING_Q_LEN;
            FTB_INFO("FTBC_Connect In - 5");
        }
    }
    else if (strcasecmp(cinfo->client_subscription_style, "FTB_SUBSCRIPTION_NOTIFY") == 0) {
        comp_info->subscription_type = FTB_SUBSCRIPTION_NOTIFY;
        comp_info->max_polling_queue_len = 0;
    }
    else if (strcasecmp(cinfo->client_subscription_style, "FTB_SUBSCRIPTION_NONE") == 0) {
        comp_info->subscription_type = FTB_SUBSCRIPTION_NONE;
        comp_info->max_polling_queue_len = 0;
    }
    else if ((strcasecmp(cinfo->client_subscription_style, "FTB_SUBSCRIPTION_BOTH") == 0) ||
                (strcasecmp(cinfo->client_subscription_style, "") == 0)) {
        comp_info->subscription_type = FTB_SUBSCRIPTION_POLLING + FTB_SUBSCRIPTION_NOTIFY;
        if (cinfo->client_polling_queue_len > 0) {
            comp_info->max_polling_queue_len = cinfo->client_polling_queue_len;
        }
        else {
            comp_info->max_polling_queue_len = FTB_DEFAULT_POLLING_Q_LEN;
        }
    }
    else {
        return FTB_ERR_SUBSCRIPTION_STYLE;
    }
    /* Expose the error handling as option to use */
    comp_info->err_handling = FTB_ERR_HANDLE_NONE;


    if (util_split_namespace(cinfo->event_space, region_name, category_name, component_name)) {
        FTB_WARNING("Invalid namespace format");
        FTB_INFO("FTBC_Connect Out");
        return FTB_ERR_EVENTSPACE_FORMAT;
    }

    lock_client();
    if (num_components == 0) {
        //FTBC_comp_info_map = FTBU_map_init(NULL); /*Since the key: client_handle is uint32_t*/
        FTBC_comp_info_map = FTBU_map_init(FTBC_util_is_equal_clienthandle);
        FTBC_tag_map = FTBU_map_init(FTBC_util_is_equal_tag);
        memset(tag_string,0,FTB_MAX_DYNAMIC_DATA_SIZE);
        FTBM_hash_init();
        FTBM_event_table_init();
        /*
        FTBM_compcat_table_init();
        FTBM_comp_table_init();
        FTBM_severity_table_init();
        */
        FTBM_Init(1);
    }
    num_components++;
    unlock_client();
    /*
    if (FTBM_get_compcat_by_name(category_name, &category) == FTB_ERR_HASHKEY_NOT_FOUND) {
        FTB_INFO("After FTBM_get_compcat_by_name");
        return FTB_ERR_EVENTSPACE_FORMAT;
    }
    if (FTBM_get_comp_by_name(component_name, &component)  == FTB_ERR_HASHKEY_NOT_FOUND) {
        FTB_INFO("After FTBM_get_comp_by_name");
            return FTB_ERR_EVENTSPACE_FORMAT;
    }
    comp_info->id = (FTB_id_t *)malloc(sizeof(FTB_id_t));
    comp_info->id->client_id.comp_cat = category;
    comp_info->id->client_id.comp = component;
    */
    comp_info->id = (FTB_id_t *)malloc(sizeof(FTB_id_t));
    strcpy(comp_info->id->client_id.comp_cat, category_name);
    strcpy(comp_info->id->client_id.comp, component_name);
    comp_info->id->client_id.ext = extension;
    if (strlen(cinfo->client_name) > (FTB_MAX_CLIENT_NAME-1)) 
        return FTB_ERR_INVALID_VALUE;
    else {
        strcpy(comp_info->comp_client_name, cinfo->client_name);    //Qs: Is this needed?
        strcpy(comp_info->id->client_id.client_name, cinfo->client_name);
    }
    FTBM_Get_location_id(&comp_info->id->location_id);
    //comp_info->client_handle = FTB_CLIENT_ID_TO_HANDLE(comp_info->id->client_id);
    convert_clientid_to_clienthandle(comp_info->id->client_id, &comp_info->client_handle);
    *client_handle = comp_info->client_handle;
    
    comp_info->finalizing = 0;
    comp_info->event_queue_size = 0;
    comp_info->seqnum = 0;
    strcpy(comp_info->comp_region, region_name);
    if (strlen(cinfo->client_jobid) > (FTB_MAX_CLIENT_JOBID-1)) 
        return FTB_ERR_INVALID_VALUE;
    else
    strcpy(comp_info->comp_client_jobid, cinfo->client_jobid);
    if (comp_info->subscription_type & FTB_SUBSCRIPTION_POLLING) {
        if (comp_info->max_polling_queue_len == 0) {
            FTB_WARNING("Polling queue size equal to 0, changing to default value");
            comp_info->max_polling_queue_len = FTB_DEFAULT_POLLING_Q_LEN;
        }
        comp_info->event_queue = (FTBU_list_node_t*)malloc(sizeof(FTBU_list_node_t));
        FTBU_list_init(comp_info->event_queue);
    }
    else {
        comp_info->event_queue = NULL;
    }

    pthread_mutex_init(&comp_info->lock, NULL);
    if (comp_info->subscription_type & FTB_SUBSCRIPTION_NOTIFY) {
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
    //if (FTBU_map_insert(FTBC_comp_info_map,FTBU_MAP_UINT_KEY(comp_info->client_handle), (void *)comp_info)==FTBU_EXIST) {
    if (FTBU_map_insert(FTBC_comp_info_map,FTBU_MAP_PTR_KEY(&comp_info->client_handle), (void *)comp_info)==FTBU_EXIST) {
        FTB_WARNING("This component has already been registered");
        FTB_INFO("FTBC_Connect Out");
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
            FTB_INFO("FTBC_Connect Out");
            return ret;
        }
    }
    
    FTB_INFO("FTBC_Connect Out");
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
                strcpy(event_mask->event.client_name, "ALL");
                strcpy(event_mask->event.client_jobid, "ALL");

                strcpy(event_mask->event.severity, "ALL");
                strcpy(event_mask->event.comp_cat, "ALL");
                strcpy(event_mask->event.comp, "ALL");
                //strcpy(event_mask->event.event_cat, "ALL");
                strcpy(event_mask->event.event_name, "ALL");
                /*
                event_mask->event.severity = 0-1;
                event_mask->event.comp_cat = 0-1; 
                event_mask->event.comp = 0-1; 
                event_mask->event.event_cat = 0-1; 
                event_mask->event.event_name = 0-1;
                */
    }
    else if (strcasecmp(field_name, "hostname") == 0) {
        if (strlen(field_val) > (FTB_MAX_HOST_NAME-1)) 
            return FTB_ERR_INVALID_VALUE;
        strcpy(event_mask->event.hostname, field_val);
    }
    else if (strcasecmp(field_name, "client_jobid") == 0) {
        if (strlen(field_val) > (FTB_MAX_CLIENT_JOBID-1)) 
            return FTB_ERR_INVALID_VALUE; 
        strcpy(event_mask->event.client_jobid, field_val);
    }
    else if (strcasecmp(field_name, "client_name") == 0) {
        if (strlen(field_val) > (FTB_MAX_CLIENT_NAME-1)) 
            return FTB_ERR_INVALID_VALUE; 
        strcpy(event_mask->event.client_name, field_val);
    }
    else if (strcasecmp(field_name, "namespace") == 0) {
        char region_name[FTB_MAX_EVENTSPACE];
        char comp_name[FTB_MAX_EVENTSPACE];
        char comp_cat[FTB_MAX_EVENTSPACE];
        //FTB_comp_cat_code_t comp_cat_code = 0;
        //FTB_comp_code_t comp_code = 0;
        int ret = 0;
        
        if (sizeof(field_val) > (FTB_MAX_EVENTSPACE-1)) 
            return FTB_ERR_INVALID_VALUE; 
        
        ret = util_split_namespace(field_val, region_name, comp_cat, comp_name);
        if (ret != FTB_SUCCESS) {
            FTB_INFO("After util_split_namespace-3");
            return FTB_ERR_EVENTSPACE_FORMAT;
        }
        /*
        if ((strlen(region_name) > (FTB_MAX_REGION-1)) || 
                (strlen(comp_name) > (FTB_MAX_COMP-1)) || 
                (strlen(comp_cat) > (FTB_MAX_COMP_CAT-1))) {
            return FTB_ERR_INVALID_VALUE; 
        }
        */
        strcpy(event_mask->event.region, region_name);
        strcpy(event_mask->event.comp_cat, comp_cat);
        strcpy(event_mask->event.comp, comp_name);
        //strcpy(event_mask->event.event_cat, "ALL");
       
        /*
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
        */
        
    }
    else if (strcasecmp(field_name, "severity") == 0) {
        //int ret = 0, i = 0;
        //char tempstr[FTB_MAX_SEVERITY+1];
        //FTB_severity_code_t severity_code = 0;
        
        if (sizeof(field_val) > (FTB_MAX_SEVERITY-1)) 
            return FTB_ERR_VALUE_NOT_FOUND; 
        /*
        for (i=0; i<=strlen(field_val); i++) 
            tempstr[i]=toupper(field_val[i]);
        ret = FTBM_get_severity_by_name(tempstr, &severity_code);
        if (ret == FTB_ERR_HASHKEY_NOT_FOUND) 
            return FTB_ERR_VALUE_NOT_FOUND;
        event_mask->event.severity=severity_code;
        */
        strcpy( event_mask->event.severity, field_val);
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
        strcpy(event_mask->event.event_name, evt.event_name);
        strcpy(event_mask->event.severity, evt.severity);
        strcpy(event_mask->event.comp, evt.comp);
        strcpy(event_mask->event.comp_cat, evt.comp_cat);
        //strcpy(event_mask->event.event_cat, evt.event_cat);
        /*    
        event_mask->event.event_name = evt.event_name;
        event_mask->event.severity   = evt.severity;
        event_mask->event.comp       = evt.comp;
        event_mask->event.comp_cat   = evt.comp_cat;
        event_mask->event.event_cat  = evt.event_cat;
        */
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

    if (comp_info->subscription_type & FTB_SUBSCRIPTION_NOTIFY) {
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

int FTBC_Disconnect(FTB_client_handle_t handle)
{
    FTBC_comp_info_t *comp_info;
    LOOKUP_COMP_INFO(handle,comp_info);
    FTB_INFO("FTBC_Disconnect In");
    
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
    FTBU_map_remove_key(FTBC_comp_info_map, FTBU_MAP_PTR_KEY(&handle));
    if (comp_info->subscription_type & FTB_SUBSCRIPTION_NOTIFY) {
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
    
    FTB_INFO("FTBC_Disconnect Out");
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


int FTBC_Reg_catch_polling_mask(FTB_client_handle_t handle, const FTB_event_t *event)
{
    FTBM_msg_t msg;
    FTBC_comp_info_t *comp_info;
    int ret;
    LOOKUP_COMP_INFO(handle,comp_info);

    FTB_INFO("FTBC_Reg_catch_polling_mask In");
    if (!(comp_info->subscription_type & FTB_SUBSCRIPTION_POLLING)){
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


//int FTBC_Reg_catch_notify_mask(FTB_client_handle_t handle, const FTB_event_t *event, int (*callback)(FTB_event_t *, FTB_id_t *, void*), void *arg)
int FTBC_Reg_catch_notify_mask(FTB_client_handle_t handle, const FTB_event_t *event, int (*callback)(FTB_catch_event_info_t *, void*), void *arg)
{
    FTBM_msg_t msg;
    FTBC_comp_info_t *comp_info;
    int ret;
    LOOKUP_COMP_INFO(handle,comp_info);

    FTB_INFO("FTBC_Reg_catch_notify_mask In");
    if (!(comp_info->subscription_type & FTB_SUBSCRIPTION_NOTIFY)){
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

int FTBC_Publish(FTB_client_handle_t handle, const char *event_name,  const FTB_event_properties_t *event_properties, FTB_event_handle_t *event_handle)
{
    FTBM_msg_t msg;
    FTBC_comp_info_t *comp_info;
    int ret;
    LOOKUP_COMP_INFO(handle,comp_info);
    
    FTB_INFO("FTBC_Publish In");
    ret = FTBM_get_event_by_name(event_name, &msg.event);
    if (ret == FTB_ERR_HASHKEY_NOT_FOUND) 
        return FTB_ERR_VALUE_NOT_FOUND; 
    lock_client();
    memcpy(&msg.event.dynamic_data, tag_string, FTB_MAX_DYNAMIC_DATA_SIZE);
    unlock_client();
    comp_info->seqnum +=1; 
    memcpy(&msg.src,comp_info->id,sizeof(FTB_id_t));
    //if (datadetails == NULL) {
    //    datadetails=(FTB_event_data_t*)malloc(sizeof(FTB_event_data_t));
    //}
    //memcpy(&msg.event.event_data, datadetails, sizeof(FTB_event_data_t));
    if (event_properties == NULL) {
        event_properties = (FTB_event_properties_t*)malloc(sizeof(FTB_event_properties_t));
    }
    memcpy(&msg.event.event_properties, event_properties, sizeof(FTB_event_properties_t));
    strcpy(msg.event.hostname, msg.src.location_id.hostname);
    strcpy(msg.event.region, comp_info->comp_region);
    strcpy(msg.event.client_name, comp_info->comp_client_name);
    strcpy(msg.event.client_jobid, comp_info->comp_client_jobid);
    msg.event.seqnum=comp_info->seqnum;
    msg.msg_type = FTBM_MSG_TYPE_NOTIFY;
    FTBM_Get_parent_location_id(&msg.dst.location_id);
    ret = FTBM_Send(&msg);
    //qs: create the event handle and return it to use. FTB does not need
    //the event handle in anyway.
    /* 
    if (event_handle == NULL) 
        return FTB_ERR_NULL_POINTER;
    else {
        memcpy(event_handle->id, comp_info->id,sizeof(FTB_id_t));
        event_handle->seqnum = comp_info->seqnum;
    }*/
    event_handle = NULL; //Qs: Remove this
    FTB_INFO("FTBC_Publish Out");
    return ret;
}

//int FTBC_Catch(FTB_client_handle_t handle, FTB_event_t *event, FTB_id_t *src)
int FTBC_Poll_event(FTB_subscribe_handle_t shandle, FTB_catch_event_info_t *ce_event)
{
    FTBM_msg_t msg;
    FTB_location_id_t incoming_src;
    FTBC_comp_info_t *comp_info;
    FTBC_event_inst_list_t *entry;
    FTB_INFO("FTBC_Poll_event In");
    FTB_client_handle_t handle;
    handle = shandle.chandle;
    LOOKUP_COMP_INFO(handle,comp_info);

    //*error_msg = 0;
    if (!(comp_info->subscription_type & FTB_SUBSCRIPTION_POLLING)){
        FTB_INFO("FTBC_Poll_event Out");
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
                strcpy(ce_event->comp_cat, entry->event_inst.comp_cat);
                strcpy(ce_event->comp, entry->event_inst.comp);
                strcpy(ce_event->client_name, entry->event_inst.client_name);
                strcpy(ce_event->severity, entry->event_inst.severity);
                strcpy(ce_event->event_name, entry->event_inst.event_name);
                strcpy(ce_event->region, entry->event_inst.region);
                strcpy(ce_event->client_jobid, entry->event_inst.client_jobid);
                strcpy(ce_event->hostname, entry->event_inst.hostname);
                memcpy(&ce_event->seqnum, &entry->event_inst.seqnum, sizeof(int));
                memcpy(&ce_event->len, &entry->event_inst.len, sizeof(FTB_tag_len_t));
                memcpy(&ce_event->event_properties, &entry->event_inst.event_properties, sizeof(FTB_event_properties_t));
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
            FTB_INFO("FTBC_Poll_event Out");
            return FTB_CAUGHT_EVENT;
        }
    }
    
    /*Then poll FTBM once*/
    while (FTBM_Poll(&msg,&incoming_src) == FTB_SUCCESS) {
        FTB_client_handle_t temp_handle;
        convert_clientid_to_clienthandle(msg.dst.client_id, &temp_handle);
        //if (FTB_CLIENT_ID_TO_HANDLE(msg.dst.client_id)==handle) {
        if (FTBC_util_is_equal_clienthandle(&temp_handle, &handle)) { //FTB_CLIENT_ID_TO_HANDLE(msg.dst.client_id)==handle) {
            FTB_INFO("Polled event for myself");
            int is_for_callback = 0;
            if (msg.msg_type != FTBM_MSG_TYPE_NOTIFY) {
                FTB_WARNING("unexpected message type %d",msg.msg_type);
                continue;
            }
            if (comp_info->subscription_type & FTB_SUBSCRIPTION_NOTIFY) {
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
                if (FTBU_match_mask(&msg.event, &shandle.cmask.event))  {
                    
                    strcpy(ce_event->comp_cat, msg.event.comp_cat);
                    strcpy(ce_event->comp, msg.event.comp);
                    strcpy(ce_event->client_name, msg.event.client_name);
                    strcpy(ce_event->severity, msg.event.severity);
                    strcpy(ce_event->event_name, msg.event.event_name);
                    strcpy(ce_event->region, msg.event.region);
                    strcpy(ce_event->client_jobid,  msg.event.client_jobid);
                    strcpy(ce_event->client_name, msg.event.client_name);
                    strcpy(ce_event->hostname, msg.event.hostname);
                    memcpy(&ce_event->seqnum, &msg.event.seqnum, sizeof(int));
                    memcpy(&ce_event->len, &msg.event.len, sizeof(FTB_tag_len_t));
                    memcpy(&ce_event->event_properties, &msg.event.event_properties, sizeof(FTB_event_properties_t));
                    memcpy(&ce_event->incoming_src, &msg.src.location_id, sizeof(FTB_location_id_t));
                    memcpy(ce_event->dynamic_data, msg.event.dynamic_data,  FTB_MAX_DYNAMIC_DATA_SIZE);
                    //memcpy(temp_src, &entry->src, sizeof(FTB_id_t));
                    unlock_component(comp_info);
                    FTB_INFO("FTBC_Poll_event Out");
                    return FTB_CAUGHT_EVENT;
                }
                else {
                    unlock_component(comp_info);
                    util_handle_FTBM_msg(&msg);
                    FTB_INFO("FTBC_Poll_event Out");
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
                do { 
                    if (FTBU_match_mask(&entry->event_inst, &shandle.cmask.event))  {
                        strcpy(ce_event->comp_cat, entry->event_inst.comp_cat);
                        strcpy(ce_event->comp, entry->event_inst.comp);
                        strcpy(ce_event->severity, entry->event_inst.severity);
                        strcpy(ce_event->event_name, entry->event_inst.event_name);
                        strcpy(ce_event->region, entry->event_inst.region);
                        strcpy(ce_event->client_jobid, entry->event_inst.client_jobid);
                        strcpy(ce_event->client_name, entry->event_inst.client_name);
                        strcpy(ce_event->hostname, entry->event_inst.hostname);
                        memcpy(&ce_event->seqnum, &entry->event_inst.seqnum, sizeof(int));
                        memcpy(&ce_event->len, &entry->event_inst.len, sizeof(FTB_tag_len_t));
                        memcpy(&ce_event->event_properties, &entry->event_inst.event_properties, sizeof(FTB_event_properties_t));
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
                    FTB_INFO("FTBC_Poll_event Out");
                 return FTB_CAUGHT_EVENT;
                }
            }
            FTB_INFO("No events put in my queue, keep polling");
        }
    }
    unlock_component(comp_info);
    FTB_INFO("FTBC_Poll_event Out");
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
/*
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
    offset = sizeof(tag_count);
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
        offset += temp_len;
    }

    FTB_INFO("FTBC_Read_dynamic_tag Out");
    return FTB_ERR_TAG_NOT_FOUND;
}
*/
