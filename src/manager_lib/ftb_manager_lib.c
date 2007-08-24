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

typedef FTBU_map_node_t FTBM_set_event_mask_t; /*a set of event_mask*/

typedef struct FTBM_comp_info{
    FTB_id_t id;
    pthread_mutex_t lock;
    FTBM_set_event_mask_t* catch_event_set;
}FTBM_comp_info_t;

typedef FTBU_map_node_t FTBM_map_ftb_id_2_comp_info_t; /*ftb_id as key and comp_info as data*/
typedef FTBU_map_node_t FTBM_map_event_mask_2_comp_info_map_t; /*event_mask as key, a _map_ of comp_info as data*/

typedef struct FTBM_node_info{
    FTB_location_id_t parent; /*NULL if root*/
    FTB_id_t self;
    FTB_err_handling_t err_handling;
    int leaf;
    volatile int waiting;
    FTBM_map_ftb_id_2_comp_info_t *peers; /*the map of peers includes parent*/
    FTBM_map_event_mask_2_comp_info_map_t *catch_event_map; 
}FTBM_node_info_t;

static pthread_mutex_t FTBM_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t FTBN_lock = PTHREAD_MUTEX_INITIALIZER;
static FTBM_node_info_t FTBM_info;
static volatile int FTBM_initialized = 0;

static inline void lock_manager()
{
    pthread_mutex_lock(&FTBM_lock);
}

static inline void unlock_manager()
{
    pthread_mutex_unlock(&FTBM_lock);
}

static inline void lock_network()
{
    pthread_mutex_lock(&FTBN_lock);
}

static inline void unlock_network()
{
    pthread_mutex_unlock(&FTBN_lock);
}

static inline void lock_comp(FTBM_comp_info_t *comp_info)
{
    pthread_mutex_lock(&comp_info->lock);
}

static inline void unlock_comp(FTBM_comp_info_t *comp_info)
{
    pthread_mutex_unlock(&comp_info->lock);
}


int FTBM_util_is_equal_ftb_id(const void* lhs_void, const void* rhs_void)
{
    FTB_id_t *lhs = (FTB_id_t *)lhs_void;
    FTB_id_t *rhs = (FTB_id_t *)rhs_void;
    return FTBU_is_equal_ftb_id(lhs, rhs);
}


int FTBM_util_is_equal_event(const void * lhs_void, const void * rhs_void)
{
    FTB_event_t *lhs = (FTB_event_t *)lhs_void;
    FTB_event_t *rhs = (FTB_event_t *)rhs_void;
    return FTBU_is_equal_event(lhs, rhs);
}

static void util_reg_propagation(int msg_type, const FTB_event_t *event, const FTB_location_id_t *incoming)
{
    FTBM_msg_t msg;
    int ret;
    FTBU_map_iterator_t iter_comp;

    msg.msg_type = msg_type;
    memcpy(&msg.src, &FTBM_info.self, sizeof(FTB_id_t));
    memcpy(&msg.event,event,sizeof(FTB_event_t));
    for (iter_comp = FTBU_map_begin(FTBM_info.peers);
           iter_comp != FTBU_map_end(FTBM_info.peers);
           iter_comp = FTBU_map_next_iterator(iter_comp)) 
    {
        FTB_id_t *id = (FTB_id_t *)FTBU_map_get_key(iter_comp).key_ptr;
        /*Propagation to other FTB agents but not the source*/
        if (!(id->client_id.comp_ctgy == FTB_COMP_CTGY_BACKPLANE && id->client_id.comp == FTB_COMP_MANAGER && id->client_id.ext == 0)
          || FTBU_is_equal_location_id(&id->location_id,incoming))
            continue;
        memcpy(&msg.dst,id,sizeof(FTB_id_t));
        ret = FTBN_Send_msg(&msg);
        if (ret != FTB_SUCCESS) {
            FTB_WARNING("FTBN_Send_msg failed");
        }
    }
}

