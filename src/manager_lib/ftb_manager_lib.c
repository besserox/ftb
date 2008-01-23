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
#include "ftb_util.h"
#include "ftb_manager_lib.h"
#include "ftb_network.h"

extern FILE* FTBU_log_file_fp;

typedef FTBU_map_node_t FTBMI_set_event_mask_t; /*a set of event_mask*/

typedef struct FTBMI_comp_info{
    FTB_id_t id;
    pthread_mutex_t lock;
    FTBMI_set_event_mask_t* catch_event_set;
}FTBMI_comp_info_t;

typedef FTBU_map_node_t FTBMI_map_ftb_id_2_comp_info_t; /*ftb_id as key and comp_info as data*/
typedef FTBU_map_node_t FTBMI_map_event_mask_2_comp_info_map_t; /*event_mask as key, a _map_ of comp_info as data*/

typedef struct FTBM_node_info{
    FTB_location_id_t parent; /*NULL if root*/
    FTB_id_t self;
    uint8_t err_handling;
    int leaf;
    volatile int waiting;
    FTBMI_map_ftb_id_2_comp_info_t *peers; /*the map of peers includes parent*/
    FTBMI_map_event_mask_2_comp_info_map_t *catch_event_map; 
}FTBMI_node_info_t;

static pthread_mutex_t FTBMI_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t FTBN_lock = PTHREAD_MUTEX_INITIALIZER;
static FTBMI_node_info_t FTBMI_info;
static volatile int FTBMI_initialized = 0;

static inline void lock_manager()
{
    pthread_mutex_lock(&FTBMI_lock);
}

static inline void unlock_manager()
{
    pthread_mutex_unlock(&FTBMI_lock);
}

static inline void lock_network()
{
    pthread_mutex_lock(&FTBN_lock);
}

static inline void unlock_network()
{
    pthread_mutex_unlock(&FTBN_lock);
}

static inline void lock_comp(FTBMI_comp_info_t *comp_info)
{
    pthread_mutex_lock(&comp_info->lock);
}

static inline void unlock_comp(FTBMI_comp_info_t *comp_info)
{
    pthread_mutex_unlock(&comp_info->lock);
}


int FTBMI_util_is_equal_ftb_id(const void* lhs_void, const void* rhs_void)
{
    FTB_id_t *lhs = (FTB_id_t *)lhs_void;
    FTB_id_t *rhs = (FTB_id_t *)rhs_void;
    return FTBU_is_equal_ftb_id(lhs, rhs);
}


int FTBMI_util_is_equal_event(const void * lhs_void, const void * rhs_void)
{
    FTB_event_t *lhs = (FTB_event_t *)lhs_void;
    FTB_event_t *rhs = (FTB_event_t *)rhs_void;
    return FTBU_is_equal_event(lhs, rhs);
}

static void FTBMI_util_reg_propagation(int msg_type, const FTB_event_t *event, const FTB_location_id_t *incoming)
{
    FTBM_msg_t msg;
    int ret;
    FTBU_map_iterator_t iter_comp;

    msg.msg_type = msg_type;
    memcpy(&msg.src, &FTBMI_info.self, sizeof(FTB_id_t));
    memcpy(&msg.event,event,sizeof(FTB_event_t));
    for (iter_comp = FTBU_map_begin(FTBMI_info.peers);
           iter_comp != FTBU_map_end(FTBMI_info.peers);
           iter_comp = FTBU_map_next_iterator(iter_comp)) 
    {
        FTB_id_t *id = (FTB_id_t *)FTBU_map_get_key(iter_comp).key_ptr;
        /*Propagation to other FTB agents but not the source*/
        if (!((strcasecmp(id->client_id.comp_cat, "FTB_COMP_CAT_BACKPLANE") == 0) 
                    && (strcasecmp(id->client_id.comp, "FTB_COMP_MANAGER") == 0) 
                    && (id->client_id.ext == 0))
                || FTBU_is_equal_location_id(&id->location_id,incoming))
            continue;
        memcpy(&msg.dst,id,sizeof(FTB_id_t));
        ret = FTBN_Send_msg(&msg);
        if (ret != FTB_SUCCESS) {
            FTB_WARNING("FTBN_Send_msg failed");
        }
    }
}

