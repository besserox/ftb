/**********************************************************************************/
/* FTB:ftb-info */
/* This file is part of FTB (Fault Tolerance Backplance) - the core of CIFTS
 * (Co-ordinated Infrastructure for Fault Tolerant Systems)
 * 
 * See http://www.mcs.anl.gov/research/cifts for more information.
 * 	
 */
/* FTB:ftb-info */
/* FTB:ftb-fillin */
/* FTB_Version: 0.6
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
/*********************************************************************************/
#include <stdio.h>
#include <errno.h>
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
#include "ftb_client_lib_defs.h"
#include "ftb_util.h"
#include "ftb_manager_lib.h"
#include "ftb_network.h"

extern FILE *FTBU_log_file_fp;

typedef FTBU_map_node_t FTBMI_set_event_mask_t;	/*a set of event_mask */

typedef struct FTBMI_comp_info {
    FTB_id_t id;
    pthread_mutex_t lock;
    FTBMI_set_event_mask_t *catch_event_set;
} FTBMI_comp_info_t;

typedef FTBU_map_node_t FTBMI_map_ftb_id_2_comp_info_t;	/*ftb_id as key and comp_info as data */

typedef FTBU_map_node_t FTBMI_map_event_mask_2_comp_info_map_t;	/*event_mask as key, a _map_ of comp_info as data */

typedef struct FTBM_node_info {
    FTB_location_id_t parent;	/*NULL if root */
    FTB_id_t self;
    uint8_t err_handling;
    int leaf;
    volatile int waiting;
    FTBMI_map_ftb_id_2_comp_info_t *peers;	/*the map of peers includes parent */
    FTBMI_map_event_mask_2_comp_info_map_t *catch_event_map;
} FTBMI_node_info_t;

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
    //return FTBU_is_equal_event(lhs, rhs);
}

static void FTBMI_util_reg_propagation(int msg_type, const FTB_event_t * event,
				       const FTB_location_id_t * incoming)
{
    FTBM_msg_t msg;
    int ret;
    FTBU_map_iterator_t iter_comp;
    FTB_INFO("In FTBMI_util_reg_propagation with msg_type = %d\n", msg_type);

    msg.msg_type = msg_type;

    memcpy(&msg.src, &FTBMI_info.self, sizeof(FTB_id_t));
    memcpy(&msg.event, event, sizeof(FTB_event_t));

    for (iter_comp = FTBU_map_begin(FTBMI_info.peers); iter_comp != FTBU_map_end(FTBMI_info.peers);
	 iter_comp = FTBU_map_next_iterator(iter_comp)) {
	FTB_id_t *id = (FTB_id_t *) FTBU_map_get_key(iter_comp).key_ptr;

	/* Propagation to other FTB agents but not the source */
	if (!((strcasecmp(id->client_id.comp_cat, "FTB_COMP_CAT_BACKPLANE") == 0)
	      && (strcasecmp(id->client_id.comp, "FTB_COMP_MANAGER") == 0)
	      && (id->client_id.ext == 0))
	    || FTBU_is_equal_location_id(&id->location_id, incoming))
	    continue;


	memcpy(&msg.dst, id, sizeof(FTB_id_t));

	ret = FTBN_Send_msg(&msg);
	if (ret != FTB_SUCCESS) {
	    FTB_INFO("Out FTBMI_util_reg_propagation\n");
	    FTB_WARNING("FTBN_Send_msg failed");
	}
    }
    FTB_INFO("Out FTBMI_util_reg_propagation\n");
}


