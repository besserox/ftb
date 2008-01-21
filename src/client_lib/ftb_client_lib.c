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
#include "ftb_auxil.h"
#include "ftb_event_def.h"
#include "ftb_util.h"
#include "ftb_manager_lib.h"

#define MAX_SUBSCRIPTION_FIELDS 10
extern FILE* FTBU_log_file_fp;

typedef struct FTBC_event_inst_list {
    struct FTBU_list_node *next;
    struct FTBU_list_node *prev;
    FTB_event_t event_inst;
    FTB_id_t src;
}FTBC_event_inst_list_t;

typedef struct FTBC_callback_entry{
    FTB_event_t *mask;
    int (*callback)(FTB_receive_event_t *, void*);
    void *arg;
}FTBC_callback_entry_t;

typedef struct FTBC_tag_entry {
    FTB_tag_t tag;
    FTB_client_handle_t owner;
    char data[FTB_MAX_DYNAMIC_DATA_SIZE];
    FTB_tag_len_t data_len;
}FTBC_tag_entry_t;

typedef FTBU_map_node_t FTBC_map_mask_2_callback_entry_t;

typedef struct FTBC_comp_info {
    FTB_client_handle_t client_handle;
    FTB_id_t *id;
    FTB_client_jobid_t comp_client_jobid;
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
        return FTB_ERR_INVALID_HANDLE;\
    }\
    comp_info = (FTBC_comp_info_t *)FTBU_map_get_data(iter); \
    unlock_client();\
}while(0)

int print_event(FTB_event_t *event) {
    printf ("event->severity=%s\n", event->severity);
    printf ("event->region=%s\n", event->region);
    printf ("event->comp_cat=%s\n", event->comp_cat);
    printf ("event->comp=%s\n", event->comp);
    printf ("event->event_name=%s\n", event->event_name);
    printf ("event->client_jobid=%s\n", event->client_jobid);
    printf ("event->hostname=%s\n", event->hostname);
    return 0 ;
}

static int convert_clientid_to_clienthandle (const FTB_client_id_t client_id, FTB_client_handle_t *client_handle) {
    memcpy(client_handle, &client_id, sizeof(FTB_client_handle_t));
    return 1;
}

int util_split_namespace(const char *event_space, char *region_name, char *category_name, char *component_name)
{
    char *tempstr = (char *)malloc(strlen(event_space)+1);
    char *str = (char *)malloc(strlen(event_space)+1);
    if (strlen(event_space) >= FTB_MAX_EVENTSPACE) {
        FTB_INFO("In util_split_namespace");
        return FTB_ERR_EVENTSPACE_FORMAT;
    }
    strcpy(tempstr, event_space);
    str=strsep(&tempstr,"."); strcpy(region_name,str);
    str=strsep(&tempstr,"."); strcpy(category_name,str);
    str=strsep(&tempstr,"."); strcpy(component_name,str);
    printf("region = %s, category=%s and component = %s\n", region_name, category_name, component_name);
    if ((strcmp(region_name,"\0")==0) || (strcmp(category_name,"\0")==0) 
            || (strcmp(component_name,"\0")==0)  || (tempstr != NULL) 
            || (!check_alphanumeric_underscore_format(region_name)) 
            || (!check_alphanumeric_underscore_format(category_name)) 
            || (!check_alphanumeric_underscore_format(component_name))) { 
        FTB_INFO("Out util_split_namespace");
        return FTB_ERR_EVENTSPACE_FORMAT;
    }
    return FTB_SUCCESS;
}