static void FTBMI_util_remove_com_from_catch_map(const FTBMI_comp_info_t *comp, const FTB_event_t *mask)
{
    int ret;
    FTBMI_map_ftb_id_2_comp_info_t *map;
    FTBU_map_iterator_t iter;
    FTB_event_t *mask_manager;
    iter = FTBU_map_find(FTBMI_info.catch_event_map, FTBU_MAP_PTR_KEY(mask));
    if (iter == FTBU_map_end(FTBMI_info.catch_event_map)) {
        FTB_WARNING("not found component map");
        return;
    }
    mask_manager = (FTB_event_t*)FTBU_map_get_key(iter).key_ptr;
    map = (FTBMI_map_ftb_id_2_comp_info_t *)FTBU_map_get_data(iter);
    ret = FTBU_map_remove_key(map, FTBU_MAP_PTR_KEY(&comp->id));
    if (ret == FTBU_NOT_EXIST) {
        FTB_WARNING("not found component");
        return;
    }
    if (FTBU_map_is_empty(map)) {
        FTBU_map_finalize(map);
        FTBU_map_remove_iter(iter);
        /*Cancel catch mask to other ftb cores*/
        FTBMI_util_reg_propagation(FTBM_MSG_TYPE_SUBSCRIPTION_CANCEL, mask_manager, &comp->id.location_id);
        free(mask_manager);
    }
}

static void FTBMI_util_clean_component(FTBMI_comp_info_t *comp)
{/*assumes it has the lock of component*/
    /*remove it from catch map*/
    FTBU_map_iterator_t iter = FTBU_map_begin(comp->catch_event_set);
    for (;iter != FTBU_map_end(comp->catch_event_set); iter=FTBU_map_next_iterator(iter))
    {
        FTB_event_t *mask = (FTB_event_t *)FTBU_map_get_data(iter);
        FTBMI_util_remove_com_from_catch_map(comp,mask);
        free(mask);
    }
    /*Finalize comp's catch set*/
    FTBU_map_finalize(comp->catch_event_set);
}

static inline FTBMI_comp_info_t *FTBMI_lookup_component(const FTB_id_t *id) 
{
    FTBMI_comp_info_t *comp = NULL;
    FTBU_map_iterator_t iter;
    lock_manager();
    iter = FTBU_map_find(FTBMI_info.peers, FTBU_MAP_PTR_KEY(id));
    if (iter != FTBU_map_end(FTBMI_info.peers)) {
        comp = (FTBMI_comp_info_t *)FTBU_map_get_data(iter);
    }
    unlock_manager();
    return comp;
}

static void FTBMI_util_reconnect()
{
    FTBM_msg_t msg;
    int ret;
    memcpy(&msg.src, &FTBMI_info.self, sizeof(FTB_id_t));
    msg.msg_type = FTBM_MSG_TYPE_CLIENT_REG;
    FTBN_Connect(&msg, &FTBMI_info.parent);
    if (FTBMI_info.leaf && FTBMI_info.parent.pid == 0) {
        FTB_WARNING("Can not connect to any ftb agent");
        FTBMI_initialized = 0;
        return;
    }
    
    if (!FTBMI_info.leaf && FTBMI_info.parent.pid != 0) {
        FTBMI_comp_info_t *comp;
        FTBU_map_iterator_t iter;
        FTB_INFO("Adding parent to peers");
        comp = (FTBMI_comp_info_t *)malloc(sizeof(FTBMI_comp_info_t));
        memcpy(&comp->id.location_id, &FTBMI_info.parent, sizeof(FTB_location_id_t));
        strcpy(comp->id.client_id.comp_cat, "FTB_COMP_CAT_BACKPLANE");
        strcpy(comp->id.client_id.comp, "FTB_COMP_MANAGER");
        comp->id.client_id.ext = 0;
        pthread_mutex_init(&comp->lock, NULL);
        comp->catch_event_set = (FTBMI_set_event_mask_t*)FTBU_map_init(FTBMI_util_is_equal_event);

        if (FTBU_map_insert(FTBMI_info.peers, FTBU_MAP_PTR_KEY(&comp->id), (void*)comp) == FTBU_EXIST)
        {
            return;
        }

        FTB_INFO("Register all catches");
        memcpy((void*)&msg.src, (void*)&FTBMI_info.self,sizeof(FTB_id_t));
        msg.msg_type =  FTBM_MSG_TYPE_REG_SUBSCRIPTION;
        memcpy(&msg.dst, &comp->id, sizeof(FTB_id_t));
        for (iter = FTBU_map_begin(FTBMI_info.catch_event_map);
               iter != FTBU_map_end(FTBMI_info.catch_event_map);
               iter = FTBU_map_next_iterator(iter)) 
        {
            FTB_event_t *mask = (FTB_event_t*)FTBU_map_get_key(iter).key_ptr;
            memcpy(&msg.event,mask,sizeof(FTB_event_t));
            ret = FTBN_Send_msg(&msg);
            if (ret != FTB_SUCCESS) {
                FTB_WARNING("FTBN_Send_msg failed");
            }
        }
    }
}