static void FTBMI_util_remove_com_from_catch_map(const FTBMI_comp_info_t * comp,
						 const FTB_event_t * mask)
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

    mask_manager = (FTB_event_t *) FTBU_map_get_key(iter).key_ptr;
    map = (FTBMI_map_ftb_id_2_comp_info_t *) FTBU_map_get_data(iter);
    ret = FTBU_map_remove_key(map, FTBU_MAP_PTR_KEY(&comp->id));
    if (ret == FTBU_NOT_EXIST) {
	FTB_WARNING("not found component");
	return;
    }

    if (FTBU_map_is_empty(map)) {
	FTBU_map_finalize(map);
	FTBU_map_remove_iter(iter);
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
    FTBU_map_iterator_t iter = FTBU_map_begin(comp->catch_event_set);

    for (; iter != FTBU_map_end(comp->catch_event_set); iter = FTBU_map_next_iterator(iter)) {
	FTB_event_t *mask = (FTB_event_t *) FTBU_map_get_data(iter);
	FTBMI_util_remove_com_from_catch_map(comp, mask);
	free(mask);
    }

    /*Finalize comp's catch set */
    FTBU_map_finalize(comp->catch_event_set);
}


static inline FTBMI_comp_info_t *FTBMI_lookup_component(const FTB_id_t * id)
{
    FTBMI_comp_info_t *comp = NULL;
    FTBU_map_iterator_t iter;

    lock_manager();
    iter = FTBU_map_find(FTBMI_info.peers, FTBU_MAP_PTR_KEY(id));
    if (iter != FTBU_map_end(FTBMI_info.peers)) {
	comp = (FTBMI_comp_info_t *) FTBU_map_get_data(iter);
    }
    unlock_manager();
    return comp;
}


static void FTBMI_util_reconnect()
{
    FTBM_msg_t msg;
    int ret;
    FTB_INFO("In FTBMI_util_reconnect");

    memcpy(&msg.src, &FTBMI_info.self, sizeof(FTB_id_t));
    msg.msg_type = FTBM_MSG_TYPE_CLIENT_REG;

    FTBN_Connect(&msg, &FTBMI_info.parent);
    if (FTBMI_info.leaf && FTBMI_info.parent.pid == 0) {
	FTB_WARNING("Can not connect to any ftb agent");
	FTBMI_initialized = 0;
        FTB_INFO("Out FTBMI_util_reconnect - could not connect to ftb agent");
	return;
    }
    if (!FTBMI_info.leaf && FTBMI_info.parent.pid != 0) {
	FTBMI_comp_info_t *comp;
	FTBU_map_iterator_t iter;

	FTB_INFO("Adding parent to peers while reconecting");
	comp = (FTBMI_comp_info_t *) malloc(sizeof(FTBMI_comp_info_t));
	memcpy(&comp->id.location_id, &FTBMI_info.parent, sizeof(FTB_location_id_t));
	strcpy(comp->id.client_id.comp_cat, "FTB_COMP_CAT_BACKPLANE");
	strcpy(comp->id.client_id.comp, "FTB_COMP_MANAGER");
	comp->id.client_id.ext = 0;
	pthread_mutex_init(&comp->lock, NULL);
	//comp->catch_event_set = (FTBMI_set_event_mask_t *) FTBU_map_init(FTBMI_util_is_equal_event);
	comp->catch_event_set = (FTBMI_set_event_mask_t *) FTBU_map_init(FTBMI_util_is_equal_event_mask);

        FTB_INFO("Going to call FTBU_map_insert() to insert as key=client_info->comp_info->id (parents id), data=comp_info (parents backplane) map=FTBMI_info.peers");
	if (FTBU_map_insert(FTBMI_info.peers, FTBU_MAP_PTR_KEY(&comp->id), (void *) comp) == FTBU_EXIST) {
            FTB_INFO("Out FTBMI_util_reconnect - new client already present in peer list");
	    return;
	}
	FTB_INFO("Register all catches");
	memcpy((void *) &msg.src, (void *) &FTBMI_info.self, sizeof(FTB_id_t));
	msg.msg_type = FTBM_MSG_TYPE_REG_SUBSCRIPTION;
	memcpy(&msg.dst, &comp->id, sizeof(FTB_id_t));

	for (iter = FTBU_map_begin(FTBMI_info.catch_event_map);
	     iter != FTBU_map_end(FTBMI_info.catch_event_map); iter = FTBU_map_next_iterator(iter)) {
	    FTB_event_t *mask = (FTB_event_t *) FTBU_map_get_key(iter).key_ptr;
	    memcpy(&msg.event, mask, sizeof(FTB_event_t));
	    ret = FTBN_Send_msg(&msg);
	    if (ret != FTB_SUCCESS) {
		FTB_WARNING("FTBN_Send_msg failed");
	    }
	}
    }
    return;
    FTB_INFO("Out FTBMI_util_reconnect - successfully");
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
	FTB_WARNING("Failed to get the IP address of the %s.", location_id->hostname);
    }

    if (time(&raw_time) != -1) {
	localtime_r(&raw_time, &timeinfo);
	strftime(location_id->pid_starttime, FTB_MAX_PID_TIME, "%a_%b_%d_%j_%H:%M:%S", &timeinfo);
    }
    else {
	FTB_WARNING("Failed to get the current time for the process.");
    }
}


