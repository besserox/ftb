/***********************************************************************************/
/* FTB:ftb-info */
/* This file is part of FTB (Fault Tolerance Backplance) - the core of CIFTS
 * (Co-ordinated Infrastructure for Fault Tolerant Systems)
 *
 * See http://www.mcs.anl.gov/research/cifts for more information.
 * 	
 */
/* FTB:ftb-info */

/* FTB:ftb-fillin */
/* FTB_Version: 0.6.2
 * FTB_API_Version: 0.5
 * FTB_Heredity:FOSS_ORIG
 * FTB_License:BSD
 */
/* FTB:ftb-fillin */

/* FTB:ftb-bsd */
/* This software is licensed under BSD. See the file FTB/misc/license.BSD for
 * complete details on your rights to copy, modify, and use this software.
 */
/* FTB:ftb-bsd */
/***********************************************************************************/

#include <stdio.h>
#include <errno.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <pthread.h>
#include <string.h>
#include <ctype.h>

#include "ftb_util.h"
#include "ftbm.h"
#include "ftb_network.h"

extern FILE *FTBU_log_file_fp;

/* The variable FTBU_map_node_t is redefined to the following for code clarity sake */
typedef FTBU_map_node_t FTBMI_set_event_mask_t; /*a set of event_mask */
typedef FTBU_map_node_t FTBMI_map_ftb_id_to_comp_info_t; /*ftb_id as key and comp_info as data */
typedef FTBU_map_node_t FTBMI_map_subscription_mask_to_comp_info_map_t; /*event_mask as key, a _map_ of comp_info as data */

typedef struct FTBMI_comp_info {
    FTB_id_t id;
    pthread_mutex_t lock;
    FTBMI_set_event_mask_t *catch_event_set;
} FTBMI_comp_info_t;



typedef struct FTBM_node_info {
    FTB_location_id_t parent;   /*NULL if root */
    FTB_id_t self;
    uint8_t err_handling;
    int leaf;
    volatile int waiting;
    FTBMI_map_ftb_id_to_comp_info_t *peers;  /*the map of peers includes parent */
    FTBMI_map_subscription_mask_to_comp_info_map_t *catch_event_map;
} FTBMI_node_info_t;

static pthread_mutex_t FTBMI_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t FTBN_lock = PTHREAD_MUTEX_INITIALIZER;
static FTBMI_node_info_t FTBMI_info;
static volatile int FTBMI_initialized = 0;

pthread_mutex_t message_queue_mutex;
pthread_cond_t message_queue_cond;
FTBM_msg_node_t *message_queue_head = NULL;
FTBM_msg_node_t *message_queue_tail = NULL;


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

static inline void lock_comp(FTBMI_comp_info_t * comp_info)
{
    pthread_mutex_lock(&comp_info->lock);
}

static inline void unlock_comp(FTBMI_comp_info_t * comp_info)
{
    pthread_mutex_unlock(&comp_info->lock);
}


int FTBMI_util_is_equal_ftb_id(const void *lhs_void, const void *rhs_void)
{
    FTB_id_t *lhs = (FTB_id_t *) lhs_void;
    FTB_id_t *rhs = (FTB_id_t *) rhs_void;
    return FTBU_is_equal_ftb_id(lhs, rhs);
}


int FTBMI_util_is_equal_event(const void *lhs_void, const void *rhs_void)
{
    FTB_event_t *lhs = (FTB_event_t *) lhs_void;
    FTB_event_t *rhs = (FTB_event_t *) rhs_void;
    return FTBU_is_equal_event(lhs, rhs);
}

int FTBMI_util_is_equal_event_mask(const void *lhs_void, const void *rhs_void)
{
    FTB_event_t *lhs = (FTB_event_t *) lhs_void;
    FTB_event_t *rhs = (FTB_event_t *) rhs_void;
    return FTBU_is_equal_event_mask(lhs, rhs);
}


/*
 * This routine is used by the manager library to propagate registration information to other agents.
 * More specifically, an agent propagates the message to its peers in the topology
 */
static void FTBMI_util_reg_propagation(int msg_type, const FTB_event_t * event,
                                       const FTB_location_id_t * incoming)
{
    FTBM_msg_t msg;
    int ret;
    FTBU_map_node_t *iter_comp;

    FTBU_INFO("In FTBMI_util_reg_propagation with msg_type = %d\n", msg_type);

    msg.msg_type = msg_type;
    memcpy(&msg.src, &FTBMI_info.self, sizeof(FTB_id_t));
    memcpy(&msg.event, event, sizeof(FTB_event_t));

    for (iter_comp = FTBU_map_begin(FTBMI_info.peers); iter_comp != FTBU_map_end(FTBMI_info.peers);
         iter_comp = FTBU_map_next_node(iter_comp)) {
        FTB_id_t *id = (FTB_id_t *) FTBU_map_get_key(iter_comp).key_ptr;

        /* Propagation to other FTB agents but not the source */
        if (!((strcasecmp(id->client_id.comp_cat, "FTB_COMP_CAT_BACKPLANE") == 0)
              && (strcasecmp(id->client_id.comp, "FTB_COMP_MANAGER") == 0)
              && (id->client_id.ext == 0))
            || FTBU_is_equal_location_id(&id->location_id, incoming)) {
            continue;
        }

        memcpy(&msg.dst, id, sizeof(FTB_id_t));
        FTBU_INFO("FTBMI_util_reg_propagation - Sending msg type=%d to destination %s\n", msg.msg_type,
                  msg.dst.location_id.hostname);
        ret = FTBN_Send_msg(&msg);
        if (ret != FTB_SUCCESS) {
            FTBU_INFO("Out FTBMI_util_reg_propagation\n");
            FTBU_WARNING("FTBN_Send_msg failed");
        }
    }
    FTBU_INFO("Out FTBMI_util_reg_propagation\n");
}