static void FTBMI_util_get_system_id(uint32_t *system_id)
{
    *system_id = 1;
}

static void FTBMI_util_get_location_id(FTB_location_id_t *location_id)
{
    location_id->pid = getpid();
    FTBU_get_output_of_cmd("hostname", location_id->hostname, FTB_MAX_HOST_NAME);
}

int FTBM_Get_catcher_comp_list(const FTB_event_t *event, FTB_id_t **list, int *len)
{
    FTBU_map_iterator_t iter_comp;
    FTBU_map_iterator_t iter_mask;
    FTBMI_map_ftb_id_2_comp_info_t* catcher_set;
    int temp_len=0;
    if (!FTBMI_initialized)
        return FTB_ERR_GENERAL;
    
    /*Contruct a deliver set to keep track which components have already gotten the event and which are not, to avoid duplication*/
    FTB_INFO("FTBM_Get_catcher_comp_list In");
    catcher_set = (FTBMI_map_ftb_id_2_comp_info_t*)FTBU_map_init(FTBMI_util_is_equal_ftb_id);

    lock_manager();
    for (iter_mask=FTBU_map_begin(FTBMI_info.catch_event_map);
           iter_mask!=FTBU_map_end(FTBMI_info.catch_event_map);
           iter_mask=FTBU_map_next_iterator(iter_mask))
    {
        FTB_event_t *mask = (FTB_event_t *)FTBU_map_get_key(iter_mask).key_ptr;
        if (FTBU_match_mask(event, mask)) {
            FTB_INFO("Get the map of components");
            FTBU_map_node_t *map = (FTBU_map_node_t *)FTBU_map_get_data(iter_mask);
            for (iter_comp=FTBU_map_begin(map);
                   iter_comp!=FTBU_map_end(map);
                   iter_comp=FTBU_map_next_iterator(iter_comp)){
                FTBMI_comp_info_t *comp = (FTBMI_comp_info_t *)FTBU_map_get_data(iter_comp);
                FTB_INFO("Test whether component is already in catcher set");
                if (FTBU_map_find(catcher_set, FTBU_MAP_PTR_KEY(&comp->id))!=FTBU_map_end(catcher_set)) {
                    FTB_INFO("already counted in");
                    continue;
                }
                temp_len++;
                FTBU_map_insert(catcher_set, FTBU_MAP_PTR_KEY(&comp->id), (void *)&comp->id);
            }
        }
    }
    unlock_manager();
    *list = (FTB_id_t *)malloc(sizeof(FTB_id_t)*temp_len);
    *len = temp_len;
    for (iter_comp = FTBU_map_begin(catcher_set);
           iter_comp != FTBU_map_end(catcher_set);
           iter_comp = FTBU_map_next_iterator(iter_comp)) 
    {
        FTB_id_t *id = (FTB_id_t*)FTBU_map_get_data(iter_comp);
        temp_len--;
        memcpy(&((*list)[temp_len]),id,sizeof(FTB_id_t));
    }
    FTBU_map_finalize(catcher_set);

    FTB_INFO("FTBM_Get_catcher_comp_list Out");
    
    return FTB_SUCCESS;
}

int FTBM_Release_comp_list(FTB_id_t *list)
{
    FTB_INFO("FTBM_Release_comp_list In");
    free(list);
    FTB_INFO("FTBM_Release_comp_list Out");
    return FTB_SUCCESS;
}


int FTBM_Get_location_id(FTB_location_id_t *location_id)
{
    if (!FTBMI_initialized)
        return FTB_ERR_GENERAL;
    memcpy(location_id,&FTBMI_info.self.location_id,sizeof(FTB_location_id_t));
    return FTB_SUCCESS;
}