int FTBM_Get_catcher_comp_list(const FTB_event_t * event, FTB_id_t ** list, int *len)
{
    FTBU_map_iterator_t iter_comp;
    FTBU_map_iterator_t iter_mask;
    FTBMI_map_ftb_id_2_comp_info_t *catcher_set;
    int temp_len = 0;

    if (!FTBMI_initialized)
	return FTB_ERR_GENERAL;

    /*
     * Contruct a deliver set to keep track which components have already
     * gotten the event and which are not, to avoid duplication
     */
    FTB_INFO("FTBM_Get_catcher_comp_list In");
    catcher_set = (FTBMI_map_ftb_id_2_comp_info_t *) FTBU_map_init(FTBMI_util_is_equal_ftb_id);

    lock_manager();

    FTB_INFO("****Going through the list of catcher components");

    /* 
     * Masks are stored in my catch_event_map. Go through my
     * catch_event_map to check if (1) Event matches the mask and 
     * (2) Who all are interested in getting this event that matched the mask 
     */

    for (iter_mask = FTBU_map_begin(FTBMI_info.catch_event_map);
	 iter_mask != FTBU_map_end(FTBMI_info.catch_event_map);
	 iter_mask = FTBU_map_next_iterator(iter_mask)) {
	FTB_event_t *mask = (FTB_event_t *) FTBU_map_get_key(iter_mask).key_ptr;

	if (FTBU_match_mask(event, mask)) {

	    FTB_INFO("This event has matched a mask stored in my catch_event_map. Get the list of clients who are interested in this event");

	    FTBU_map_node_t *map = (FTBU_map_node_t *) FTBU_map_get_data(iter_mask);

	    /* 
	     * The data part of the catch_event map is another linked
	     * list that contains the list of destinations interested in
	     * receiving this event
	     */

	    for (iter_comp = FTBU_map_begin(map); iter_comp !=FTBU_map_end(map);
		 iter_comp = FTBU_map_next_iterator(iter_comp)) {
		FTBMI_comp_info_t *comp = (FTBMI_comp_info_t *) FTBU_map_get_data(iter_comp);

		/*
 		 * This event could have matched many masks registered by the same client. 
 		 * Check and see if this component/client was already selected and put in the 
 		 * catcher list in one of the previous iterations of these for loops
 		 */

		FTB_INFO("Test whether component is already in catcher set");

		if (FTBU_map_find(catcher_set, FTBU_MAP_PTR_KEY(&comp->id)) !=
		    FTBU_map_end(catcher_set)) {
		    FTB_INFO("Component has already counted in to receive this event. ");
		    continue;
		}
		temp_len++;

		/*
 		 * If compnent/client is not a part of the catcher list, then make it a part now by adding its FTB_idS
 		 */
                FTB_INFO("Going to call FTBU_map_insert() to insert as key=comp->id, data=comp->id map=catcher_set");
		FTBU_map_insert(catcher_set, FTBU_MAP_PTR_KEY(&comp->id), (void *) &comp->id);
	    }
	}
    }
    unlock_manager();

    *list = (FTB_id_t *) malloc(sizeof(FTB_id_t) * temp_len);
    *len = temp_len;

    /*
     * Go through the catcher_set that was created - and copy the entire list in the calleefunction-passed argument for the list. The list
     * is a collection of FTB_ids.
     * Also get the total number of destinations, to pass it back to the callee function
     */
    for (iter_comp = FTBU_map_begin(catcher_set); iter_comp != FTBU_map_end(catcher_set);
	 iter_comp = FTBU_map_next_iterator(iter_comp)) {
	FTB_id_t *id = (FTB_id_t *) FTBU_map_get_data(iter_comp);
	temp_len--;
	memcpy(&((*list)[temp_len]), id, sizeof(FTB_id_t));
	FTB_INFO("For this mask - send events to destination=%s client_name=%s\n", id->location_id.hostname, id->client_id.client_name);
    }

    /*
     * Destroy the catcher_set map for now
     */
    FTBU_map_finalize(catcher_set);

    FTB_INFO("FTBM_Get_catcher_comp_list Out");
    return FTB_SUCCESS;
}