static void FTBMI_util_remove_com_from_catch_map(const FTBMI_comp_info_t * comp,
                                                 const FTB_event_t * mask)
{
    int ret;
    FTBMI_map_ftb_id_to_comp_info_t *map;
    FTBU_map_node_t *iter;
    FTB_event_t *mask_manager;

    FTBU_INFO("In FTBMI_util_remove_com_from_catch_map");
    iter = FTBU_map_find_key(FTBMI_info.catch_event_map, FTBU_MAP_PTR_KEY(mask));
    if (iter == FTBU_map_end(FTBMI_info.catch_event_map)) {
        FTBU_WARNING("Agent cannot find subscription string/mask in its catch_event_map");
        return;
    }

    mask_manager = (FTB_event_t *) FTBU_map_get_key(iter).key_ptr;
    map = (FTBMI_map_ftb_id_to_comp_info_t *) FTBU_map_get_data(iter);
    ret = FTBU_map_remove_key(map, FTBU_MAP_PTR_KEY(&comp->id));
    if (ret == FTBU_NOT_EXIST) {
        FTBU_WARNING("Agent cannot find the component for the specified mask from its catch_event_map");
        return;
    }

    if (FTBU_map_is_empty(map)) {
        FTBU_map_finalize(map);
        FTBU_map_remove_node(iter);
        /*Cancel catch mask to other ftb cores */
        FTBMI_util_reg_propagation(FTBM_MSG_TYPE_SUBSCRIPTION_CANCEL, mask_manager,
                                   &comp->id.location_id);
        free(mask_manager);
    }
}


static void FTBMI_util_clean_component(FTBMI_comp_info_t * comp)
{
    /*assumes it has the lock of component */
    /*remove it from catch map */
    FTBU_map_node_t *iter = FTBU_map_begin(comp->catch_event_set);

    for (; iter != FTBU_map_end(comp->catch_event_set); iter = FTBU_map_next_node(iter)) {
        FTB_event_t *mask = (FTB_event_t *) FTBU_map_get_data(iter);
        FTBMI_util_remove_com_from_catch_map(comp, mask);
        free(mask);
    }

    /*Finalize comp's catch set */
    FTBU_map_finalize(comp->catch_event_set);
}

static FTBMI_comp_info_t *FTBMI_lookup_component(const FTB_id_t * id)
{
    FTBMI_comp_info_t *comp = NULL;
    FTBU_map_node_t *iter;

    lock_manager();
    iter = FTBU_map_find_key(FTBMI_info.peers, FTBU_MAP_PTR_KEY(id));
    if (iter != FTBU_map_end(FTBMI_info.peers)) {
        comp = (FTBMI_comp_info_t *) FTBU_map_get_data(iter);
    }
    unlock_manager();

    return comp;
}

static FTBMI_comp_info_t *FTBMI_lookup_component_by_location(const FTB_location_id_t * location_id)
{
    FTBMI_comp_info_t *comp = NULL;
    FTBU_map_node_t *iter;

    lock_manager();

    iter = FTBU_map_begin(FTBMI_info.peers);

    while ( iter != FTBU_map_end(FTBMI_info.peers)) {
      comp = (FTBMI_comp_info_t *) FTBU_map_get_data(iter);
      if ( FTBU_is_equal_location_id(location_id, &comp->id.location_id) ) {
	return comp;
      }
      iter = iter->next;
    }
    unlock_manager();
    return comp;
}


static int FTBMI_util_reconnect()
{
    FTBM_msg_t msg;
    int ret;
    FTBU_INFO("In FTBMI_util_reconnect");

    memcpy(&msg.src, &FTBMI_info.self, sizeof(FTB_id_t));
    msg.msg_type = FTBM_MSG_TYPE_CLIENT_REG;
    ret = FTBN_Connect(&msg, &FTBMI_info.parent);

    if ((FTBMI_info.leaf && FTBMI_info.parent.pid == 0) || (ret != FTB_SUCCESS)) {
        FTBU_WARNING("Can not connect to any ftb agent");
        FTBMI_initialized = 0;
        FTBU_INFO("Out FTBMI_util_reconnect - could not connect to ftb agent");
        return ret;
    }

    if (!FTBMI_info.leaf && FTBMI_info.parent.pid != 0) {
        FTBMI_comp_info_t *comp;
        FTBU_map_node_t *iter;
	int ret = 0;

        FTBU_INFO("Adding parent to peers while reconecting");
        if ((comp = (FTBMI_comp_info_t *) malloc(sizeof(FTBMI_comp_info_t))) == NULL) {
            FTBU_INFO("Malloc error in FTB library");
        }
	memset(comp, 0, sizeof(FTBMI_comp_info_t));

        memcpy(&comp->id.location_id, &FTBMI_info.parent, sizeof(FTB_location_id_t));
        strcpy(comp->id.client_id.comp_cat, "FTB_COMP_CAT_BACKPLANE");
        strcpy(comp->id.client_id.comp, "FTB_COMP_MANAGER");
        comp->id.client_id.ext = 0;
        pthread_mutex_init(&comp->lock, NULL);

	ret = 0;
	ret = FTBU_map_init(FTBMI_util_is_equal_event_mask, &comp->catch_event_set);
        FTBU_INFO
            ("Going to call FTBU_map_insert() to insert as key=client_info->comp_info->id (parents id), data=comp_info (parents backplane) map=FTBMI_info.peers");
        if (FTBU_map_insert(FTBMI_info.peers, FTBU_MAP_PTR_KEY(&comp->id), (void *) comp) == FTBU_EXIST) {
            free(comp);
            FTBU_INFO("Out FTBMI_util_reconnect - new client already present in peer list");
            return FTB_SUCCESS; /*Should this be a success*/
        }
        FTBU_INFO("Register all catches");
        memcpy((void *) &msg.src, (void *) &FTBMI_info.self, sizeof(FTB_id_t));
        msg.msg_type = FTBM_MSG_TYPE_REG_SUBSCRIPTION;
        memcpy(&msg.dst, &comp->id, sizeof(FTB_id_t));

        for (iter = FTBU_map_begin(FTBMI_info.catch_event_map);
             iter != FTBU_map_end(FTBMI_info.catch_event_map); iter = FTBU_map_next_node(iter)) {
            FTB_event_t *mask = (FTB_event_t *) FTBU_map_get_key(iter).key_ptr;
            memcpy(&msg.event, mask, sizeof(FTB_event_t));
            ret = FTBN_Send_msg(&msg);
            if (ret != FTB_SUCCESS) {
                FTBU_WARNING("FTBN_Send_msg failed");
            }
        }
    }
    return FTB_SUCCESS;
    FTBU_INFO("Out FTBMI_util_reconnect - successfully");
}