int check_subscription_value_pair(const char *lhs, const char *rhs, FTB_event_t *subscription_event) {
    static uint8_t track;
    int ret=0;
    FTB_INFO("In check_subscription_value_pair");
    if ((strcasecmp(lhs, "") == 0) && (strcasecmp(rhs, "") == 0)) {
        track = 1;
        strcpy(subscription_event->severity, "all");
        strcpy(subscription_event->region, "all");
        strcpy(subscription_event->comp_cat, "all");
        strcpy(subscription_event->comp, "all");
        strcpy(subscription_event->event_name, "all");
        strcpy(subscription_event->client_jobid, "all");
        strcpy(subscription_event->hostname, "all");
    }
    else if (strcasecmp(lhs, "severity") ==  0) {
        if (track & 2) 
            return FTB_ERR_SUBSCRIPTION_STR;
        track = track | 2;
        if ((strcasecmp(rhs, "fatal") == 0) ||
            (strcasecmp(rhs, "info") == 0) ||
            (strcasecmp(rhs, "warning") == 0) ||
            (strcasecmp(rhs, "error") == 0) ||
            (strcasecmp(rhs, "all") == 0))
            strcpy(subscription_event->severity, rhs);
        else {
            FTB_INFO("Out check_subscription_value_pair");
            return FTB_ERR_FILTER_VALUE; 
        }
    }
    else if (strcasecmp(lhs, "event_space") == 0) {
        char region[FTB_MAX_EVENTSPACE];
        char comp_name[FTB_MAX_EVENTSPACE];
        char comp_cat[FTB_MAX_EVENTSPACE];
        if (track & 4) 
            return FTB_ERR_SUBSCRIPTION_STR;
        track = track | 4;
        if (strlen(rhs) >= FTB_MAX_EVENTSPACE) {
              FTB_INFO("Out check_subscription_value_pair");
              return FTB_ERR_EVENTSPACE_FORMAT;
        }
        ret = util_split_namespace(rhs, region, comp_cat, comp_name);
        if (ret != FTB_SUCCESS) {
              FTB_INFO("Out check_subscription_value_pair");
              return FTB_ERR_EVENTSPACE_FORMAT; 
        }
        strcpy(subscription_event->region, region);
        strcpy(subscription_event->comp_cat, comp_cat);
        strcpy(subscription_event->comp, comp_name);
    }
    else if (strcasecmp(lhs, "jobid") == 0) {
        if (track & 8) 
            return FTB_ERR_SUBSCRIPTION_STR;
        track = track | 8;
        if (strlen(rhs) >= FTB_MAX_CLIENT_JOBID)  {
            FTB_INFO("Out check_subscription_value_pair");
            return FTB_ERR_FILTER_VALUE; 
        }
        strcpy(subscription_event->client_jobid, rhs);
    }
    else if (strcasecmp(lhs, "host_name") == 0) {
        if (track & 16) 
            return FTB_ERR_SUBSCRIPTION_STR;
        track = track | 16;
        if (strlen(rhs) >= FTB_MAX_HOST_NAME) {
            FTB_INFO("Out check_subscription_value_pair");
            return FTB_ERR_FILTER_VALUE;
        }
        strcpy(subscription_event->hostname, rhs);
    }
    else if (strcasecmp(lhs, "event_name") == 0) {
        if (track & 32) 
            return FTB_ERR_SUBSCRIPTION_STR;
        track = track | 32;
        if ((strlen(rhs) >= FTB_MAX_EVENT_NAME) 
                ||(!check_alphanumeric_underscore_format(lhs))) {
            FTB_INFO("Out check_subscription_value_pair");
            return FTB_ERR_FILTER_VALUE;
        }
        strcpy(subscription_event->event_name, rhs);
    }
    else {
        FTB_INFO("Out check_subscription_value_pair");
        return FTB_ERR_FILTER_ATTR;
    }
    FTB_INFO("Out check_subscription_value_pair");
    return FTB_SUCCESS;
}