int FTBM_Get_parent_location_id(FTB_location_id_t *location_id)
{
    if (!FTBMI_initialized)
        return FTB_ERR_GENERAL;
    memcpy(location_id,&FTBMI_info.parent,sizeof(FTB_location_id_t));
    return FTB_SUCCESS;
}

int FTBM_Init(int leaf)
{
    FTBN_config_info_t config;
    FTBM_msg_t msg;

    FTB_INFO("FTBM_Init In");

    FTBMI_info.leaf = leaf;
    if (!leaf) {
        FTBMI_info.peers = 
            (FTBMI_map_ftb_id_2_comp_info_t*) FTBU_map_init(FTBMI_util_is_equal_ftb_id);
        FTBMI_info.catch_event_map = 
            (FTBMI_map_event_mask_2_comp_info_map_t *)FTBU_map_init(FTBMI_util_is_equal_event);
        FTBMI_info.err_handling = FTB_ERR_HANDLE_RECOVER;
    }
    else {
        FTBMI_info.peers = NULL;
        FTBMI_info.catch_event_map = NULL;
        FTBMI_info.err_handling = FTB_ERR_HANDLE_NONE; /*May change to recover mode if one client component requires so*/
    }

    config.leaf = leaf;
    FTBMI_util_get_system_id(&config.FTB_system_id);
    FTBMI_util_get_location_id(&FTBMI_info.self.location_id);
    //FTBMI_info.self.client_id.comp = FTB_COMP_MANAGER;
    //FTBMI_info.self.client_id.comp_cat = FTB_COMP_CAT_BACKPLANE;
    strcpy(FTBMI_info.self.client_id.comp, "FTB_COMP_MANAGER");
    strcpy(FTBMI_info.self.client_id.comp_cat, "FTB_COMP_CAT_BACKPLANE");
    //strcpy(FTBMI_info.self.client_id.client_name, "");
    FTBMI_info.self.client_id.ext = leaf;
    FTBN_Init(&FTBMI_info.self.location_id, &config);
    memcpy(&msg.src, &FTBMI_info.self, sizeof(FTB_id_t));
    msg.msg_type = FTBM_MSG_TYPE_CLIENT_REG;
    FTBN_Connect(&msg, &FTBMI_info.parent);
    if (leaf && FTBMI_info.parent.pid == 0) {
        FTB_WARNING("Can not connect to any ftb agent");
        FTB_INFO("FTBM_Init Out");
        return FTB_ERR_GENERAL;
    }
    
    if (!leaf && FTBMI_info.parent.pid != 0) {
        FTBMI_comp_info_t *comp;
        FTB_INFO("Adding parent to peers");
        comp = (FTBMI_comp_info_t *)malloc(sizeof(FTBMI_comp_info_t));
        memcpy(&comp->id.location_id, &FTBMI_info.parent, sizeof(FTB_location_id_t));
        //comp->id.client_id.comp_cat = FTB_COMP_CAT_BACKPLANE;
        //comp->id.client_id.comp = FTB_COMP_MANAGER;
        strcpy(comp->id.client_id.comp_cat, "FTB_COMP_CAT_BACKPLANE");
        strcpy(comp->id.client_id.comp, "FTB_COMP_MANAGER");
        comp->id.client_id.ext = 0;
        pthread_mutex_init(&comp->lock, NULL);
        comp->catch_event_set = (FTBMI_set_event_mask_t*)FTBU_map_init(FTBMI_util_is_equal_event);

        if (FTBU_map_insert(FTBMI_info.peers, FTBU_MAP_PTR_KEY(&comp->id), (void*)comp) == FTBU_EXIST)
        {
            return FTB_ERR_GENERAL;
        }
    }

    FTBMI_info.waiting = 0;
    lock_manager();
    FTBMI_initialized = 1;
    unlock_manager();
    FTB_INFO("FTBM_Init Out");
    return FTB_SUCCESS;
}