static void FTBMI_util_get_system_id(uint32_t * system_id)
{
    *system_id = 1;
}


static void FTBMI_util_get_location_id(FTB_location_id_t * location_id)
{
    time_t raw_time;
    struct tm timeinfo;

    location_id->pid = getpid();
    if (FTBN_Get_my_network_address(location_id->hostname) < 0) {
        FTBU_WARNING("Failed to get the IP address of the %s.", location_id->hostname);
    }

    if (time(&raw_time) != -1) {
        localtime_r(&raw_time, &timeinfo);
        strftime(location_id->pid_starttime, FTB_MAX_PID_TIME, "%a_%b_%d_%j_%H:%M:%S", &timeinfo);
    }
    else {
        FTBU_WARNING("Failed to get the current time for the process.");
    }
}

int FTBM_Get_catcher_comp_list(const FTB_event_t * event, FTB_id_t ** list, int *len)
{
    FTBU_map_node_t *iter_comp;
    FTBU_map_node_t *iter_mask;
    FTBMI_map_ftb_id_to_comp_info_t *catcher_set;
    int temp_len = 0;
    int ret = 0;

    if (!FTBMI_initialized)
        return FTB_ERR_GENERAL;

    /*
     * Contruct a deliver set to keep track which components have already
     * gotten the event and which are not, to avoid duplication
     */
    FTBU_INFO("FTBM_Get_catcher_comp_list In");
    ret = 0;
    ret = FTBU_map_init(FTBMI_util_is_equal_ftb_id, &catcher_set);

    lock_manager();

    FTBU_INFO("Agent going through the list of clients that have subscribed to the event");

    /*
     * Masks are stored in my catch_event_map. Go through my
     * catch_event_map to check if (1) Event matches the mask and
     * (2) Who all are interested in getting this event that matched the mask
     */
    for (iter_mask = FTBU_map_begin(FTBMI_info.catch_event_map);
         iter_mask != FTBU_map_end(FTBMI_info.catch_event_map);
         iter_mask = FTBU_map_next_node(iter_mask)) {
        FTB_event_t *mask = (FTB_event_t *) FTBU_map_get_key(iter_mask).key_ptr;


        if (FTBU_match_mask(event, mask)) {

            FTBU_INFO
                ("Event matched against mask in catch_event_map. Retrieving clients who have subscribed to this mask");
            FTBU_map_node_t *map = (FTBU_map_node_t *) FTBU_map_get_data(iter_mask);

            /*
             * The data part of the catch_event map is another linked
             * list that contains the list of destinations interested in
             * receiving this event
             */
            for (iter_comp = FTBU_map_begin(map); iter_comp != FTBU_map_end(map);
                 iter_comp = FTBU_map_next_node(iter_comp)) {
                FTBMI_comp_info_t *comp = (FTBMI_comp_info_t *) FTBU_map_get_data(iter_comp);

                /*
                 * This event could have matched many masks registered by the same client.
                 * Check and see if this component/client was already selected and put in the
                 * catcher list in one of the previous iterations of these for loops
                 */

                FTBU_INFO("Test whether client has already been added to receive this event");
                if (FTBU_map_find_key(catcher_set, FTBU_MAP_PTR_KEY(&comp->id)) !=
                    FTBU_map_end(catcher_set)) {
                    FTBU_INFO("Component has already counted in to receive this event. ");
                    continue;
                }
                temp_len++;

                /*
                 * If compnent/client is not a part of the catcher list, then make it a part now by adding its FTB_idS
                 */
                FTBU_INFO
                    ("Going to call FTBU_map_insert() to insert as key=comp->id, data=comp->id map=catcher_set");
                FTBU_map_insert(catcher_set, FTBU_MAP_PTR_KEY(&comp->id), (void *) &comp->id);
            }
        }
    }
    unlock_manager();

    if ((*list = (FTB_id_t *) malloc(sizeof(FTB_id_t) * temp_len)) == NULL) {
        FTBU_INFO("Malloc error in FTB library");
        return (FTB_ERR_CLASS_FATAL + FTB_ERR_MALLOC);
    }
    *len = temp_len;

    /*
     * Go through the catcher_set that was created - and copy the entire list in the calleefunction-passed argument for the list. The list
     * is a collection of FTB_ids.
     * Also get the total number of destinations, to pass it back to the callee function
     */
    for (iter_comp = FTBU_map_begin(catcher_set); iter_comp != FTBU_map_end(catcher_set);
         iter_comp = FTBU_map_next_node(iter_comp)) {
        FTB_id_t *id = (FTB_id_t *) FTBU_map_get_data(iter_comp);
        temp_len--;
        memcpy(&((*list)[temp_len]), id, sizeof(FTB_id_t));
        FTBU_INFO("For this mask - send events to destination=%s client_name=%s\n",
                  id->location_id.hostname, id->client_id.client_name);
    }

    /*
     * Destroy the catcher_set map for now
     */
    FTBU_map_finalize(catcher_set);

    FTBU_INFO("FTBM_Get_catcher_comp_list Out");
    return FTB_SUCCESS;
}


int FTBM_Release_comp_list(FTB_id_t * list)
{
    FTBU_INFO("FTBM_Release_comp_list In");
    free(list);
    FTBU_INFO("FTBM_Release_comp_list Out");
    return FTB_SUCCESS;
}