int FTBM_Release_comp_list(FTB_id_t * list)
{
    FTB_INFO("FTBM_Release_comp_list In");
    free(list);
    FTB_INFO("FTBM_Release_comp_list Out");
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

    FTB_INFO("FTBM_Init In");

    FTBMI_info.leaf = leaf;
    if (!leaf) {
	FTBMI_info.peers = (FTBMI_map_ftb_id_2_comp_info_t *) FTBU_map_init(FTBMI_util_is_equal_ftb_id);
	FTBMI_info.catch_event_map =(FTBMI_map_event_mask_2_comp_info_map_t *) FTBU_map_init(FTBMI_util_is_equal_event_mask);
	//FTBMI_info.catch_event_map =(FTBMI_map_event_mask_2_comp_info_map_t *) FTBU_map_init(FTBMI_util_is_equal_event);
	FTBMI_info.err_handling = FTB_ERR_HANDLE_RECOVER;
    }
    else {
	FTBMI_info.peers = NULL;
	FTBMI_info.catch_event_map = NULL;
	FTBMI_info.err_handling = FTB_ERR_HANDLE_NONE;	/* May change to recover mode if one client component requires so */
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

    FTBN_Connect(&msg, &FTBMI_info.parent);
    if (leaf && FTBMI_info.parent.pid == 0) {
	FTB_WARNING("Can not connect to any ftb agent");
	FTB_INFO("FTBM_Init Out - could not connect to any ftb agent");
	return FTB_ERR_GENERAL;
    }

    if (!leaf && FTBMI_info.parent.pid != 0) {
	FTBMI_comp_info_t *comp;
	FTB_INFO("Adding parent to peers");
	comp = (FTBMI_comp_info_t *) malloc(sizeof(FTBMI_comp_info_t));
	memcpy(&comp->id.location_id, &FTBMI_info.parent, sizeof(FTB_location_id_t));
	strcpy(comp->id.client_id.comp_cat, "FTB_COMP_CAT_BACKPLANE");
	strcpy(comp->id.client_id.comp, "FTB_COMP_MANAGER");
	comp->id.client_id.ext = 0;
	pthread_mutex_init(&comp->lock, NULL);
	//comp->catch_event_set = (FTBMI_set_event_mask_t *) FTBU_map_init(FTBMI_util_is_equal_event);
	comp->catch_event_set = (FTBMI_set_event_mask_t *) FTBU_map_init(FTBMI_util_is_equal_event_mask);

        FTB_INFO("Agent is going to call FTBU_map_insert() to insert as key=comp_id, data=comp(parents) map=FTBMI_info.peers");
	if (FTBU_map_insert(FTBMI_info.peers, FTBU_MAP_PTR_KEY(&comp->id), (void *) comp) == FTBU_EXIST) {
            FTB_INFO("FTBM_Init Out - client already present in the peer list");
	    return FTB_ERR_GENERAL;
	}
    }

    FTBMI_info.waiting = 0;
    lock_manager();
    FTBMI_initialized = 1;
    unlock_manager();
    FTB_INFO("FTBM_Init Out - success");
    return FTB_SUCCESS;
}


/* Called when the ftb node get disconnected */
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
	for (; iter != FTBU_map_end(FTBMI_info.peers); iter = FTBU_map_next_iterator(iter)) {
	    FTBMI_comp_info_t *comp = (FTBMI_comp_info_t *) FTBU_map_get_data(iter);
	    FTBMI_util_clean_component(comp);

	    if ((strcasecmp(comp->id.client_id.comp_cat, "FTB_COMP_CAT_BACKPLANE") == 0)
		&& (strcasecmp(comp->id.client_id.comp, "FTB_COMP_MANAGER") == 0)
		&& (comp->id.client_id.ext == 0)) {
		int ret;
		FTBM_msg_t msg;
		FTB_INFO("deregister from peer");
		memcpy(&msg.src, &FTBMI_info.self, sizeof(FTB_id_t));
		memcpy(&msg.dst, &comp->id, sizeof(FTB_id_t));
		msg.msg_type = FTBM_MSG_TYPE_CLIENT_DEREG;
		ret = FTBN_Send_msg(&msg);
		if (ret != FTB_SUCCESS)
		    FTB_WARNING("FTBN_Send_msg failed %d", ret);
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
	msg.msg_type = FTBM_MSG_TYPE_CLIENT_DEREG;
	ret = FTBN_Send_msg(&msg);
	if (ret != FTB_SUCCESS)
	    FTB_WARNING("FTBN_Send_msg failed %d", ret);
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
	for (; iter != FTBU_map_end(FTBMI_info.peers); iter = FTBU_map_next_iterator(iter)) {
	    FTBMI_comp_info_t *comp = (FTBMI_comp_info_t *) FTBU_map_get_data(iter);
	    free(comp);
	}

	FTBU_map_finalize(FTBMI_info.peers);
	FTBU_map_finalize(FTBMI_info.catch_event_map);
    }

    FTBN_Abort();
    FTB_INFO("FTBM_Abort Out");
    return FTB_SUCCESS;
}


int FTBM_Client_register(const FTB_id_t * id)
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

    comp = (FTBMI_comp_info_t *) malloc(sizeof(FTBMI_comp_info_t));
    memcpy(&comp->id, id, sizeof(FTB_id_t));
    pthread_mutex_init(&comp->lock, NULL);
    //comp->catch_event_set = (FTBMI_set_event_mask_t *) FTBU_map_init(FTBMI_util_is_equal_event);
    comp->catch_event_set = (FTBMI_set_event_mask_t *) FTBU_map_init(FTBMI_util_is_equal_event_mask);

    lock_manager();

    FTB_INFO("Going to call FTBU_map_insert() to insert as key=comp_id, data=comp and map=FTBMI_info.peers -- for the new Client_register request");

    if (FTBU_map_insert(FTBMI_info.peers, FTBU_MAP_PTR_KEY(&comp->id), (void *) comp) == FTBU_EXIST) {
	FTB_WARNING("Client already registered as a peer");
	unlock_manager();
	FTB_INFO("FTBM_Client_register Out");
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

	FTBU_map_iterator_t iter;
	for (iter = FTBU_map_begin(FTBMI_info.catch_event_map);
	     iter != FTBU_map_end(FTBMI_info.catch_event_map); iter = FTBU_map_next_iterator(iter)) {
	    FTB_event_t *mask = (FTB_event_t *) FTBU_map_get_key(iter).key_ptr;
	    memcpy(&msg.event, mask, sizeof(FTB_event_t));
	    ret = FTBN_Send_msg(&msg);
	    if (ret != FTB_SUCCESS) {
		FTB_WARNING("FTBN_Send_msg failed");
	    }
	}
    }
    FTB_INFO
	("new client comp:%s comp_cat:%s client_name:%s ext:%d registered, from host %s, pid %d pid starttime=%s",
	 comp->id.client_id.comp, comp->id.client_id.comp_cat, comp->id.client_id.client_name,
	 comp->id.client_id.ext, comp->id.location_id.hostname, comp->id.location_id.pid,
	 comp->id.location_id.pid_starttime);

    FTB_INFO("FTBM_Client_register Out");
    return FTB_SUCCESS;
}