static void util_remove_com_from_catch_map(const FTBM_comp_info_t *comp, const FTB_event_t *mask)
{
    int ret;
    FTBM_map_ftb_id_2_comp_info_t *map;
    FTBU_map_iterator_t iter;
    FTB_event_t *mask_manager;
    iter = FTBU_map_find(FTBM_info.catch_event_map, FTBU_MAP_PTR_KEY(mask));
    if (iter == FTBU_map_end(FTBM_info.catch_event_map)) {
        FTB_WARNING("not found component map");
        return;
    }
    mask_manager = (FTB_event_t*)FTBU_map_get_key(iter).key_ptr;
    map = (FTBM_map_ftb_id_2_comp_info_t *)FTBU_map_get_data(iter);
    ret = FTBU_map_remove_key(map, FTBU_MAP_PTR_KEY(&comp->id));
    if (ret == FTBU_NOT_EXIST) {
        FTB_WARNING("not found component");
        return;
    }
    if (FTBU_map_is_empty(map)) {
        FTBU_map_finalize(map);
        FTBU_map_remove_iter(iter);
        /*Cancel catch mask to other ftb cores*/
        util_reg_propagation(FTBM_MSG_TYPE_REG_CATCH_CANCEL, mask_manager, &comp->id.location_id);
        free(mask_manager);
    }
}

static void util_clean_component(FTBM_comp_info_t *comp)
{/*assumes it has the lock of component*/
    /*remove it from catch map*/
    FTBU_map_iterator_t iter = FTBU_map_begin(comp->catch_event_set);
    for (;iter != FTBU_map_end(comp->catch_event_set); iter=FTBU_map_next_iterator(iter))
    {
        FTB_event_t *mask = (FTB_event_t *)FTBU_map_get_data(iter);
        util_remove_com_from_catch_map(comp,mask);
        free(mask);
    }
    /*Finalize comp's catch set*/
    FTBU_map_finalize(comp->catch_event_set);
}

static inline FTBM_comp_info_t *lookup_component(const FTB_id_t *id) 
{
    FTBM_comp_info_t *comp = NULL;
    FTBU_map_iterator_t iter;
    lock_manager();
    iter = FTBU_map_find(FTBM_info.peers, FTBU_MAP_PTR_KEY(id));
    if (iter != FTBU_map_end(FTBM_info.peers)) {
        comp = (FTBM_comp_info_t *)FTBU_map_get_data(iter);
    }
    unlock_manager();
    return comp;
}