int FTBM_Get_location_id(FTB_location_id_t * location_id)
{
    if (!FTBMI_initialized)
        return FTB_ERR_GENERAL;
    memcpy(location_id, &FTBMI_info.self.location_id, sizeof(FTB_location_id_t));
    return FTB_SUCCESS;
}


int FTBM_Get_parent_location_id(FTB_location_id_t * location_id)
{
    if (!FTBMI_initialized)
        return FTB_ERR_GENERAL;
    memcpy(location_id, &FTBMI_info.parent, sizeof(FTB_location_id_t));
    return FTB_SUCCESS;
}


int FTBM_Init(int leaf)
{
    FTBN_config_info_t config;
    FTBM_msg_t msg;
    int ret =0 ;

    FTBU_INFO("FTBM_Init In");

    FTBMI_info.leaf = leaf;
    if (!leaf) {
	ret = 0;
	ret = FTBU_map_init(FTBMI_util_is_equal_ftb_id, &FTBMI_info.peers);
	ret = 0;
	ret = FTBU_map_init(FTBMI_util_is_equal_event_mask, &FTBMI_info.catch_event_map);
        FTBMI_info.err_handling = FTB_ERR_HANDLE_RECOVER;

        pthread_mutex_init(&message_queue_mutex, NULL);
        pthread_cond_init(&message_queue_cond, NULL);
    }
    else {
        FTBMI_info.peers = NULL;
        FTBMI_info.catch_event_map = NULL;
        FTBMI_info.err_handling = FTB_ERR_HANDLE_NONE;  /* May change to recover mode if one client component requires so */
    }

    config.leaf = leaf;
    FTBMI_util_get_system_id(&config.FTB_system_id);
    FTBMI_util_get_location_id(&FTBMI_info.self.location_id);
    strcpy(FTBMI_info.self.client_id.comp, "FTB_COMP_MANAGER");
    strcpy(FTBMI_info.self.client_id.comp_cat, "FTB_COMP_CAT_BACKPLANE");
    FTBMI_info.self.client_id.ext = leaf;

    FTBN_Init(&FTBMI_info.self.location_id, &config);
    memcpy(&msg.src, &FTBMI_info.self, sizeof(FTB_id_t));
    msg.msg_type = FTBM_MSG_TYPE_CLIENT_REG;

    if ((ret = FTBN_Connect(&msg, &FTBMI_info.parent)) != FTB_SUCCESS) {
	FTBU_INFO("FTBN_Connect failed\n");
	return ret;
    }
    
    if (leaf && FTBMI_info.parent.pid == 0) {
        FTBU_WARNING("Can not connect to any ftb agent");
        FTBU_INFO("FTBM_Init Out - could not connect to any ftb agent");
        return FTB_ERR_GENERAL;
    }

    if (!leaf && FTBMI_info.parent.pid != 0) {
        FTBMI_comp_info_t *comp;
	int ret = 0;
        FTBU_INFO("Adding parent to peers");
        if ((comp = (FTBMI_comp_info_t *) malloc(sizeof(FTBMI_comp_info_t))) == NULL) {
            FTBU_INFO("Malloc error in FTB library");
            return (FTB_ERR_CLASS_FATAL + FTB_ERR_MALLOC);
        }
	memset(comp, 0, sizeof(FTBMI_comp_info_t));
        memcpy(&comp->id.location_id, &FTBMI_info.parent, sizeof(FTB_location_id_t));
        strcpy(comp->id.client_id.comp_cat, "FTB_COMP_CAT_BACKPLANE");
        strcpy(comp->id.client_id.comp, "FTB_COMP_MANAGER");
        comp->id.client_id.ext = 0;
        pthread_mutex_init(&comp->lock, NULL);
	ret = 0;
	ret = FTBU_map_init(FTBMI_util_is_equal_event_mask, &comp->catch_event_set);
        FTBU_INFO
            ("Agent is going to call FTBU_map_insert() to insert as key=comp_id, data=comp(parents) map=FTBMI_info.peers");
        if (FTBU_map_insert(FTBMI_info.peers, FTBU_MAP_PTR_KEY(&comp->id), (void *) comp) == FTBU_EXIST) {
            FTBU_INFO("FTBM_Init Out - client already present in the peer list");
            free(comp);
            return FTB_ERR_GENERAL;
        }
    }

    FTBMI_info.waiting = 0;
    lock_manager();
    FTBMI_initialized = 1;
    unlock_manager();
    FTBU_INFO("FTBM_Init Out - success");
    FTBU_INFO("My location is %s and my parent is %s\n", FTBMI_info.self.location_id.hostname,
              FTBMI_info.parent.hostname);
    return FTB_SUCCESS;
}