int FTBM_Client_deregister(const FTB_id_t * id)
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
	     comp->id.client_id.comp, comp->id.client_id.comp_cat, comp->id.client_id.client_name,
	     comp->id.client_id.ext, comp->id.location_id.hostname, comp->id.location_id.pid);

    lock_comp(comp);
    lock_manager();
    FTBMI_util_clean_component(comp);
    if ((strcmp(comp->id.client_id.comp_cat, "FTB_COMP_CAT_BACKPLANE") == 0)
	&& (strcmp(comp->id.client_id.comp, "FTB_COMP_MANAGER") == 0)) {
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


int FTBM_Register_publishable_event(const FTB_id_t * id, FTB_event_t * event)
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

    /*Currently not doing anything */

    FTB_INFO("FTBM_Register_publishable_event Out");
    return FTB_SUCCESS;
}

int FTBM_Register_subscription(const FTB_id_t * id, FTB_event_t * event)
{
    FTBU_map_iterator_t iter;
    FTB_event_t *new_mask_comp;
    FTB_event_t *new_mask_manager;
    FTBMI_comp_info_t *comp;
    if (!FTBMI_initialized)
	return FTB_ERR_GENERAL;
    FTB_INFO("FTBM_Register_subscription In");
    if (FTBMI_info.leaf) {
	FTB_INFO("FTBM_Register_subscription Out - Not supported error");
	return FTB_ERR_NOT_SUPPORTED;
    }
    comp = FTBMI_lookup_component(id);
    if (comp == NULL) {
	FTB_INFO("FTBM_Register_subscription Out - Invalid parameter error");
	return FTB_ERR_INVALID_PARAMETER;
    }

    /* Add to component structure */
    new_mask_comp = (FTB_event_t *) malloc(sizeof(FTB_event_t));
    memcpy(new_mask_comp, event, sizeof(FTB_event_t));
    lock_comp(comp);


    /*
     * This event variable(which is a mask) is added to the      
     * catch_event_set map, which is a linked list consisting of all the
     * masks available with this agent.
     */

    FTB_INFO("Going to call FTBU_map_insert() to insert as key=new_mask, data=new_mask, map=catch_event_set");
    if (FTBU_map_insert(comp->catch_event_set, FTBU_MAP_PTR_KEY(new_mask_comp), (void *) new_mask_comp)
	== FTBU_EXIST) {
	free(new_mask_comp);
	FTB_INFO("FTBM_Register_subscription Out");
    }

    /*
     * Every mask in the catch_event_set is also present in the catch_event_map map.
     * The catch_event_map map uses the 'mask' as the key and points to a map of 'FTB_ids'. 
     * These FTB_ids is the collection of ftb-ids keys who are interested in receiving an event that
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
    iter = FTBU_map_find(FTBMI_info.catch_event_map, FTBU_MAP_PTR_KEY(new_mask_comp));

    if (iter == FTBU_map_end(FTBMI_info.catch_event_map)) {
	FTBMI_map_ftb_id_2_comp_info_t *new_map;

	FTB_INFO("New event to catch for the manager");
	new_mask_manager = (FTB_event_t *) malloc(sizeof(FTB_event_t));
	memcpy(new_mask_manager, new_mask_comp, sizeof(FTB_event_t));

	new_map = (FTBMI_map_ftb_id_2_comp_info_t *) FTBU_map_init(FTBMI_util_is_equal_ftb_id);
        
	FTB_INFO("Going to call FTBU_map_insert() to insert as key=new_mask_manager(which is alias for new_mask), data=new_map(potential list of FTB_ids interested in this mask in the future), map=catch_event_map");
	FTBU_map_insert(FTBMI_info.catch_event_map, FTBU_MAP_PTR_KEY(new_mask_manager),
			(void *) new_map);

        FTB_INFO("Going to call FTBU_map_insert() to insert key=comp->id (of my above first client), data=comp, map=new_map(collection of ftb_ids).");
	FTBU_map_insert(new_map, FTBU_MAP_PTR_KEY(&comp->id), (void *) comp);

	/*notify other FTB nodes about catch */
	FTBMI_util_reg_propagation(FTBM_MSG_TYPE_REG_SUBSCRIPTION, event, &id->location_id);
    }
    else {
	FTBMI_map_ftb_id_2_comp_info_t *map;
	FTB_INFO("The manager is already catching the event, adding the component");
	map = (FTBMI_map_ftb_id_2_comp_info_t *) FTBU_map_get_data(iter);
	FTB_INFO("Going to call FTBU_map_insert() to insert key=comp_id, data=comp, map=FTB_id map which is present in the data part of the catch_event_map");
	if (FTBU_map_insert(map, FTBU_MAP_PTR_KEY(&comp->id), (void *) comp) == FTBU_EXIST) {
		FTB_WARNING("This Mask already registered");
	}
    }

    unlock_manager();
    unlock_comp(comp);

    FTB_INFO("FTBM_Register_subscription Out");

    return FTB_SUCCESS;
}