int parse_subscription_string(const char *subscription_str, FTB_event_t *subscription_event) {

    int subscriptionstr_len; 
    char *tempstr  = (char *)malloc(strlen(subscription_str)+1);
    char *pairs[MAX_SUBSCRIPTION_FIELDS],*lhs[MAX_SUBSCRIPTION_FIELDS], *rhs[MAX_SUBSCRIPTION_FIELDS];
    int i=0, j=0, ret=0;

    FTB_INFO("In function parse_subscription_string");
    tempstr =trim_string(subscription_str);
    ret = check_subscription_value_pair("", "", subscription_event);
    if ((subscriptionstr_len = strlen(tempstr)) == 0) return ret;
    while (tempstr != NULL) {
        pairs[i] = (char *)malloc(subscriptionstr_len+1);
        lhs[i] = (char *)malloc(subscriptionstr_len+1);
        rhs[i] = (char *)malloc(subscriptionstr_len+1);
        if ((pairs[i] = strsep(&tempstr, ",")) == NULL)
            return FTB_ERR_SUBSCRIPTION_STR;
        if ((lhs[i] = strsep(&pairs[i], "=")) == NULL)
            return FTB_ERR_SUBSCRIPTION_STR;
        else
            lhs[i] = trim_string(lhs[i]);  
        if ((rhs[i] = strsep(&pairs[i], "=")) == NULL)
            return FTB_ERR_SUBSCRIPTION_STR;
        else 
            rhs[i] = trim_string(rhs[i]);
        printf("tempstr=%s, pairs=%s, lhs=%s and rhs=%s\n", tempstr, pairs[i], lhs[i], rhs[i]);
        if ((strlen(lhs[i]) == 0) || (strlen(rhs[i]) == 0) 
                || (pairs[i] != NULL))
            return FTB_ERR_SUBSCRIPTION_STR;
        for (j=0; j<strlen(lhs[i]); j++)  {
            if (lhs[i][j] == ' ') {
                FTB_INFO("Out function parse_subscription_string");
                return FTB_ERR_FILTER_ATTR;
            }
        }
        for (j=0; j<strlen(rhs[i]); j++) {
            if (rhs[i][j] == ' ') {
                FTB_INFO("Out function parse_subscription_string");
                return FTB_ERR_FILTER_VALUE;
            }
        }
        if ((ret = check_subscription_value_pair(lhs[i], rhs[i], subscription_event)) != FTB_SUCCESS) 
            return ret;
        i++;
        if (i > MAX_SUBSCRIPTION_FIELDS) { 
            FTB_INFO("Subscription string has too many fields. This maybe an internal FTB error"); 
            FTB_INFO("Out function parse_subscription_string");
            return FTB_ERR_SUBSCRIPTION_STR; 
        }
    }
    FTB_INFO("Out function parse_subscription_string");
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

static void util_add_to_callback_map(FTBC_comp_info_t *comp_info, const FTB_event_t *event, int (*callback)(FTB_receive_event_t *, void*), void *arg)
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

static void util_remove_from_callback_map(FTBC_comp_info_t *comp_info, const FTB_event_t *event)
{
    FTBC_callback_entry_t *entry = (FTBC_callback_entry_t *)malloc(sizeof(FTBC_callback_entry_t));
    entry->mask = (FTB_event_t *)malloc(sizeof(FTB_event_t));
    int ret = 0;

    memcpy(entry->mask, event, sizeof(FTB_event_t));
    lock_component(comp_info);
    ret = FTBU_map_remove_key(comp_info->callback_map, FTBU_MAP_PTR_KEY(entry->mask));
    unlock_component(comp_info);
    // The key should be found! If the subscribe handle's subscription_event
    // was invalid, it should have been caught before
    //if (ret == FTBU_NOT_EXIST) return FTB_ERR_INVALID_HANDLE;
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
                FTB_receive_event_t receive_event;
                strcpy(receive_event.comp_cat, entry->event_inst.comp_cat);
                strcpy(receive_event.comp, entry->event_inst.comp);
                strcpy(receive_event.client_name, entry->event_inst.client_name);
                strcpy(receive_event.severity, entry->event_inst.severity);
                strcpy(receive_event.event_name, entry->event_inst.event_name);
                strcpy(receive_event.region, entry->event_inst.region);
                strcpy(receive_event.client_jobid, entry->event_inst.client_jobid);
                strcpy(receive_event.hostname, entry->event_inst.hostname);
                memcpy(&receive_event.seqnum, &entry->event_inst.seqnum, sizeof(int));
                memcpy(&receive_event.len, &entry->event_inst.len, sizeof(FTB_tag_len_t));
                memcpy(&receive_event.event_properties, &entry->event_inst.event_properties, sizeof(FTB_event_properties_t));
                memcpy(&receive_event.incoming_src, &entry->src.location_id, sizeof(FTB_location_id_t));
                memcpy(receive_event.dynamic_data, entry->event_inst.dynamic_data,  FTB_MAX_DYNAMIC_DATA_SIZE);
                (*callback_entry->callback)(&receive_event, callback_entry->arg);
                //(*callback_entry->callback)((FTB_receive_event_t *)&entry->event_inst, callback_entry->arg);
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


static void util_finalize_component(FTBC_comp_info_t * comp_info)
{
    FTBM_Component_dereg(comp_info->id);
    free(comp_info->id);
    FTB_INFO("in util_finalize_component -- after FTBM_Component_dereg");
    if (comp_info->event_queue) {
        FTBU_list_node_t *temp;
        FTBU_list_node_t *pos;
        FTBU_list_for_each(pos, comp_info->event_queue, temp){
            FTBC_event_inst_list_t *entry=(FTBC_event_inst_list_t*)pos;
            free(entry);
        }
        free(comp_info->event_queue);
    }

    FTB_INFO("In util_finalize_component: Free comp_info->event_queue");

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
    FTB_INFO("in util_finalize_component -- done");
}


int FTBC_Connect(const FTB_client_t *cinfo, uint8_t extension, FTB_client_handle_t *client_handle)
{
    FTBC_comp_info_t *comp_info;
    char category_name[FTB_MAX_EVENTSPACE];
    char component_name[FTB_MAX_EVENTSPACE];
    char region_name[FTB_MAX_EVENTSPACE];
    
    FTB_INFO("FTBC_Connect In");
    if (client_handle == NULL)
        return FTB_ERR_NULL_POINTER;

    comp_info = (FTBC_comp_info_t*)malloc(sizeof(FTBC_comp_info_t));

    /* For BGL - set appropriate subscription_style */
    if (strcasecmp(cinfo->client_subscription_style, "FTB_SUBSCRIPTION_POLLING") == 0) {    
        comp_info->subscription_type = FTB_SUBSCRIPTION_POLLING;
        if (cinfo->client_polling_queue_len > 0) {
            comp_info->max_polling_queue_len = cinfo->client_polling_queue_len;
        }
        else {
            comp_info->max_polling_queue_len = FTB_DEFAULT_POLLING_Q_LEN;
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
        FTBM_Init(1);
    }
    num_components++;
    unlock_client();

    comp_info->id = (FTB_id_t *)malloc(sizeof(FTB_id_t));
    comp_info->id->client_id.ext = extension;
    strcpy(comp_info->id->client_id.region, region_name);
    strcpy(comp_info->id->client_id.comp_cat, category_name);
    strcpy(comp_info->id->client_id.comp, component_name);
    if (strlen(cinfo->client_name) > (FTB_MAX_CLIENT_NAME-1)) 
        return FTB_ERR_INVALID_VALUE;
    else 
        strcpy(comp_info->id->client_id.client_name, cinfo->client_name);
    //comp_info->client_handle = FTB_CLIENT_ID_TO_HANDLE(comp_info->id->client_id);
    convert_clientid_to_clienthandle(comp_info->id->client_id, &comp_info->client_handle);
    *client_handle = comp_info->client_handle;
    
    FTBM_Get_location_id(&comp_info->id->location_id);
    comp_info->finalizing = 0;
    comp_info->event_queue_size = 0;
    comp_info->seqnum = 0;
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
    if (FTBU_map_insert(FTBC_comp_info_map, FTBU_MAP_PTR_KEY(&comp_info->client_handle), (void *)comp_info)==FTBU_EXIST) {
        FTB_WARNING("This component has already been registered");
        FTB_INFO("FTBC_Connect Out");
        return FTB_ERR_DUP_CALL;
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


int FTBC_Subscribe_with_polling(FTB_subscribe_handle_t *subscribe_handle, FTB_client_handle_t client_handle, const char *subscription_str)
{
    FTBM_msg_t msg;
    FTBC_comp_info_t *comp_info;
    FTB_event_t *subscription_event = (FTB_event_t *)malloc(sizeof(FTB_event_t));
    int ret;

    if (subscribe_handle == NULL) 
        return FTB_ERR_NULL_POINTER;

    FTB_INFO("FTBC_Subscribe_with_polling In");

    LOOKUP_COMP_INFO(client_handle, comp_info);
    if (!(comp_info->subscription_type & FTB_SUBSCRIPTION_POLLING)){
        FTB_INFO("FTBC_Subscribe_with_polling Out");
        return FTB_ERR_NOT_SUPPORTED;
    }

    if ((ret = parse_subscription_string(subscription_str, subscription_event)) != FTB_SUCCESS)
        return ret;

    memcpy(&msg.event, subscription_event, sizeof(FTB_event_t));
    memcpy(&msg.src, comp_info->id, sizeof(FTB_id_t));
    msg.msg_type = FTBM_MSG_TYPE_REG_CATCH;
    FTBM_Get_parent_location_id(&msg.dst.location_id);
    memcpy(&subscribe_handle->client_handle, &client_handle, sizeof(FTB_client_handle_t));
    memcpy(&subscribe_handle->subscription_event, subscription_event, sizeof(FTB_event_t));
    subscribe_handle->subscription_type = FTB_SUBSCRIPTION_POLLING;
    subscribe_handle->valid = 1;
    ret = FTBM_Send(&msg);

    FTB_INFO("FTBC_Subscribe_with_polling Out");
    return ret;
}

int FTBC_Subscribe_with_callback(FTB_subscribe_handle_t *subscribe_handle, FTB_client_handle_t client_handle, const char *subscription_str, int (*callback)(FTB_receive_event_t *, void*), void *arg)
{
    FTBM_msg_t msg;
    FTBC_comp_info_t *comp_info;
    FTB_event_t *subscription_event = (FTB_event_t *)malloc(sizeof(FTB_event_t));
    int ret;

    if (subscribe_handle == NULL) 
        return FTB_ERR_NULL_POINTER;

    FTB_INFO("FTBC_Subscribe_with_callback In");
    LOOKUP_COMP_INFO(client_handle,comp_info);

    if (!(comp_info->subscription_type & FTB_SUBSCRIPTION_NOTIFY)){
        FTB_INFO("FTBC_Subscribe_with_callback Out");
        return FTB_ERR_NOT_SUPPORTED;
    }
    if ((ret = parse_subscription_string(subscription_str, subscription_event)) != FTB_SUCCESS)
        return ret;

    util_add_to_callback_map(comp_info, subscription_event, callback, arg);

    memcpy(&msg.event, subscription_event, sizeof(FTB_event_t));
    memcpy(&msg.src, comp_info->id, sizeof(FTB_id_t));
    msg.msg_type = FTBM_MSG_TYPE_REG_CATCH;
    FTBM_Get_parent_location_id(&msg.dst.location_id);
    memcpy(&subscribe_handle->client_handle, &client_handle, sizeof(FTB_client_handle_t));
    memcpy(&subscribe_handle->subscription_event, subscription_event, sizeof(FTB_event_t));
    subscribe_handle->subscription_type = FTB_SUBSCRIPTION_NOTIFY;
    subscribe_handle->valid = 1;
    ret = FTBM_Send(&msg);
    FTB_INFO("FTBC_Subscribe_with_callback Out");
    return ret;
}

int FTBC_Publish(FTB_client_handle_t handle, const char *event_name,  const FTB_event_properties_t *event_properties, FTB_event_handle_t *event_handle)
{
    FTBM_msg_t msg;
    FTBC_comp_info_t *comp_info;
    int ret;

    if (event_handle == NULL) 
        return FTB_ERR_NULL_POINTER;
    
    FTB_INFO("FTBC_Publish In");
    LOOKUP_COMP_INFO(handle,comp_info);

    ret = FTBM_get_event_by_name(event_name, &msg.event);
    if (ret != FTB_SUCCESS) return ret;

    lock_client();
    memcpy(&msg.event.dynamic_data, tag_string, FTB_MAX_DYNAMIC_DATA_SIZE);
    unlock_client();
    comp_info->seqnum +=1;

    memcpy(&msg.src, comp_info->id, sizeof(FTB_id_t));
    if (event_properties == NULL) {
        event_properties = (FTB_event_properties_t*)malloc(sizeof(FTB_event_properties_t));
    }

    memcpy(&msg.event.event_properties, event_properties, sizeof(FTB_event_properties_t));
    strcpy(msg.event.hostname, msg.src.location_id.hostname);
    strcpy(msg.event.region, comp_info->id->client_id.region);
    strcpy(msg.event.client_name, comp_info->id->client_id.client_name);
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

int FTBC_Poll_event(FTB_subscribe_handle_t subscribe_handle, FTB_receive_event_t *receive_event) {
    FTBM_msg_t msg;
    FTB_location_id_t incoming_src;
    FTBC_comp_info_t *comp_info;
    FTBC_event_inst_list_t *entry;
    FTB_client_handle_t client_handle;

    FTB_INFO("FTBC_Poll_event In");

    if (receive_event == NULL) 
        return FTB_ERR_NULL_POINTER;

    if (subscribe_handle.valid == 0) 
        return FTB_ERR_INVALID_HANDLE;
    
    memcpy(&client_handle, &subscribe_handle.client_handle, sizeof(FTB_client_handle_t));
    LOOKUP_COMP_INFO(client_handle,comp_info);

    if (!(comp_info->subscription_type & FTB_SUBSCRIPTION_POLLING)) {
        FTB_INFO("FTBC_Poll_event Out");
        return FTB_ERR_NOT_SUPPORTED;
    }

    lock_component(comp_info);
    if (comp_info->event_queue_size > 0) {
        int event_found = 0;
        FTBC_event_inst_list_t *start = (FTBC_event_inst_list_t *)comp_info->event_queue->next;
        entry = (FTBC_event_inst_list_t *)comp_info->event_queue->next;
        do { 
            if (FTBU_match_mask(&entry->event_inst, &subscribe_handle.subscription_event))  {
                event_found = 1;
                strcpy(receive_event->comp_cat, entry->event_inst.comp_cat);
                strcpy(receive_event->comp, entry->event_inst.comp);
                strcpy(receive_event->client_name, entry->event_inst.client_name);
                strcpy(receive_event->severity, entry->event_inst.severity);
                strcpy(receive_event->event_name, entry->event_inst.event_name);
                strcpy(receive_event->region, entry->event_inst.region);
                strcpy(receive_event->client_jobid, entry->event_inst.client_jobid);
                strcpy(receive_event->hostname, entry->event_inst.hostname);
                memcpy(&receive_event->seqnum, &entry->event_inst.seqnum, sizeof(int));
                memcpy(&receive_event->len, &entry->event_inst.len, sizeof(FTB_tag_len_t));
                memcpy(&receive_event->event_properties, &entry->event_inst.event_properties, sizeof(FTB_event_properties_t));
                memcpy(&receive_event->incoming_src, &entry->src.location_id, sizeof(FTB_location_id_t));
                //memcpy(receive_event->dynamic_data, entry->event_inst.dynamic_data, entry->event_inst.len);
                memcpy(receive_event->dynamic_data, entry->event_inst.dynamic_data,  FTB_MAX_DYNAMIC_DATA_SIZE);
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
            return FTB_SUCCESS;
        }
    }
    
    /*Then poll FTBM once*/
    while (FTBM_Poll(&msg,&incoming_src) == FTB_SUCCESS) {
        FTB_client_handle_t temp_handle;
        convert_clientid_to_clienthandle(msg.dst.client_id, &temp_handle);
        //if (FTB_CLIENT_ID_TO_HANDLE(msg.dst.client_id)==handle) {
        if (FTBC_util_is_equal_clienthandle(&temp_handle, &client_handle)) { //FTB_CLIENT_ID_TO_HANDLE(msg.dst.client_id)==handle) {
            FTB_INFO("Polled event for myself");
            int is_for_callback = 0;
            if (msg.msg_type != FTBM_MSG_TYPE_NOTIFY) {
                FTB_WARNING("unexpected message type %d",msg.msg_type);
                continue;
            }
            if (comp_info->subscription_type & FTB_SUBSCRIPTION_NOTIFY) {
                FTB_INFO("Test whether belonging to any callback subscription string");
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
                if (FTBU_match_mask(&msg.event, &subscribe_handle.subscription_event))  {
                    
                    strcpy(receive_event->comp_cat, msg.event.comp_cat);
                    strcpy(receive_event->comp, msg.event.comp);
                    strcpy(receive_event->client_name, msg.event.client_name);
                    strcpy(receive_event->severity, msg.event.severity);
                    strcpy(receive_event->event_name, msg.event.event_name);
                    strcpy(receive_event->region, msg.event.region);
                    strcpy(receive_event->client_jobid,  msg.event.client_jobid);
                    strcpy(receive_event->client_name, msg.event.client_name);
                    strcpy(receive_event->hostname, msg.event.hostname);
                    memcpy(&receive_event->seqnum, &msg.event.seqnum, sizeof(int));
                    memcpy(&receive_event->len, &msg.event.len, sizeof(FTB_tag_len_t));
                    memcpy(&receive_event->event_properties, &msg.event.event_properties, sizeof(FTB_event_properties_t));
                    memcpy(&receive_event->incoming_src, &msg.src.location_id, sizeof(FTB_location_id_t));
                    memcpy(receive_event->dynamic_data, msg.event.dynamic_data,  FTB_MAX_DYNAMIC_DATA_SIZE);
                    //memcpy(temp_src, &entry->src, sizeof(FTB_id_t));
                    unlock_component(comp_info);
                    FTB_INFO("FTBC_Poll_event Out");
                    return FTB_SUCCESS;
                }
                else {
                    unlock_component(comp_info);
                    util_handle_FTBM_msg(&msg);
                    FTB_INFO("FTBC_Poll_event Out");
                    return FTB_GOT_NO_EVENT;
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
                    if (FTBU_match_mask(&entry->event_inst, &subscribe_handle.subscription_event))  {
                        strcpy(receive_event->comp_cat, entry->event_inst.comp_cat);
                        strcpy(receive_event->comp, entry->event_inst.comp);
                        strcpy(receive_event->severity, entry->event_inst.severity);
                        strcpy(receive_event->event_name, entry->event_inst.event_name);
                        strcpy(receive_event->region, entry->event_inst.region);
                        strcpy(receive_event->client_jobid, entry->event_inst.client_jobid);
                        strcpy(receive_event->client_name, entry->event_inst.client_name);
                        strcpy(receive_event->hostname, entry->event_inst.hostname);
                        memcpy(&receive_event->seqnum, &entry->event_inst.seqnum, sizeof(int));
                        memcpy(&receive_event->len, &entry->event_inst.len, sizeof(FTB_tag_len_t));
                        memcpy(&receive_event->event_properties, &entry->event_inst.event_properties, sizeof(FTB_event_properties_t));
                        memcpy(&receive_event->incoming_src, &entry->src.location_id, sizeof(FTB_location_id_t));
                        memcpy(receive_event->dynamic_data, entry->event_inst.dynamic_data,  FTB_MAX_DYNAMIC_DATA_SIZE);
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
                 return FTB_SUCCESS;
                }
            }
            FTB_INFO("No events put in my queue, keep polling");
        }
    }
    unlock_component(comp_info);
    FTB_INFO("FTBC_Poll_event Out");
    return FTB_GOT_NO_EVENT;
}

int FTBC_Unsubscribe(FTB_subscribe_handle_t *subscribe_handle) {
    FTBC_comp_info_t *comp_info;
    FTBM_msg_t msg;
    int ret;

    FTB_INFO("FTBC_Unsubscribe In");
    LOOKUP_COMP_INFO(subscribe_handle->client_handle, comp_info);

    if ((subscribe_handle == NULL) || (subscribe_handle->valid == 0)) {
        FTB_INFO("FTBC_Unsubscribe Out");
        return FTB_ERR_INVALID_HANDLE;
    }

    if (subscribe_handle->subscription_type & FTB_SUBSCRIPTION_NOTIFY) {
        //Subscription was registered for callback mechanism
        util_remove_from_callback_map(comp_info, &subscribe_handle->subscription_event);    
    }
    //If Subscription was registered using polling mechanism, so do nothing
    //for now. Just set the valid attribute for the handle  to 0;

    memcpy(&msg.event, &subscribe_handle->subscription_event, sizeof(FTB_event_t));
    memcpy(&msg.src, comp_info->id, sizeof(FTB_id_t));
    msg.msg_type = FTBM_MSG_TYPE_REG_CATCH_CANCEL;
    FTBM_Get_parent_location_id(&msg.dst.location_id);
    subscribe_handle->valid = 0;
    ret = FTBM_Send(&msg);
    FTB_INFO("FTBC_Unsubscribe Out");
    return ret;
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


/*
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

static void util_update_tag_string()
{
    int offset = 0;
    FTBU_map_iterator_t iter;
    //Format: 1 byte tag count (M) + M*{tag, len(N), N byte data} 
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
int FTBC_Read_dynamic_tag(const FTB_receive_event_t *event, FTB_tag_t tag, char *tag_data, FTB_tag_len_t *data_len)
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