/* Called when the ftb node get disconnected */
int FTBM_Finalize()
{
    FTBU_INFO("FTBM_Finalize In");
    if (!FTBMI_initialized) {
        FTBU_INFO("FTBM_Finalize Out");
        return FTB_SUCCESS;
    }

    lock_manager();
    FTBMI_initialized = 0;

    if (!FTBMI_info.leaf) {
        FTBU_map_node_t *iter = FTBU_map_begin(FTBMI_info.peers);
        for (; iter != FTBU_map_end(FTBMI_info.peers); iter = FTBU_map_next_node(iter)) {
            FTBMI_comp_info_t *comp = (FTBMI_comp_info_t *) FTBU_map_get_data(iter);
            FTBMI_util_clean_component(comp);

            if ((strcasecmp(comp->id.client_id.comp_cat, "FTB_COMP_CAT_BACKPLANE") == 0)
                && (strcasecmp(comp->id.client_id.comp, "FTB_COMP_MANAGER") == 0)
                && (comp->id.client_id.ext == 0)) {
                int ret;
                FTBM_msg_t msg;
                FTBU_INFO("deregister from peer");
                memcpy(&msg.src, &FTBMI_info.self, sizeof(FTB_id_t));
                memcpy(&msg.dst, &comp->id, sizeof(FTB_id_t));
                msg.msg_type = FTBM_MSG_TYPE_CLIENT_DEREG;
                ret = FTBN_Send_msg(&msg);
                if (ret != FTB_SUCCESS)
                    FTBU_WARNING("FTBN_Send_msg failed %d", ret);
            }
            free(comp);
        }

        FTBU_map_finalize(FTBMI_info.peers);
        FTBU_map_finalize(FTBMI_info.catch_event_map);
    }
    else {
        int ret;
        FTBM_msg_t msg;

        FTBU_INFO("deregister only from parent");
        memcpy(&msg.src, &FTBMI_info.self, sizeof(FTB_id_t));
        memcpy(&msg.dst.location_id, &FTBMI_info.parent, sizeof(FTB_location_id_t));
        msg.msg_type = FTBM_MSG_TYPE_CLIENT_DEREG;
        ret = FTBN_Send_msg(&msg);
        if (ret != FTB_SUCCESS)
            FTBU_WARNING("FTBN_Send_msg failed %d", ret);
    }
    FTBN_Finalize();
    unlock_manager();
    FTBU_INFO("FTBM_Finalize Out");
    return FTB_SUCCESS;
}


int FTBM_Abort()
{
    FTBU_INFO("FTBM_Abort In");
    if (!FTBMI_initialized) {
        FTBU_INFO("FTBM_Abort Out");
        return FTB_SUCCESS;
    }
    FTBMI_initialized = 0;

    if (!FTBMI_info.leaf) {
        FTBU_map_node_t *iter = FTBU_map_begin(FTBMI_info.peers);
        for (; iter != FTBU_map_end(FTBMI_info.peers); iter = FTBU_map_next_node(iter)) {
            FTBMI_comp_info_t *comp = (FTBMI_comp_info_t *) FTBU_map_get_data(iter);
            free(comp);
        }

        FTBU_map_finalize(FTBMI_info.peers);
        FTBU_map_finalize(FTBMI_info.catch_event_map);
    }

    FTBN_Abort();
    FTBU_INFO("FTBM_Abort Out");
    return FTB_SUCCESS;
}


int FTBM_Client_register(const FTB_id_t * id)
{
    FTBMI_comp_info_t *comp;
    int ret;
    if (!FTBMI_initialized)
        return FTB_ERR_GENERAL;

    FTBU_INFO("FTBM_Client_register In");

    if (FTBMI_info.leaf) {
        FTBU_INFO("FTBM_Client_register Out");
        return FTB_ERR_NOT_SUPPORTED;
    }

    if ((comp = (FTBMI_comp_info_t *) malloc(sizeof(FTBMI_comp_info_t))) == NULL) {
        FTBU_INFO("Malloc error in FTB library");
        return (FTB_ERR_CLASS_FATAL + FTB_ERR_MALLOC);
    }
    memcpy(&comp->id, id, sizeof(FTB_id_t));
    pthread_mutex_init(&comp->lock, NULL);

    ret = 0;
    ret = FTBU_map_init(FTBMI_util_is_equal_event_mask, &comp->catch_event_set);

    lock_manager();

    FTBU_INFO
        ("Going to call FTBU_map_insert() to insert as key=comp_id, data=comp and map=FTBMI_info.peers -- for the new Client_register request");

    if (FTBU_map_insert(FTBMI_info.peers, FTBU_MAP_PTR_KEY(&comp->id), (void *) comp) == FTBU_EXIST) {
        FTBU_WARNING("Client already registered as a peer");
        unlock_manager();
        FTBU_map_finalize(comp->catch_event_set);
        free(comp);
        FTBU_INFO("FTBM_Client_register Out");
        return FTB_ERR_INVALID_PARAMETER;
    }
    unlock_manager();

    /*propagate catch info to the new component if it is an agent */
    if ((strcasecmp(id->client_id.comp_cat, "FTB_COMP_CAT_BACKPLANE") == 0)
        && (strcasecmp(id->client_id.comp, "FTB_COMP_MANAGER") == 0)
        && id->client_id.ext == 0) {
        FTBM_msg_t msg;

        memcpy((void *) &msg.src, (void *) &FTBMI_info.self, sizeof(FTB_id_t));
        msg.msg_type = FTBM_MSG_TYPE_REG_SUBSCRIPTION;
        memcpy(&msg.dst, id, sizeof(FTB_id_t));

        FTBU_map_node_t *iter;
        for (iter = FTBU_map_begin(FTBMI_info.catch_event_map);
             iter != FTBU_map_end(FTBMI_info.catch_event_map); iter = FTBU_map_next_node(iter)) {
            FTB_event_t *mask = (FTB_event_t *) FTBU_map_get_key(iter).key_ptr;
            memcpy(&msg.event, mask, sizeof(FTB_event_t));
            ret = FTBN_Send_msg(&msg);
            if (ret != FTB_SUCCESS) {
                FTBU_WARNING("FTBN_Send_msg failed");
            }
        }
    }
    FTBU_INFO
        ("new client comp:%s comp_cat:%s client_name:%s ext:%d registered, from host %s, pid %d pid starttime=%s",
         comp->id.client_id.comp, comp->id.client_id.comp_cat, comp->id.client_id.client_name,
         comp->id.client_id.ext, comp->id.location_id.hostname, comp->id.location_id.pid,
         comp->id.location_id.pid_starttime);

    FTBU_INFO("FTBM_Client_register Out");
    return FTB_SUCCESS;
}