int FTBM_Publishable_event_registration_cancel(const FTB_id_t * id, FTB_event_t * event)
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

    /*Currently not doing anything */

    FTB_INFO("FTBM_Publishable_event_registration_cancel Out");
    return FTB_SUCCESS;
}

int FTBM_Cancel_subscription(const FTB_id_t * id, FTB_event_t * event)
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
    if (iter == FTBU_map_end(comp->catch_event_set)) {
	FTB_WARNING("Not found throw entry");
	FTB_INFO("FTBM_Cancel_subscription Out");
	return FTB_SUCCESS;
    }
    mask = (FTB_event_t *) FTBU_map_get_data(iter);
    FTBU_map_remove_iter(iter);
    lock_manager();
    FTBMI_util_remove_com_from_catch_map(comp, mask);
    unlock_manager();
    free(mask);
    unlock_comp(comp);
    FTB_INFO("FTBM_Cancel_subscription Out");
    return FTB_SUCCESS;
}

int FTBM_Send(const FTBM_msg_t * msg)
{
    int ret;

    if (!FTBMI_initialized)
	return FTB_ERR_GENERAL;
    FTB_INFO("FTBM_Send In");

    ret = FTBN_Send_msg(msg);
    if (ret != FTB_SUCCESS) {
	if (!FTBMI_info.leaf) {	//For an agent
	    FTBMI_comp_info_t *comp;
	    comp = FTBMI_lookup_component(&msg->dst);
	    FTB_INFO("client %s:%s:%d from host %s pid %d, clean up",
		     comp->id.client_id.comp, comp->id.client_id.comp_cat, comp->id.client_id.ext,
		     comp->id.location_id.hostname, comp->id.location_id.pid);
	    lock_comp(comp);
	    lock_manager();
	    FTBMI_util_clean_component(comp);
	    if ((strcasecmp(comp->id.client_id.comp_cat, "FTB_COMP_CAT_BACKPLANE") == 0)
		&& (strcasecmp(comp->id.client_id.comp, "FTB_COMP_MANAGER") == 0)) {
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

int FTBM_Poll(FTBM_msg_t * msg, FTB_location_id_t * incoming_src)
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
	FTB_WARNING("FTBN_Recv_msg failed %d", ret);
    }
    FTB_INFO("FTBM_Poll Out");
    return ret;
}

int FTBM_Wait(FTBM_msg_t * msg, FTB_location_id_t * incoming_src)
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
	FTB_WARNING("FTBN_Recv_msg failed %d", ret);
    }

    lock_network();
    FTBMI_info.waiting = 0;
    unlock_network();

    FTB_INFO("FTBM_Wait Out");
    return ret;
}