/*Called when the ftb node get disconnected*/
int FTBM_Finalize()
{
    FTB_INFO("FTBM_Finalize In");
    if (!FTBMI_initialized) {
        FTB_INFO("FTBM_Finalize Out");
        return FTB_SUCCESS;
    }

    lock_manager();
    FTBMI_initialized = 0;
    
    if (!FTBMI_info.leaf) {
        FTBU_map_iterator_t iter = FTBU_map_begin(FTBMI_info.peers);
        for (;iter!=FTBU_map_end(FTBMI_info.peers);iter=FTBU_map_next_iterator(iter)) {
            FTBMI_comp_info_t* comp = (FTBMI_comp_info_t*)FTBU_map_get_data(iter);
            FTBMI_util_clean_component(comp);
            if ((strcasecmp(comp->id.client_id.comp_cat, "FTB_COMP_CAT_BACKPLANE") == 0)
            &&  (strcasecmp(comp->id.client_id.comp, "FTB_COMP_MANAGER") == 0)
            &&  (comp->id.client_id.ext == 0)) {
                int ret;
                FTBM_msg_t msg;
                FTB_INFO("deregister from peer");
                memcpy(&msg.src, &FTBMI_info.self, sizeof(FTB_id_t));
                memcpy(&msg.dst, &comp->id, sizeof(FTB_id_t));
                msg.msg_type =  FTBM_MSG_TYPE_CLIENT_DEREG;
                ret = FTBN_Send_msg(&msg);
                if (ret != FTB_SUCCESS)
                    FTB_WARNING("FTBN_Send_msg failed %d",ret);
            }
            free(comp);
        }

        FTBU_map_finalize(FTBMI_info.peers);
        FTBU_map_finalize(FTBMI_info.catch_event_map);
    }
    else {
        int ret;
        FTBM_msg_t msg;
        FTB_INFO("deregister only from parent");
        memcpy(&msg.src, &FTBMI_info.self, sizeof(FTB_id_t));
        memcpy(&msg.dst.location_id, &FTBMI_info.parent, sizeof(FTB_location_id_t));
        msg.msg_type =  FTBM_MSG_TYPE_CLIENT_DEREG;
        ret = FTBN_Send_msg(&msg);
        if (ret != FTB_SUCCESS)
            FTB_WARNING("FTBN_Send_msg failed %d",ret);
    }
    FTBN_Finalize();
    unlock_manager();
    FTB_INFO("FTBM_Finalize Out");
    return FTB_SUCCESS;
}

int FTBM_Abort()
{
    FTB_INFO("FTBM_Abort In");
    if (!FTBMI_initialized) {
        FTB_INFO("FTBM_Abort Out");
        return FTB_SUCCESS;
    }
    FTBMI_initialized = 0;

    if (!FTBMI_info.leaf) {
        FTBU_map_iterator_t iter = FTBU_map_begin(FTBMI_info.peers);
        for (;iter!=FTBU_map_end(FTBMI_info.peers);iter=FTBU_map_next_iterator(iter)) {
            FTBMI_comp_info_t* comp = (FTBMI_comp_info_t*)FTBU_map_get_data(iter);
            free(comp);
        }

        FTBU_map_finalize(FTBMI_info.peers);
        FTBU_map_finalize(FTBMI_info.catch_event_map);
    }

    FTBN_Abort();
    FTB_INFO("FTBM_Abort Out");
    return FTB_SUCCESS;
}