int FTBM_Client_deregister(const FTB_id_t * id)
{
    FTBMI_comp_info_t *comp;
    int is_parent = 0;

    if (!FTBMI_initialized)
        return FTB_ERR_GENERAL;

    FTBU_INFO("FTBM_Client_deregister In");
    if (FTBMI_info.leaf) {
        FTBU_INFO("FTBM_Client_deregister Out");
        return FTB_ERR_NOT_SUPPORTED;
    }

    comp = FTBMI_lookup_component(id);
    if (comp == NULL) {
        FTBU_INFO("FTBM_Client_deregister Out");
        return FTB_ERR_INVALID_PARAMETER;
    }

    FTBU_INFO("client comp:%s comp_cat:%s client_name=%s  ext:%d from host %s pid %d, deregistering",
              comp->id.client_id.comp, comp->id.client_id.comp_cat, comp->id.client_id.client_name,
              comp->id.client_id.ext, comp->id.location_id.hostname, comp->id.location_id.pid);

    lock_comp(comp);
    lock_manager();
    FTBMI_util_clean_component(comp);
    if ((strcmp(comp->id.client_id.comp_cat, "FTB_COMP_CAT_BACKPLANE") == 0)
        && (strcmp(comp->id.client_id.comp, "FTB_COMP_MANAGER") == 0)) {
        FTBU_INFO("Disconnect component");
        FTBN_Disconnect_peer(&comp->id.location_id);
        if (FTBU_is_equal_location_id(&FTBMI_info.parent, &comp->id.location_id))
            is_parent = 1;
    }
    FTBU_map_remove_key(FTBMI_info.peers, FTBU_MAP_PTR_KEY(&comp->id));
    unlock_manager();
    unlock_comp(comp);
    free(comp);

    if (is_parent) {
        FTBU_WARNING("Parent exiting");
        if (FTBMI_info.err_handling & FTB_ERR_HANDLE_RECOVER) {
	    int ret1 = 0;
            FTBU_WARNING("Reconnecting");
            lock_manager();
            ret1 = FTBMI_util_reconnect();
            unlock_manager();
	    if (ret1 != FTB_SUCCESS) {
                FTBU_WARNING("Reconnection to agent failed");
		FTBU_INFO("FTBM_Client_deregister Out")
		return ret1;
            }	
	    
        }
    }

    FTBU_INFO("FTBM_Client_deregister Out");
    return FTB_SUCCESS;
}


int FTBM_Register_publishable_event(const FTB_id_t * id, FTB_event_t * event)
{
    FTBMI_comp_info_t *comp;
    if (!FTBMI_initialized)
        return FTB_ERR_GENERAL;
    FTBU_INFO("FTBM_Register_publishable_event In");
    if (FTBMI_info.leaf) {
        FTBU_INFO("FTBM_Register_publishable_event Out");
        return FTB_ERR_NOT_SUPPORTED;
    }
    comp = FTBMI_lookup_component(id);
    if (comp == NULL) {
        FTBU_INFO("FTBM_Register_publishable_event Out");
        return FTB_ERR_INVALID_PARAMETER;
    }

    /*Currently not doing anything */

    FTBU_INFO("FTBM_Register_publishable_event Out");
    return FTB_SUCCESS;
}


/*
 * This routine is called by the agent to register a susbscription.
 * The agent will also propagate this subscription information to other agents
 */

int FTBM_Register_subscription(const FTB_id_t * id, FTB_event_t * event)
{
    FTBU_map_node_t *iter;
    FTB_event_t *temp_mask1, *temp_mask2;
    FTBMI_comp_info_t *comp;

    FTBU_INFO("FTBM_Register_subscription In");

    if (!FTBMI_initialized)
        return FTB_ERR_GENERAL;

    if (FTBMI_info.leaf) {
        FTBU_INFO
            ("FTBM_Register_subscription Out with Error - Routine can be called only by FTB agents");
        return FTB_ERR_NOT_SUPPORTED;
    }

    comp = FTBMI_lookup_component(id);
    if (comp == NULL) {
        FTBU_INFO("FTBM_Register_subscription Out - Invalid parameter");
        return FTB_ERR_INVALID_PARAMETER;
    }

    if ((temp_mask1 = (FTB_event_t *) malloc(sizeof(FTB_event_t))) == NULL) {
        FTBU_INFO("Malloc error in FTB library");
        return (FTB_ERR_CLASS_FATAL + FTB_ERR_MALLOC);
    }
    memcpy(temp_mask1, event, sizeof(FTB_event_t));

    lock_comp(comp);
    /*
     * Agent maintains a list of all components. For each component,
     * it keep track of the unique subscription/masks that the component
     * has subscribed to in the catch_event_set subfield.
     */
    FTBU_INFO("Agent inserting the new subscription/mask to the catch_event_set of the component");
    if (FTBU_map_insert(comp->catch_event_set, FTBU_MAP_PTR_KEY(temp_mask1), (void *) temp_mask1)
        == FTBU_EXIST) {
        free(temp_mask1);
        FTBU_INFO("FTBM_Register_subscription Out - Mask for the component already registered");
    }

    /*
     * Every mask in the catch_event_set is also present in the catch_event_map map.
     * The catch_event_map map uses the 'mask' as the key and points to a map of 'FTB_ids'.
     * These FTB_ids is the collection of ftb-ids who are interested in receiving an event that
     * matches this mask
     *
     * First, this new mask is checked to see if it is already present in the catch_event_map map.
     * If it is, then the source FTB_id of this subscription request is added to the list of
     * the 'FTB_ids' map
     *
     * If this mask is not present as a 'key' in the catch_event_map map, then add it to the
     * catch_event_map map. Also create the intial map of FTB_ids for this mask and add the source
     * FTB-id of this request to it.
     */

    lock_manager();
    iter = FTBU_map_find_key(FTBMI_info.catch_event_map, FTBU_MAP_PTR_KEY(temp_mask1));
    if (iter == FTBU_map_end(FTBMI_info.catch_event_map)) {
        FTBMI_map_ftb_id_to_comp_info_t *new_map;
	int ret;

        if ((temp_mask2 = (FTB_event_t *) malloc(sizeof(FTB_event_t))) == NULL) {
    	    FTBU_INFO("Malloc error in FTB library");
            return (FTB_ERR_CLASS_FATAL + FTB_ERR_MALLOC);
        }
        memcpy(temp_mask2, event, sizeof(FTB_event_t));
	ret = 0;
	ret = FTBU_map_init(FTBMI_util_is_equal_ftb_id, &new_map);
        FTBU_INFO
            ("Agent has received a new subscription string/mask for registration from host:%s",
             comp->id.location_id.hostname);
        FTBU_INFO
            ("Agent inserting the new subscription (as a key) and the corresponding component information (as data) in its global catch_event_map");
        /*
         * The received mask becomes the key and the data portion for this key is a map containing the component information.
         * The map, in turn, is a linked list with the ftb_id of the component as the key and the rest of the component
         * information as the data.
         */
        FTBU_map_insert(FTBMI_info.catch_event_map, FTBU_MAP_PTR_KEY(temp_mask2), (void *) new_map);
        FTBU_map_insert(new_map, FTBU_MAP_PTR_KEY(&comp->id), (void *) comp);
        FTBMI_util_reg_propagation(FTBM_MSG_TYPE_REG_SUBSCRIPTION, event, &id->location_id);

    }
    else {
        FTBMI_map_ftb_id_to_comp_info_t *existing_map;

        existing_map = (FTBMI_map_ftb_id_to_comp_info_t *) FTBU_map_get_data(iter);
        if (FTBU_map_insert(existing_map, FTBU_MAP_PTR_KEY(&comp->id), (void *) comp) == FTBU_EXIST) {
            /* FTBU_WARNING("Agent has received duplicate registration request for subscription string/mask and client combination; host = %s", comp->id.location_id.hostname); */
        }
        else {
            FTBMI_util_reg_propagation(FTBM_MSG_TYPE_REG_SUBSCRIPTION, event, &id->location_id);
        }
    }

    unlock_manager();
    unlock_comp(comp);
    FTBU_INFO("FTBM_Register_subscription Out");

    return FTB_SUCCESS;
}