int FTBM_Get_catcher_comp_list(const FTB_event_t *event, FTB_id_t **list, int *len)
{
    FTBU_map_iterator_t iter_comp;
    FTBU_map_iterator_t iter_mask;
    FTBM_map_ftb_id_2_comp_info_t* catcher_set;
    int temp_len=0;
    
    /*Contruct a deliver set to keep track which components have already gotten the event and which are not, to avoid duplication*/
    FTB_INFO("FTBM_Get_catcher_comp_list In");
    catcher_set = (FTBM_map_ftb_id_2_comp_info_t*)FTBU_map_init(FTBM_util_is_equal_ftb_id);

    lock_manager();
    for (iter_mask=FTBU_map_begin(FTBM_info.catch_event_map);
           iter_mask!=FTBU_map_end(FTBM_info.catch_event_map);
           iter_mask=FTBU_map_next_iterator(iter_mask))
    {
        FTB_event_t *mask = (FTB_event_t *)FTBU_map_get_key(iter_mask).key_ptr;
        if (FTBU_match_mask(event, mask)) {
            FTB_INFO("Get the map of components");
            FTBU_map_node_t *map = (FTBU_map_node_t *)FTBU_map_get_data(iter_mask);
            for (iter_comp=FTBU_map_begin(map);
                   iter_comp!=FTBU_map_end(map);
                   iter_comp=FTBU_map_next_iterator(iter_comp)){
                FTBM_comp_info_t *comp = (FTBM_comp_info_t *)FTBU_map_get_data(iter_comp);
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

static void util_get_system_id(uint32_t *system_id)
{
    *system_id = 1;
}

static void util_get_location_id(FTB_location_id_t *location_id)
{
    location_id->pid = getpid();
    FTBU_get_output_of_cmd("hostname", location_id->hostname, FTB_MAX_HOST_NAME);
}

int FTBM_Get_location_id(FTB_location_id_t *location_id)
{
    if (!FTBM_initialized)
        return FTB_ERR_GENERAL;
    memcpy(location_id,&FTBM_info.self.location_id,sizeof(FTB_location_id_t));
    return FTB_SUCCESS;
}

int FTBM_Get_parent_location_id(FTB_location_id_t *location_id)
{
    if (!FTBM_initialized)
        return FTB_ERR_GENERAL;
    memcpy(location_id,&FTBM_info.parent,sizeof(FTB_location_id_t));
    return FTB_SUCCESS;
}

int FTBM_Init(int leaf)
{
    FTBN_config_info_t config;
    FTBM_msg_t msg;

    FTB_INFO("FTBM_Init In");

    FTBM_info.leaf = leaf;
    if (!leaf) {
        FTBM_info.peers = 
            (FTBM_map_ftb_id_2_comp_info_t*) FTBU_map_init(FTBM_util_is_equal_ftb_id);
        FTBM_info.catch_event_map = 
            (FTBM_map_event_mask_2_comp_info_map_t *)FTBU_map_init(FTBM_util_is_equal_event);
        FTBM_info.err_handling = FTB_ERR_HANDLE_RECOVER;
    }
    else {
        FTBM_info.peers = NULL;
        FTBM_info.catch_event_map = NULL;
        FTBM_info.err_handling = FTB_ERR_HANDLE_NONE; /*May change to recover mode if one client component requires so*/
    }

    config.leaf = leaf;
    util_get_system_id(&config.FTB_system_id);
    util_get_location_id(&FTBM_info.self.location_id);
    FTBM_info.self.client_id.comp = FTB_COMP_MANAGER;
    FTBM_info.self.client_id.comp_ctgy = FTB_COMP_CTGY_BACKPLANE;
    FTBM_info.self.client_id.ext = leaf;
    FTBN_Init(&FTBM_info.self.location_id, &config);
    memcpy(&msg.src, &FTBM_info.self, sizeof(FTB_id_t));
    msg.msg_type = FTBM_MSG_TYPE_COMP_REG;
    FTBN_Connect(&msg, &FTBM_info.parent);
    if (leaf && FTBM_info.parent.pid == 0) {
        FTB_WARNING("Can not connect to any ftb agent");
        FTB_INFO("FTBM_Init Out");
        /*TODO: add the check to let FTBM do nothing if not initialized*/
        return FTB_ERR_GENERAL;
    }
    
    if (!leaf && FTBM_info.parent.pid != 0) {
        FTBM_comp_info_t *comp;
        FTB_INFO("Adding parent to peers");
        comp = (FTBM_comp_info_t *)malloc(sizeof(FTBM_comp_info_t));
        memcpy(&comp->id.location_id, &FTBM_info.parent, sizeof(FTB_location_id_t));
        comp->id.client_id.comp_ctgy = FTB_COMP_CTGY_BACKPLANE;
        comp->id.client_id.comp = FTB_COMP_MANAGER;
        comp->id.client_id.ext = 0;
        pthread_mutex_init(&comp->lock, NULL);
        comp->catch_event_set = (FTBM_set_event_mask_t*)FTBU_map_init(FTBM_util_is_equal_event);

        if (FTBU_map_insert(FTBM_info.peers, FTBU_MAP_PTR_KEY(&comp->id), (void*)comp) == FTBU_EXIST)
        {
            return FTB_ERR_GENERAL;
        }
    }

    FTBM_info.waiting = 0;
    lock_manager();
    FTBM_initialized = 1;
    unlock_manager();
    FTB_INFO("FTBM_Init Out");
    return FTB_SUCCESS;
}

/*Called when the ftb node get finalized*/
int FTBM_Finalize()
{
    FTB_INFO("FTBM_Finalize In");
    if (!FTBM_initialized) {
        FTB_INFO("FTBM_Finalize Out");
        return FTB_SUCCESS;
    }

    lock_manager();
    FTBM_initialized = 0;
    
    if (!FTBM_info.leaf) {
        FTBU_map_iterator_t iter = FTBU_map_begin(FTBM_info.peers);
        for (;iter!=FTBU_map_end(FTBM_info.peers);iter=FTBU_map_next_iterator(iter)) {
            FTBM_comp_info_t* comp = (FTBM_comp_info_t*)FTBU_map_get_data(iter);
            util_clean_component(comp);
            if (comp->id.client_id.comp_ctgy == FTB_COMP_CTGY_BACKPLANE
            &&  comp->id.client_id.comp == FTB_COMP_MANAGER
            &&  comp->id.client_id.ext == 0) {
                int ret;
                FTBM_msg_t msg;
                FTB_INFO("deregister from peer");
                memcpy(&msg.src, &FTBM_info.self, sizeof(FTB_id_t));
                memcpy(&msg.dst, &comp->id, sizeof(FTB_id_t));
                msg.msg_type = FTBM_MSG_TYPE_COMP_DEREG;
                ret = FTBN_Send_msg(&msg);
                if (ret != FTB_SUCCESS)
                    FTB_WARNING("FTBN_Send_msg failed %d",ret);
            }
            free(comp);
        }

        FTBU_map_finalize(FTBM_info.peers);
        FTBU_map_finalize(FTBM_info.catch_event_map);
    }
    else {
        int ret;
        FTBM_msg_t msg;
        FTB_INFO("deregister only from parent");
        memcpy(&msg.src, &FTBM_info.self, sizeof(FTB_id_t));
        memcpy(&msg.dst.location_id, &FTBM_info.parent, sizeof(FTB_location_id_t));
        msg.msg_type = FTBM_MSG_TYPE_COMP_DEREG;
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
    if (!FTBM_initialized) {
        FTB_INFO("FTBM_Abort Out");
        return FTB_SUCCESS;
    }

    if (!FTBM_info.leaf) {
        /*This part needs connections to other peers*/
        FTBU_map_iterator_t iter = FTBU_map_begin(FTBM_info.peers);
        for (;iter!=FTBU_map_end(FTBM_info.peers);iter=FTBU_map_next_iterator(iter)) {
            FTBM_comp_info_t* comp = (FTBM_comp_info_t*)FTBU_map_get_data(iter);
            util_clean_component(comp);
            free(comp);
        }

        FTBU_map_finalize(FTBM_info.peers);
        FTBU_map_finalize(FTBM_info.catch_event_map);
    }

    FTBN_Abort();
    FTB_INFO("FTBM_Abort Out");
    return FTB_SUCCESS;
}

int FTBM_Component_reg(const FTB_id_t *id)
{
    FTBM_comp_info_t *comp;
    int ret;
    if (!FTBM_initialized)
        return FTB_ERR_GENERAL;
    
    FTB_INFO("FTBM_Component_reg In");

    if (FTBM_info.leaf) {
        FTB_INFO("FTBM_Component_reg Out");
        return FTB_ERR_NOT_SUPPORTED;
    }

    comp = (FTBM_comp_info_t *)malloc(sizeof(FTBM_comp_info_t));
    memcpy(&comp->id,id,sizeof(FTB_id_t));
    pthread_mutex_init(&comp->lock, NULL);
    comp->catch_event_set = (FTBM_set_event_mask_t*)FTBU_map_init(FTBM_util_is_equal_event);

    lock_manager();
    if (FTBU_map_insert(FTBM_info.peers, FTBU_MAP_PTR_KEY(&comp->id), (void*)comp) == FTBU_EXIST)
    {
        FTB_WARNING("Already registered");
        unlock_manager();
        FTB_INFO("FTBM_Component_reg Out");
        return FTB_ERR_INVALID_PARAMETER;
    }
    unlock_manager();

    /*propagate catch info to the new component if it is an agent*/
    if (id->client_id.comp_ctgy == FTB_COMP_CTGY_BACKPLANE 
     && id->client_id.comp == FTB_COMP_MANAGER
     && id->client_id.ext == 0)
    {
        FTBM_msg_t msg;
        memcpy((void*)&msg.src, (void*)&FTBM_info.self,sizeof(FTB_id_t));
        msg.msg_type = FTBM_MSG_TYPE_REG_CATCH;
        memcpy(&msg.dst, id,sizeof(FTB_id_t));
        FTBU_map_iterator_t iter;
        for (iter = FTBU_map_begin(FTBM_info.catch_event_map);
               iter != FTBU_map_end(FTBM_info.catch_event_map);
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
    FTB_INFO("new client %d:%d:%d registered, from host %s, pid %d",
        comp->id.client_id.comp, comp->id.client_id.comp_ctgy, comp->id.client_id.ext, 
        comp->id.location_id.hostname, comp->id.location_id.pid);

    FTB_INFO("FTBM_Component_reg Out");
    return FTB_SUCCESS;
}

static void util_reconnect()
{
    FTBM_msg_t msg;
    int ret;
    memcpy(&msg.src, &FTBM_info.self, sizeof(FTB_id_t));
    msg.msg_type = FTBM_MSG_TYPE_COMP_REG;
    FTBN_Connect(&msg, &FTBM_info.parent);
    if (FTBM_info.leaf && FTBM_info.parent.pid == 0) {
        FTB_WARNING("Can not connect to any ftb agent");
        FTBM_initialized = 0;
        return;
    }
    
    if (!FTBM_info.leaf && FTBM_info.parent.pid != 0) {
        FTBM_comp_info_t *comp;
        FTBU_map_iterator_t iter;
        FTB_INFO("Adding parent to peers");
        comp = (FTBM_comp_info_t *)malloc(sizeof(FTBM_comp_info_t));
        memcpy(&comp->id.location_id, &FTBM_info.parent, sizeof(FTB_location_id_t));
        comp->id.client_id.comp_ctgy = FTB_COMP_CTGY_BACKPLANE;
        comp->id.client_id.comp = FTB_COMP_MANAGER;
        comp->id.client_id.ext = 0;
        pthread_mutex_init(&comp->lock, NULL);
        comp->catch_event_set = (FTBM_set_event_mask_t*)FTBU_map_init(FTBM_util_is_equal_event);

        if (FTBU_map_insert(FTBM_info.peers, FTBU_MAP_PTR_KEY(&comp->id), (void*)comp) == FTBU_EXIST)
        {
            return;
        }

        FTB_INFO("Register all catches");
        memcpy((void*)&msg.src, (void*)&FTBM_info.self,sizeof(FTB_id_t));
        msg.msg_type = FTBM_MSG_TYPE_REG_CATCH;
        memcpy(&msg.dst, &comp->id, sizeof(FTB_id_t));
        for (iter = FTBU_map_begin(FTBM_info.catch_event_map);
               iter != FTBU_map_end(FTBM_info.catch_event_map);
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
   
int FTBM_Component_dereg(const FTB_id_t *id)
{
    FTBM_comp_info_t *comp;
    int is_parent = 0;
    if (!FTBM_initialized)
        return FTB_ERR_GENERAL;

    FTB_INFO("FTBM_Component_dereg In");
    if (FTBM_info.leaf) {
        FTB_INFO("FTBM_Component_dereg Out");
        return FTB_ERR_NOT_SUPPORTED;
    }
    comp = lookup_component(id);
    if (comp == NULL) {
        FTB_INFO("FTBM_Component_dereg Out");
        return FTB_ERR_INVALID_PARAMETER;
    }
    FTB_INFO("client %d:%d:%d from host %s pid %d, deregistering", 
        comp->id.client_id.comp, comp->id.client_id.comp_ctgy, comp->id.client_id.ext,
        comp->id.location_id.hostname, comp->id.location_id.pid);
    lock_comp(comp);
    lock_manager();
    util_clean_component(comp);
    if (comp->id.client_id.comp_ctgy == FTB_COMP_CTGY_BACKPLANE 
     && comp->id.client_id.comp == FTB_COMP_MANAGER) {
        FTB_INFO("Disconnect component");
        FTBN_Disconnect_peer(&comp->id.location_id);
        if (FTBU_is_equal_location_id(&FTBM_info.parent, &comp->id.location_id))
            is_parent = 1;
    }
    FTBU_map_remove_key(FTBM_info.peers, FTBU_MAP_PTR_KEY(&comp->id));
    unlock_manager();
    unlock_comp(comp);
    free(comp);

    if (is_parent) {
        FTB_WARNING("Parent exiting");
        if (FTBM_info.err_handling & FTB_ERR_HANDLE_RECOVER) {
            FTB_WARNING("Reconnecting");
            lock_manager();
            util_reconnect();
            unlock_manager();
        }
    }
    
    FTB_INFO("FTBM_Component_dereg Out");
    return FTB_SUCCESS;
}

int FTBM_Reg_throw(const FTB_id_t *id, FTB_event_t *event)
{
    FTBM_comp_info_t *comp;
    if (!FTBM_initialized)
        return FTB_ERR_GENERAL;
    FTB_INFO("FTBM_Reg_throw In");
    if (FTBM_info.leaf) {
        FTB_INFO("FTBM_Reg_throw Out");
        return FTB_ERR_NOT_SUPPORTED;
    }
    comp = lookup_component(id);
    if (comp == NULL) {
        FTB_INFO("FTBM_Reg_throw Out");
        return FTB_ERR_INVALID_PARAMETER;
    }

    /*Currently not doing anything*/
    
    FTB_INFO("FTBM_Reg_throw Out");
    return FTB_SUCCESS;
}

int FTBM_Reg_catch(const FTB_id_t *id, FTB_event_t *event)
{
    FTBU_map_iterator_t iter;
    FTB_event_t *new_mask_comp;
    FTB_event_t *new_mask_manager;
    FTBM_comp_info_t *comp;
    if (!FTBM_initialized)
        return FTB_ERR_GENERAL;
    FTB_INFO("FTBM_Reg_catch In");
    if (FTBM_info.leaf) {
        FTB_INFO("FTBM_Reg_catch Out");
        return FTB_ERR_NOT_SUPPORTED;
    }
    comp = lookup_component(id);
    if (comp == NULL) {
        FTB_INFO("FTBM_Reg_catch Out");
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
        FTB_INFO("FTBM_Reg_catch Out");
        return FTB_SUCCESS;
    }

    /*Add to node structure if needed*/
    lock_manager();
    iter = FTBU_map_find(FTBM_info.catch_event_map, FTBU_MAP_PTR_KEY(new_mask_comp));
    if (iter == FTBU_map_end(FTBM_info.catch_event_map)) {
        FTBM_map_ftb_id_2_comp_info_t *new_map;
        FTB_INFO("New event to catch for the manager");
        new_mask_manager = (FTB_event_t*)malloc(sizeof(FTB_event_t));
        memcpy(new_mask_manager,new_mask_comp,sizeof(FTB_event_t));
        new_map = (FTBM_map_ftb_id_2_comp_info_t *)FTBU_map_init(FTBM_util_is_equal_ftb_id);
        FTBU_map_insert(FTBM_info.catch_event_map, FTBU_MAP_PTR_KEY(new_mask_manager), (void*)new_map);
        FTBU_map_insert(new_map, FTBU_MAP_PTR_KEY(&comp->id), (void*)comp);
        /*notify other FTB nodes about catch*/
        util_reg_propagation(FTBM_MSG_TYPE_REG_CATCH, event, &id->location_id);
    }
    else {
        FTBM_map_ftb_id_2_comp_info_t *map;
        FTB_INFO("The manager is already catching the event, adding the component");
        map = (FTBM_map_ftb_id_2_comp_info_t *)FTBU_map_get_data(iter);
        FTBU_map_insert(map, FTBU_MAP_PTR_KEY(&comp->id), (void*)comp);
    }
    unlock_manager();
    unlock_comp(comp);

    FTB_INFO("FTBM_Reg_catch Out");

    return FTB_SUCCESS;
}

int FTBM_Reg_throw_cancel(const FTB_id_t *id, FTB_event_t *event)
{
    FTBM_comp_info_t *comp;
    if (!FTBM_initialized)
        return FTB_ERR_GENERAL;
    FTB_INFO("FTBM_Reg_throw_cancel In");
    if (FTBM_info.leaf) {
        FTB_INFO("FTBM_Reg_throw_cancel Out");
        return FTB_ERR_NOT_SUPPORTED;
    }
    comp = lookup_component(id);
    if (comp == NULL) {
        FTB_INFO("FTBM_Reg_throw_cancel Out");
        return FTB_ERR_INVALID_PARAMETER;
    }

    /*Currently not doing anything*/
    
    FTB_INFO("FTBM_Reg_throw_cancel Out");
    return FTB_SUCCESS;
}

int FTBM_Reg_catch_cancel(const FTB_id_t *id, FTB_event_t *event)
{
    FTB_event_t *mask;
    FTBU_map_iterator_t iter;
    FTBM_comp_info_t *comp;

    if (!FTBM_initialized)
        return FTB_ERR_GENERAL;
    FTB_INFO("FTBM_Reg_catch_cancel In");
    if (FTBM_info.leaf) {
        FTB_INFO("FTBM_Reg_catch_cancel Out");
        return FTB_ERR_NOT_SUPPORTED;
    }
    comp = lookup_component(id);
    if (comp == NULL) {
        FTB_INFO("FTBM_Reg_catch_cancel Out");
        return FTB_ERR_INVALID_PARAMETER;
    }

    lock_comp(comp);
    FTB_INFO("util_reg_catch_cancel");
    iter = FTBU_map_find(comp->catch_event_set, FTBU_MAP_PTR_KEY(event));
    if (iter == FTBU_map_end(comp->catch_event_set))
    {
        FTB_WARNING("Not found throw entry");
        FTB_INFO("FTBM_Reg_catch_cancel Out");
        return FTB_SUCCESS;
    }
    mask = (FTB_event_t *)FTBU_map_get_data(iter);
    FTBU_map_remove_iter(iter);
    lock_manager();
    util_remove_com_from_catch_map(comp, mask);
    unlock_manager();
    free(mask);
    unlock_comp(comp);
    FTB_INFO("FTBM_Reg_catch_cancel Out");
    return FTB_SUCCESS;
}

int FTBM_Send(const FTBM_msg_t *msg)
{
    int ret;
    if (!FTBM_initialized)
        return FTB_ERR_GENERAL;
    FTB_INFO("FTBM_Send In");
    ret = FTBN_Send_msg(msg);
    if (ret != FTB_SUCCESS) {
        if (!FTBM_info.leaf) {
            FTBM_comp_info_t *comp;
            comp = lookup_component(&msg->dst);
            FTB_INFO("client %d:%d:%d from host %s pid %d, clean up", 
                comp->id.client_id.comp, comp->id.client_id.comp_ctgy, comp->id.client_id.ext,
                comp->id.location_id.hostname, comp->id.location_id.pid);
            lock_comp(comp);
            lock_manager();
            util_clean_component(comp);
            if (comp->id.client_id.comp_ctgy == FTB_COMP_CTGY_BACKPLANE 
             && comp->id.client_id.comp == FTB_COMP_MANAGER) {
                FTB_INFO("Disconnect component");
                FTBN_Disconnect_peer(&comp->id.location_id);
            }
            FTBU_map_remove_key(FTBM_info.peers, FTBU_MAP_PTR_KEY(&comp->id));
            unlock_manager();
            unlock_comp(comp);
            free(comp);
        }
        
        if (FTBU_is_equal_location_id(&FTBM_info.parent, &msg->dst.location_id)) {
            FTB_WARNING("Lost connection to parent");
            if (FTBM_info.err_handling & FTB_ERR_HANDLE_RECOVER) {
                FTB_WARNING("Reconnecting");
                lock_manager();
                util_reconnect();
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
    if (!FTBM_initialized)
        return FTB_ERR_GENERAL;
    FTB_INFO("FTBM_Poll In");
    lock_network();
    if (FTBM_info.waiting) {
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
    if (!FTBM_initialized)
        return FTB_ERR_GENERAL;
    FTB_INFO("FTBM_Wait In");
    lock_network();
    FTBM_info.waiting = 1;
    unlock_network();
    ret = FTBN_Recv_msg(msg, incoming_src, 1);
    if (ret != FTB_SUCCESS) {
        FTB_WARNING("FTBN_Recv_msg failed %d",ret);
    }
    lock_network();
    FTBM_info.waiting = 0;
    unlock_network();
    FTB_INFO("FTBM_Wait Out");
    return ret;
}