int FTBM_Client_register(const FTB_id_t *id)
{
    FTBMI_comp_info_t *comp;
    int ret;
    if (!FTBMI_initialized) 
        return FTB_ERR_GENERAL;
    
    FTB_INFO("FTBM_Client_register In");

    if (FTBMI_info.leaf) {
        FTB_INFO("FTBM_Client_register Out");
        return FTB_ERR_NOT_SUPPORTED;
    }

    comp = (FTBMI_comp_info_t *)malloc(sizeof(FTBMI_comp_info_t));
    memcpy(&comp->id,id,sizeof(FTB_id_t));
    pthread_mutex_init(&comp->lock, NULL);
    comp->catch_event_set = (FTBMI_set_event_mask_t*)FTBU_map_init(FTBMI_util_is_equal_event);

    lock_manager();
    if (FTBU_map_insert(FTBMI_info.peers, FTBU_MAP_PTR_KEY(&comp->id), (void*)comp) == FTBU_EXIST)
    {
        FTB_WARNING("Already registered");
        unlock_manager();
        FTB_INFO("FTBM_Client_register Out");
        return FTB_ERR_INVALID_PARAMETER;
    }
    unlock_manager();

    /*propagate catch info to the new component if it is an agent*/
    if ((strcasecmp(id->client_id.comp_cat, "FTB_COMP_CAT_BACKPLANE") == 0)
            && (strcasecmp(id->client_id.comp, "FTB_COMP_MANAGER") == 0)
            && id->client_id.ext == 0)
    {
        FTBM_msg_t msg;
        memcpy((void*)&msg.src, (void*)&FTBMI_info.self,sizeof(FTB_id_t));
        msg.msg_type =  FTBM_MSG_TYPE_REG_SUBSCRIPTION;
        memcpy(&msg.dst, id,sizeof(FTB_id_t));
        FTBU_map_iterator_t iter;
        for (iter = FTBU_map_begin(FTBMI_info.catch_event_map);
               iter != FTBU_map_end(FTBMI_info.catch_event_map);
               iter = FTBU_map_next_iterator(iter)) 
        {
            FTB_event_t *mask = (FTB_event_t*)FTBU_map_get_key(iter).key_ptr;
            memcpy(&msg.event,mask,sizeof(FTB_event_t));
            ret = FTBN_Send_msg(&msg);
            if (ret != FTB_SUCCESS) {
                FTB_WARNING("FTBN_Send_msg failed");
            }
        }
    }
    FTB_INFO("new client comp:%s comp_cat:%s client_name:%s ext:%d registered, from host %s, pid %d",
        comp->id.client_id.comp, comp->id.client_id.comp_cat, comp->id.client_id.client_name, comp->id.client_id.ext, 
        comp->id.location_id.hostname, comp->id.location_id.pid);

    FTB_INFO("FTBM_Client_register Out");
    return FTB_SUCCESS;
}

   
int FTBM_Client_deregister(const FTB_id_t *id)
{
    FTBMI_comp_info_t *comp;
    int is_parent = 0;
    if (!FTBMI_initialized)
        return FTB_ERR_GENERAL;

    FTB_INFO("FTBM_Client_deregister In");
    if (FTBMI_info.leaf) {
        FTB_INFO("FTBM_Client_deregister Out");
        return FTB_ERR_NOT_SUPPORTED;
    }
    comp = FTBMI_lookup_component(id);
    if (comp == NULL) {
        FTB_INFO("FTBM_Client_deregister Out");
        return FTB_ERR_INVALID_PARAMETER;
    }
    FTB_INFO("client comp:%s comp_cat:%s client_name=%s  ext:%d from host %s pid %d, deregistering", 
        comp->id.client_id.comp, comp->id.client_id.comp_cat, comp->id.client_id.client_name, comp->id.client_id.ext,
        comp->id.location_id.hostname, comp->id.location_id.pid);
    lock_comp(comp);
    lock_manager();
    FTBMI_util_clean_component(comp);
    if ((strcmp(comp->id.client_id.comp_cat, "FTB_COMP_CAT_BACKPLANE") == 0) 
     && (strcmp(comp->id.client_id.comp, "FTB_COMP_MANAGER"))) {
        FTB_INFO("Disconnect component");
        FTBN_Disconnect_peer(&comp->id.location_id);
        if (FTBU_is_equal_location_id(&FTBMI_info.parent, &comp->id.location_id))
            is_parent = 1;
    }
    FTBU_map_remove_key(FTBMI_info.peers, FTBU_MAP_PTR_KEY(&comp->id));
    unlock_manager();
    unlock_comp(comp);
    free(comp);

    if (is_parent) {
        FTB_WARNING("Parent exiting");
        if (FTBMI_info.err_handling & FTB_ERR_HANDLE_RECOVER) {
            FTB_WARNING("Reconnecting");
            lock_manager();
            FTBMI_util_reconnect();
            unlock_manager();
        }
    }
    
    FTB_INFO("FTBM_Client_deregister Out");
    return FTB_SUCCESS;
}

int FTBM_Register_publishable_event(const FTB_id_t *id, FTB_event_t *event)
{
    FTBMI_comp_info_t *comp;
    if (!FTBMI_initialized)
        return FTB_ERR_GENERAL;
    FTB_INFO("FTBM_Register_publishable_event In");
    if (FTBMI_info.leaf) {
        FTB_INFO("FTBM_Register_publishable_event Out");
        return FTB_ERR_NOT_SUPPORTED;
    }
    comp = FTBMI_lookup_component(id);
    if (comp == NULL) {
        FTB_INFO("FTBM_Register_publishable_event Out");
        return FTB_ERR_INVALID_PARAMETER;
    }

    /*Currently not doing anything*/
    
    FTB_INFO("FTBM_Register_publishable_event Out");
    return FTB_SUCCESS;
}