int FTBM_Publishable_event_registration_cancel(const FTB_id_t * id, FTB_event_t * event)
{
    FTBMI_comp_info_t *comp;
    if (!FTBMI_initialized)
        return FTB_ERR_GENERAL;
    FTBU_INFO("FTBM_Publishable_event_registration_cancel In");
    if (FTBMI_info.leaf) {
        FTBU_INFO("FTBM_Publishable_event_registration_cancel Out");
        return FTB_ERR_NOT_SUPPORTED;
    }
    comp = FTBMI_lookup_component(id);
    if (comp == NULL) {
        FTBU_INFO("FTBM_Publishable_event_registration_cancel Out");
        return FTB_ERR_INVALID_PARAMETER;
    }

    /*Currently not doing anything */

    FTBU_INFO("FTBM_Publishable_event_registration_cancel Out");
    return FTB_SUCCESS;
}

int FTBM_Cancel_subscription(const FTB_id_t * id, FTB_event_t * event)
{
    FTB_event_t *mask;
    FTBU_map_node_t *iter;
    FTBMI_comp_info_t *comp;

    if (!FTBMI_initialized)
        return FTB_ERR_GENERAL;
    FTBU_INFO("FTBM_Cancel_subscription In");
    if (FTBMI_info.leaf) {
        FTBU_INFO("FTBM_Cancel_subscription Out");
        return FTB_ERR_NOT_SUPPORTED;
    }
    comp = FTBMI_lookup_component(id);
    if (comp == NULL) {
        FTBU_INFO("FTBM_Cancel_subscription Out");
        return FTB_ERR_INVALID_PARAMETER;
    }

    lock_comp(comp);
    FTBU_INFO("util_reg_catch_cancel");
    iter = FTBU_map_find_key(comp->catch_event_set, FTBU_MAP_PTR_KEY(event));
    if (iter == FTBU_map_end(comp->catch_event_set)) {
        FTBU_WARNING("Not found throw entry");
        FTBU_INFO("FTBM_Cancel_subscription Out");
        unlock_comp(comp);
        return FTB_SUCCESS;
    }
    mask = (FTB_event_t *) FTBU_map_get_data(iter);
    FTBU_map_remove_node(iter);
    lock_manager();
    FTBMI_util_remove_com_from_catch_map(comp, mask);
    unlock_manager();
    free(mask);
    unlock_comp(comp);
    FTBU_INFO("FTBM_Cancel_subscription Out");
    return FTB_SUCCESS;
}

int FTBM_Send(const FTBM_msg_t * msg)
{
    int ret;

    if (!FTBMI_initialized)
        return FTB_ERR_GENERAL;
    FTBU_INFO("FTBM_Send In");

    ret = FTBN_Send_msg(msg);
    if (ret != FTB_SUCCESS) {
        if (!FTBMI_info.leaf) { /* For an agent */
            FTBMI_comp_info_t *comp;
            comp = FTBMI_lookup_component(&msg->dst);
            FTBU_INFO("client %s:%s:%d from host %s pid %d, clean up",
                      comp->id.client_id.comp, comp->id.client_id.comp_cat, comp->id.client_id.ext,
                      comp->id.location_id.hostname, comp->id.location_id.pid);
            lock_comp(comp);
            lock_manager();
            FTBMI_util_clean_component(comp);
            if ((strcasecmp(comp->id.client_id.comp_cat, "FTB_COMP_CAT_BACKPLANE") == 0)
                && (strcasecmp(comp->id.client_id.comp, "FTB_COMP_MANAGER") == 0)) {
                FTBU_INFO("Disconnect component");
                FTBN_Disconnect_peer(&comp->id.location_id);
            }
            FTBU_map_remove_key(FTBMI_info.peers, FTBU_MAP_PTR_KEY(&comp->id));
            unlock_manager();
            unlock_comp(comp);
            free(comp);
        }

        if (FTBU_is_equal_location_id(&FTBMI_info.parent, &msg->dst.location_id)) {
	    int ret1 = 0;
            FTBU_WARNING("Lost connection to parent");
            //if (FTBMI_info.err_handling & FTB_ERR_HANDLE_RECOVER) {
            FTBU_WARNING("Reconnecting");
            lock_manager();
            ret1 = FTBMI_util_reconnect();
	    if (ret1 != FTB_SUCCESS) {
        	FTBU_WARNING("FTB could not connect to parent");
      	    }
            unlock_manager();
            //}
        }
    }
    FTBU_INFO("FTBM_Send Out");
    return ret;
}