int FTBM_Register_subscription(const FTB_id_t *id, FTB_event_t *event)
{
    FTBU_map_iterator_t iter;
    FTB_event_t *new_mask_comp;
    FTB_event_t *new_mask_manager;
    FTBMI_comp_info_t *comp;
    if (!FTBMI_initialized)
        return FTB_ERR_GENERAL;
    FTB_INFO("FTBM_Register_subscription In");
    if (FTBMI_info.leaf) {
        FTB_INFO("FTBM_Register_subscription Out");
        return FTB_ERR_NOT_SUPPORTED;
    }
    comp = FTBMI_lookup_component(id);
    if (comp == NULL) {
        FTB_INFO("FTBM_Register_subscription Out");
        return FTB_ERR_INVALID_PARAMETER;
    }

    /*Add to component structure*/
    new_mask_comp = (FTB_event_t*)malloc(sizeof(FTB_event_t));
    memcpy(new_mask_comp, event, sizeof(FTB_event_t));
    lock_comp(comp);
    if (FTBU_map_insert(comp->catch_event_set, FTBU_MAP_PTR_KEY(new_mask_comp), (void *)new_mask_comp)==FTBU_EXIST)
    {
        FTB_WARNING("Already registered same event mask");
        free(new_mask_comp);
        FTB_INFO("FTBM_Register_subscription Out");
        return FTB_SUCCESS;
    }

    /*Add to node structure if needed*/
    lock_manager();
    iter = FTBU_map_find(FTBMI_info.catch_event_map, FTBU_MAP_PTR_KEY(new_mask_comp));
    if (iter == FTBU_map_end(FTBMI_info.catch_event_map)) {
        FTBMI_map_ftb_id_2_comp_info_t *new_map;
        FTB_INFO("New event to catch for the manager");
        new_mask_manager = (FTB_event_t*)malloc(sizeof(FTB_event_t));
        memcpy(new_mask_manager,new_mask_comp,sizeof(FTB_event_t));
        new_map = (FTBMI_map_ftb_id_2_comp_info_t *)FTBU_map_init(FTBMI_util_is_equal_ftb_id);
        FTBU_map_insert(FTBMI_info.catch_event_map, FTBU_MAP_PTR_KEY(new_mask_manager), (void*)new_map);
        FTBU_map_insert(new_map, FTBU_MAP_PTR_KEY(&comp->id), (void*)comp);
        /*notify other FTB nodes about catch*/
        FTBMI_util_reg_propagation( FTBM_MSG_TYPE_REG_SUBSCRIPTION, event, &id->location_id);
    }
    else {
        FTBMI_map_ftb_id_2_comp_info_t *map;
        FTB_INFO("The manager is already catching the event, adding the component");
        map = (FTBMI_map_ftb_id_2_comp_info_t *)FTBU_map_get_data(iter);
        FTBU_map_insert(map, FTBU_MAP_PTR_KEY(&comp->id), (void*)comp);
    }
    unlock_manager();
    unlock_comp(comp);

    FTB_INFO("FTBM_Register_subscription Out");

    return FTB_SUCCESS;
}

int FTBM_Publishable_event_registration_cancel(const FTB_id_t *id, FTB_event_t *event)
{
    FTBMI_comp_info_t *comp;
    if (!FTBMI_initialized)
        return FTB_ERR_GENERAL;
    FTB_INFO("FTBM_Publishable_event_registration_cancel In");
    if (FTBMI_info.leaf) {
        FTB_INFO("FTBM_Publishable_event_registration_cancel Out");
        return FTB_ERR_NOT_SUPPORTED;
    }
    comp = FTBMI_lookup_component(id);
    if (comp == NULL) {
        FTB_INFO("FTBM_Publishable_event_registration_cancel Out");
        return FTB_ERR_INVALID_PARAMETER;
    }

    /*Currently not doing anything*/
    
    FTB_INFO("FTBM_Publishable_event_registration_cancel Out");
    return FTB_SUCCESS;
}