int FTBM_Cleanup_connection(const FTB_location_id_t * location_id)
{
    int count = 0;
    FTBMI_comp_info_t *comp = NULL;
    FTBU_map_node_t *iter, *temp;


    if (!FTBMI_initialized)
        return FTB_ERR_GENERAL;

    FTBU_INFO("FTBM_Cleanup_connection In");

    iter = FTBU_map_begin(FTBMI_info.peers);
    while ( iter != FTBU_map_end(FTBMI_info.peers) ) {
      comp = (FTBMI_comp_info_t *) FTBU_map_get_data(iter);
      
      if ( FTBU_is_equal_location_id(location_id, &comp->id.location_id ) ) {
	     FTBU_INFO("client %s:%s:%d from host %s pid %d, clean up",
		       comp->id.client_id.comp, comp->id.client_id.comp_cat, 
		       comp->id.client_id.ext, comp->id.location_id.hostname, 
		       comp->id.location_id.pid);
	     lock_comp(comp);

	     FTBMI_util_clean_component(comp);

	     temp = iter;
	     iter = iter->next;

	     temp->next->prev = temp->prev;
	     temp->prev->next = temp->next;
	     free(temp);

	     unlock_comp(comp);
	     free(comp);
	     count++;
      }
      else {
	iter = iter->next;
      }
    }

    if (FTBU_is_equal_location_id(&FTBMI_info.parent, location_id)) {
      int ret1 = 0;
      FTBU_WARNING("Lost connection to parent");
      FTBU_WARNING("Reconnecting");
      lock_manager();
      ret1 = FTBMI_util_reconnect();
      unlock_manager();
      if (ret1 != FTB_SUCCESS) {
	FTBU_WARNING("FTB could not connect to parent");
      }
    }

    FTBU_INFO("FTBM_Cleanup_connection Out");

    /* Any good error code? */
    return count > 0 ? FTB_SUCCESS : FTB_ERR_GENERAL;
}

int FTBM_Get(FTBM_msg_t * msg, FTB_location_id_t * incoming_src, int blocking)
{
    int ret;

    if (!FTBMI_initialized)
        return FTB_ERR_GENERAL;
    FTBU_INFO("FTBM_Get In");

    ret = FTBN_Recv_msg(msg, incoming_src, blocking);
    if (ret == FTB_ERR_NETWORK_GENERAL) {
        FTBU_WARNING("Client is going to try to reconnect");
	int ret1 = 0;
        lock_manager();
        ret1 = FTBMI_util_reconnect();
        unlock_manager();
        FTBU_WARNING("FTBN_Recv_msg failed %d", ret);
	if (ret1 != FTB_SUCCESS) {
		FTBU_WARNING("Reconnection to agent failed");
	}
    }

    FTBU_INFO("FTBM_Get Out");
    return ret;
}

int FTBM_Recv(FTBM_msg_t * msg, FTB_location_id_t * incoming_src)
{
    FTBM_msg_node_t *ptr;
    pthread_mutex_lock(&message_queue_mutex);

    if (message_queue_head == NULL) {
        pthread_cond_wait(&message_queue_cond, &message_queue_mutex);
    }

    if (message_queue_head == NULL) {
        pthread_mutex_unlock(&message_queue_mutex);
        return FTBN_NO_MSG;
    }

    memcpy(msg, message_queue_head->msg, sizeof(FTBM_msg_t));
    memcpy(incoming_src, message_queue_head->incoming_src, sizeof(FTB_location_id_t));
    ptr = message_queue_head;
    message_queue_head = message_queue_head->next;

    if (message_queue_head == NULL)
        message_queue_tail = NULL;

    pthread_mutex_unlock(&message_queue_mutex);
    free(ptr->msg);
    free(ptr->incoming_src);
    free(ptr);

    return FTB_SUCCESS;
}

void *FTBM_Fill_message_queue(void *arg)
{
    FTBM_msg_node_t *head, *tail;
    FTB_location_id_t disconnected_location;
    int ret;

    while (1) {
        head = tail = NULL;
        if ((ret = FTBN_Grab_messages(&head, &tail, &disconnected_location)) == FTB_ERR_NETWORK_GENERAL) {
            FTBM_msg_t *msg;
	    FTBM_msg_node_t *msg_node;
	    if ((msg = (FTBM_msg_t *) malloc(sizeof(FTBM_msg_t))) == NULL) {
	        FTBU_INFO("Malloc error in FTB library");
            }
            msg->msg_type = FTBM_MSG_TYPE_CLEANUP_CONN;
	
            if ((msg_node = (FTBM_msg_node_t *) malloc(sizeof(FTBM_msg_node_t))) == NULL) {
                FTBU_INFO("Malloc error in FTB library");
            }
            msg_node->msg = msg;
            if ((msg_node->incoming_src = (FTB_location_id_t *) malloc(sizeof(FTB_location_id_t))) == NULL) {
                FTBU_INFO("Malloc error in FTB library");
            }
            memcpy(msg_node->incoming_src, &disconnected_location, sizeof(FTB_location_id_t));

            msg_node->next = head;
            head = msg_node;
        }

        pthread_mutex_lock(&message_queue_mutex);

        if (head != NULL) {
            if (message_queue_head == NULL)
                message_queue_head = head;
            if (message_queue_tail != NULL)
                message_queue_tail->next = head;
            message_queue_tail = tail;
        }

        pthread_cond_signal(&message_queue_cond);
        pthread_mutex_unlock(&message_queue_mutex);
    }
}