int FTBM_Cancel_subscription(const FTB_id_t *id, FTB_event_t *event)
{
    FTB_event_t *mask;
    FTBU_map_iterator_t iter;
    FTBMI_comp_info_t *comp;

    if (!FTBMI_initialized)
        return FTB_ERR_GENERAL;
    FTB_INFO("FTBM_Cancel_subscription In");
    if (FTBMI_info.leaf) {
        FTB_INFO("FTBM_Cancel_subscription Out");
        return FTB_ERR_NOT_SUPPORTED;
    }
    comp = FTBMI_lookup_component(id);
    if (comp == NULL) {
        FTB_INFO("FTBM_Cancel_subscription Out");
        return FTB_ERR_INVALID_PARAMETER;
    }

    lock_comp(comp);
    FTB_INFO("util_reg_catch_cancel");
    iter = FTBU_map_find(comp->catch_event_set, FTBU_MAP_PTR_KEY(event));
    if (iter == FTBU_map_end(comp->catch_event_set))
    {
        FTB_WARNING("Not found throw entry");
        FTB_INFO("FTBM_Cancel_subscription Out");
        return FTB_SUCCESS;
    }
    mask = (FTB_event_t *)FTBU_map_get_data(iter);
    FTBU_map_remove_iter(iter);
    lock_manager();
    FTBMI_util_remove_com_from_catch_map(comp, mask);
    unlock_manager();
    free(mask);
    unlock_comp(comp);
    FTB_INFO("FTBM_Cancel_subscription Out");
    return FTB_SUCCESS;
}

int FTBM_Send(const FTBM_msg_t *msg)
{
    int ret;
    if (!FTBMI_initialized)
        return FTB_ERR_GENERAL;
    FTB_INFO("FTBM_Send In");
    ret = FTBN_Send_msg(msg);
    if (ret != FTB_SUCCESS) {
        if (!FTBMI_info.leaf) { //For an agent
            FTBMI_comp_info_t *comp;
            comp = FTBMI_lookup_component(&msg->dst);
            FTB_INFO("client %s:%s:%d from host %s pid %d, clean up", 
                comp->id.client_id.comp, comp->id.client_id.comp_cat, comp->id.client_id.ext,
                comp->id.location_id.hostname, comp->id.location_id.pid);
            lock_comp(comp);
            lock_manager();
            FTBMI_util_clean_component(comp);
            if ((strcasecmp(comp->id.client_id.comp_cat, "FTB_COMP_CAT_BACKPLANE") == 0) 
                    && (strcasecmp(comp->id.client_id.comp, "FTB_COMP_MANAGER") == 0)){
                FTB_INFO("Disconnect component");
                FTBN_Disconnect_peer(&comp->id.location_id);
            }
            FTBU_map_remove_key(FTBMI_info.peers, FTBU_MAP_PTR_KEY(&comp->id));
            unlock_manager();
            unlock_comp(comp);
            free(comp);
        }
        
        if (FTBU_is_equal_location_id(&FTBMI_info.parent, &msg->dst.location_id)) {
            FTB_WARNING("Lost connection to parent");
            if (FTBMI_info.err_handling & FTB_ERR_HANDLE_RECOVER) {
                FTB_WARNING("Reconnecting");
                lock_manager();
                FTBMI_util_reconnect();
                unlock_manager();
            }
        }
    }
    FTB_INFO("FTBM_Send Out");
    return ret;
}

int FTBM_Poll(FTBM_msg_t *msg, FTB_location_id_t *incoming_src)
{
    int ret;
    if (!FTBMI_initialized)
        return FTB_ERR_GENERAL;
    FTB_INFO("FTBM_Poll In");
    lock_network();
    if (FTBMI_info.waiting) {
        unlock_network();
        return FTBM_NO_MSG;
    }
    ret = FTBN_Recv_msg(msg, incoming_src, 0);
    unlock_network();
    if (ret == FTBN_NO_MSG)
        return FTBM_NO_MSG;
    if (ret != FTB_SUCCESS) {
        FTB_WARNING("FTBN_Recv_msg failed %d",ret);
    }
    FTB_INFO("FTBM_Poll Out");
    return ret;
}

int FTBM_Wait(FTBM_msg_t *msg, FTB_location_id_t *incoming_src)
{
    int ret;
    if (!FTBMI_initialized)
        return FTB_ERR_GENERAL;
    FTB_INFO("FTBM_Wait In");
    lock_network();
    FTBMI_info.waiting = 1;
    unlock_network();
    ret = FTBN_Recv_msg(msg, incoming_src, 1);
    if (ret != FTB_SUCCESS) {
        FTB_WARNING("FTBN_Recv_msg failed %d",ret);
    }
    lock_network();
    FTBMI_info.waiting = 0;
    unlock_network();
    FTB_INFO("FTBM_Wait Out");
    return ret;
}

