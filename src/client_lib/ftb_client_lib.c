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

#include "ftb_auxil.h"
#include "ftb_util.h"
#include "ftbm.h"

#define FTBCI_MAX_SUBSCRIPTION_FIELDS 10
#define FTBCI_MAX_EVENTS_PER_CLIENT 100
#define FTBCI_NUM_ERROR_CLASS 61440
#define FTBCI_NUM_ERROR_TYPE 4095


extern FILE *FTBU_log_file_fp;

/* The variable FTBU_map_node_t is redefined to the following for code clarity sake */
typedef FTBU_map_node_t FTBCI_map_mask_2_callback_entry_t;
typedef FTBU_map_node_t FTBCI_map_publishable_event_index_to_event_details;
typedef FTBU_map_node_t FTBCI_map_client_handle_to_client_info_t;

typedef struct FTBCI_event_inst_list {
    FTB_event_t event_inst;
    FTB_id_t src;
} FTBCI_event_inst_list_t;

typedef struct FTBCI_callback_entry {
    FTB_event_t *mask;
    int (*callback) (FTB_receive_event_t *, void *);
    void *arg;
} FTBCI_callback_entry_t;

typedef struct FTBCI_polling_entry {
    FTB_event_t *mask;
} FTBCI_polling_entry_t;


typedef struct FTBCI_client_info {
    FTB_client_handle_t client_handle;
    FTB_id_t *id;
    FTB_client_jobid_t jobid;
    uint8_t subscription_type;
    uint8_t err_handling;
    int max_polling_queue_len;
    int event_queue_size;
    FTBU_list_node_t *event_queue;  /* entry type: event_inst_list */
    FTBU_list_node_t *callback_event_queue; /* entry type: event_inst_list */

    FTBCI_map_mask_2_callback_entry_t *polling_map;
    FTBCI_map_mask_2_callback_entry_t *callback_map;
    FTBCI_map_publishable_event_index_to_event_details *declared_events_map;
    int total_publish_events;
    uint16_t seqnum;
    pthread_mutex_t lock;
    pthread_cond_t cond;
    pthread_t callback;
    volatile int finalizing;
} FTBCI_client_info_t;

/*
 * Currently the FTBCI_publish_event_index consists only of the event name.
 * We are keeping it as a structure in case we want to add something else tomorrow
 */
typedef struct FTBCI_publish_event_index {
    FTB_event_name_t event_name;
} FTBCI_publish_event_index_t;

typedef struct FTBCI_publish_event_details {
    FTB_severity_t severity;
} FTBCI_publish_event_details_t;

static pthread_mutex_t FTBCI_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_t callback_thread;
static FTBCI_map_client_handle_to_client_info_t *FTBCI_client_info_map = NULL;

static int enable_callback = 0;
static int num_components = 0;

static inline void FTBCI_lock_client_lib()
{
    pthread_mutex_lock(&FTBCI_lock);
}


static inline void FTBCI_unlock_client_lib()
{
    pthread_mutex_unlock(&FTBCI_lock);
}


static inline void FTBCI_lock_client(FTBCI_client_info_t * client_info)
{
    pthread_mutex_lock(&client_info->lock);
}


static inline void FTBCI_unlock_client(FTBCI_client_info_t * client_info)
{
    pthread_mutex_unlock(&client_info->lock);
}


int FTBCI_util_is_equal_declared_event_index(const void *lhs_void, const void *rhs_void)
{
    FTBCI_publish_event_index_t *lhs = (FTBCI_publish_event_index_t *) lhs_void;
    FTBCI_publish_event_index_t *rhs = (FTBCI_publish_event_index_t *) rhs_void;
    if (strcasecmp(lhs->event_name, rhs->event_name) == 0)
        return 1;
    else
        return 0;

}

int FTBCI_util_is_equal_event(const void *lhs_void, const void *rhs_void)
{
    FTB_event_t *lhs = (FTB_event_t *) lhs_void;
    FTB_event_t *rhs = (FTB_event_t *) rhs_void;
    return FTBU_is_equal_event(lhs, rhs);
}


int FTBCI_util_is_equal_clienthandle(const void *lhs_void, const void *rhs_void)
{
    FTB_client_handle_t *lhs = (FTB_client_handle_t *) lhs_void;
    FTB_client_handle_t *rhs = (FTB_client_handle_t *) rhs_void;
    return FTBU_is_equal_clienthandle(lhs, rhs);
}


#define FTBCI_LOOKUP_CLIENT_INFO(handle, client_info)  do {\
    FTBU_map_node_t *iter;\
    if (FTBCI_client_info_map == NULL) {\
        FTBU_WARNING("Not initialized");\
        return FTB_ERR_GENERAL;\
    }\
    FTBCI_lock_client_lib();\
    iter = FTBU_map_find_key(FTBCI_client_info_map, FTBU_MAP_PTR_KEY(&handle));\
    if (iter == FTBU_map_end(FTBCI_client_info_map)) {\
        FTBU_WARNING("Not registered");\
        FTBCI_unlock_client_lib();\
        return FTB_ERR_INVALID_HANDLE;\
    }\
    client_info = (FTBCI_client_info_t *)FTBU_map_get_data(iter); \
    FTBCI_unlock_client_lib();\
} while (0)


static int FTBCI_convert_clientid_to_clienthandle(const FTB_client_id_t client_id,
                                                  FTB_client_handle_t * client_handle)
{
    client_handle->valid = 1;
    memcpy(&client_handle->client_id, &client_id, sizeof(FTB_client_id_t));
    return 1;
}


int FTBCI_split_namespace(const char *event_space, char *region_name, char *category_name,
                          char *component_name)
{
    char *tempstr, *ptr, *str;

    FTBU_INFO("In FTBCI_split_namespace");

    if (strlen(event_space) >= FTB_MAX_EVENTSPACE) {
        FTBU_INFO("Out FTBCI_split_namespace");
        return FTB_ERR_EVENTSPACE_FORMAT;
    }

    if ((tempstr = (char *) malloc(strlen(event_space) + 1)) == NULL) {
        FTBU_INFO("Malloc error in FTB library");
        return (FTB_ERR_CLASS_FATAL + FTB_ERR_MALLOC);
    }
    ptr = tempstr;
    strcpy(tempstr, event_space);

    str = strsep(&tempstr, ".");
    if ((strcmp(str, "\0") == 0) || (strcmp(tempstr, "\0") == 0)) {
        free(ptr);
        FTBU_INFO("Out FTBCI_split_namespace");
        return FTB_ERR_EVENTSPACE_FORMAT;
    }
    strcpy(region_name, str);

    str = strsep(&tempstr, ".");
    if ((strcmp(str, "\0") == 0) || (strcmp(tempstr, "\0") == 0)) {
        free(ptr);
        FTBU_INFO("Out FTBCI_split_namespace");
        return FTB_ERR_EVENTSPACE_FORMAT;
    }
    strcpy(category_name, str);

    str = strsep(&tempstr, ".");
    if (strcmp(str, "\0") == 0) {
        free(ptr);
        FTBU_INFO("Out FTBCI_split_namespace");
        return FTB_ERR_EVENTSPACE_FORMAT;
    }
    strcpy(component_name, str);

    if ((tempstr != NULL)
        || (!check_alphanumeric_underscore_format(region_name))
        || (!check_alphanumeric_underscore_format(category_name))
        || (!check_alphanumeric_underscore_format(component_name))) {
        free(ptr);
        FTBU_INFO("Out FTBCI_split_namespace");
        return FTB_ERR_EVENTSPACE_FORMAT;
    }

    if (ptr != NULL) {
        free(ptr);
    }

    FTBU_INFO("Out FTBCI_split_namespace");

    return FTB_SUCCESS;
}


int FTBCI_check_subscription_value_pair(const char *lhs, const char *rhs,
                                        FTB_event_t * subscription_event)
{
    static uint8_t track;
    int ret = 0;
    FTBU_INFO("In FTBCI_check_subscription_value_pair");
    if ((strcasecmp(lhs, "") == 0) && (strcasecmp(rhs, "") == 0)) {
        track = 1;
        strcpy(subscription_event->severity, "all");
        strcpy(subscription_event->region, "all");
        strcpy(subscription_event->comp_cat, "all");
        strcpy(subscription_event->comp, "all");
        strcpy(subscription_event->event_name, "all");
        strcpy(subscription_event->client_jobid, "all");
        strcpy(subscription_event->hostname, "all");
        strcpy(subscription_event->client_name, "all");
    }
    else if (strcasecmp(lhs, "severity") == 0) {
        if (track & 2)
            return FTB_ERR_SUBSCRIPTION_STR;
        track = track | 2;
        if ((strcasecmp(rhs, "fatal") == 0) ||
            (strcasecmp(rhs, "info") == 0) ||
            (strcasecmp(rhs, "warning") == 0) ||
            (strcasecmp(rhs, "error") == 0) || (strcasecmp(rhs, "all") == 0))
            strcpy(subscription_event->severity, rhs);
        else {
            FTBU_INFO("Out FTBCI_check_subscription_value_pair");
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
            FTBU_INFO("Out FTBCI_check_subscription_value_pair");
            return FTB_ERR_EVENTSPACE_FORMAT;
        }
        ret = FTBCI_split_namespace(rhs, region, comp_cat, comp_name);
        if (ret != FTB_SUCCESS) {
            FTBU_INFO("Out FTBCI_check_subscription_value_pair");
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
        if (strlen(rhs) >= FTB_MAX_CLIENT_JOBID) {
            FTBU_INFO("Out FTBCI_check_subscription_value_pair");
            return FTB_ERR_FILTER_VALUE;
        }
        strcpy(subscription_event->client_jobid, rhs);
    }
    else if (strcasecmp(lhs, "host_name") == 0) {
        if (track & 16)
            return FTB_ERR_SUBSCRIPTION_STR;
        track = track | 16;
        if (strlen(rhs) >= FTB_MAX_HOST_ADDR) {
            FTBU_INFO("Out FTBCI_check_subscription_value_pair");
            return FTB_ERR_FILTER_VALUE;
        }
        strcpy(subscription_event->hostname, rhs);
    }
    else if (strcasecmp(lhs, "event_name") == 0) {
        if (track & 32)
            return FTB_ERR_SUBSCRIPTION_STR;
        track = track | 32;
        if ((strlen(rhs) >= FTB_MAX_EVENT_NAME)
            || (!check_alphanumeric_underscore_format(lhs))) {
            FTBU_INFO("Out FTBCI_check_subscription_value_pair");
            return FTB_ERR_FILTER_VALUE;
        }
        strcpy(subscription_event->event_name, rhs);
    }
    else if (strcasecmp(lhs, "client_name") == 0) {
        if (track & 64)
            return FTB_ERR_SUBSCRIPTION_STR;
        track = track | 64;
        if ((strlen(rhs) >= FTB_MAX_CLIENT_NAME)
            || (!check_alphanumeric_underscore_format(lhs))) {
            FTBU_INFO("Out FTBCI_check_subscription_value_pair");
            return FTB_ERR_FILTER_VALUE;
        }
        strcpy(subscription_event->client_name, rhs);
    }
    else {
        FTBU_INFO("Out FTBCI_check_subscription_value_pair");
        return FTB_ERR_FILTER_ATTR;
    }
    FTBU_INFO("Out FTBCI_check_subscription_value_pair");
    return FTB_SUCCESS;
}


int FTBCI_parse_subscription_string(const char *subscription_str, FTB_event_t * subscription_event)
{

    int subscriptionstr_len;
    char *tempstr, *ptr;
    char *pair[FTBCI_MAX_SUBSCRIPTION_FIELDS], *lhs, *rhs;
    int i = 0, j = 0, k = 0, ret = 0;

    FTBU_INFO("FTBCI_parse_subscription_string In");
    if (subscription_str == NULL) {
        FTBU_INFO("FTBCI_parse_subscription_string Out");
        return FTB_ERR_SUBSCRIPTION_STR;
    }

    if ((tempstr = (char *) malloc(strlen(subscription_str) + 1)) == NULL) {
        FTBU_INFO("Malloc error in FTB library");
        return (FTB_ERR_CLASS_FATAL + FTB_ERR_MALLOC);
    }
    strcpy(tempstr, subscription_str);
    ptr = tempstr;

    ret = FTBCI_check_subscription_value_pair("", "", subscription_event);
    if ((subscriptionstr_len = strlen(tempstr)) == 0) {
        free(ptr);
        return ret;
    }

    while (tempstr != NULL) {
        if ((pair[i++] = strsep(&tempstr, ",")) == NULL) {
            FTBU_INFO("FTBCI_parse_subscription_string Out");
            free(ptr);
            return FTB_ERR_SUBSCRIPTION_STR;
        }
    }

    if (i > FTBCI_MAX_SUBSCRIPTION_FIELDS) {
        FTBU_INFO("Subscription string has too many fields. This maybe an internal FTB error");
        FTBU_INFO("FTBCI_parse_subscription_string Out");
        free(ptr);
        return FTB_ERR_SUBSCRIPTION_STR;
    }

    for (j = 0; j < i; j++) {
        if ((lhs = strsep(&pair[j], "=")) == NULL) {
            free(ptr);
            FTBU_INFO("FTBCI_parse_subscription_string Out");
            return FTB_ERR_SUBSCRIPTION_STR;
        }

        if ((rhs = strsep(&pair[j], "=")) == NULL) {
            free(ptr);
            FTBU_INFO("FTBCI_parse_subscription_string Out");
            return FTB_ERR_SUBSCRIPTION_STR;
        }

        soft_trim(&lhs);
        soft_trim(&rhs);

        if (strlen(lhs) == 0 || strlen(rhs) == 0) {
            free(ptr);
            FTBU_INFO("FTBCI_parse_subscription_string Out");
            return FTB_ERR_SUBSCRIPTION_STR;
        }

        for (k = 0; k < strlen(lhs); k++) {
            if (lhs[k] == ' ' || lhs[k] == '\t') {
                free(ptr);
                FTBU_INFO("FTBCI_parse_subscription_string Out");
                return FTB_ERR_FILTER_ATTR;
            }
        }

        for (k = 0; k < strlen(rhs); k++) {
            if (rhs[k] == ' ' || rhs[k] == '\t') {
                free(ptr);
                FTBU_INFO("FTBCI_parse_subscription_string Out");
                return FTB_ERR_FILTER_VALUE;
            }
        }

        if ((ret = FTBCI_check_subscription_value_pair(lhs, rhs, subscription_event)) != FTB_SUCCESS) {
            free(ptr);
            FTBU_INFO("FTBCI_parse_subscription_string Out");
            return ret;
        }
    }

    free(ptr);
    FTBU_INFO("FTBCI_parse_subscription_string Out");
    return FTB_SUCCESS;
}


int FTBCI_check_severity_values(const FTB_severity_t severity)
{
    if ((strcasecmp(severity, "fatal") == 0) ||
        (strcasecmp(severity, "info") == 0) ||
        (strcasecmp(severity, "warning") == 0) || (strcasecmp(severity, "error") == 0))
        return 1;
    else
        return 0;
}

int FTBCI_store_declared_events(FTBCI_client_info_t * client_info, const FTB_event_info_t * event_table,
                                int num_events)
{
    int i = 0;

    FTBU_INFO("In FTBCI_store_declared_events");
    if (num_events == 0) {
        FTBU_WARNING("0 events being registered!");
        FTBU_INFO("Out FTBCI_store_declared_events");
        return FTB_SUCCESS;
    }

    for (i = 0; i < num_events; i++) {
        FTBU_map_node_t *iter;
        FTBCI_publish_event_index_t *event_index;
        FTBCI_publish_event_details_t *event_details;

        FTBCI_lock_client(client_info);
        if (client_info->total_publish_events >= FTBCI_MAX_EVENTS_PER_CLIENT) {
            FTBCI_unlock_client(client_info);
            FTBU_INFO("Out FTBCI_store_declared_events");
            return FTB_ERR_GENERAL;
        }
        FTBCI_unlock_client(client_info);
        if ((strlen(event_table[i].event_name) >= FTB_MAX_EVENT_NAME)
            || (!check_alphanumeric_underscore_format(event_table[i].event_name))) {
            FTBU_INFO("Out FTBCI_store_declared_events");
            return FTB_ERR_INVALID_FIELD;
        }
        if (!FTBCI_check_severity_values(event_table[i].severity)) {
            FTBU_INFO("Out FTBCI_store_declared_events");
            return FTB_ERR_INVALID_FIELD;
        }

        if ((event_index = (FTBCI_publish_event_index_t *) malloc(sizeof(FTBCI_publish_event_index_t))) == NULL) {
            FTBU_INFO("Malloc error in FTB library");
            return (FTB_ERR_CLASS_FATAL + FTB_ERR_MALLOC);
	}
        strcpy(event_index->event_name, event_table[i].event_name);

        FTBCI_lock_client(client_info);
        iter = FTBU_map_find_key(client_info->declared_events_map, FTBU_MAP_PTR_KEY(event_index));
        if (iter != FTBU_map_end(client_info->declared_events_map)) {
            free(event_index);
            FTBCI_unlock_client(client_info);
            FTBU_INFO("Out FTBCI_store_declared_events : Duplicate event");
            return FTB_SUCCESS;
            /* return FTB_ERR_DUP_EVENT; */
        }
        FTBCI_unlock_client(client_info);

        if ((event_details = (FTBCI_publish_event_details_t *) malloc(sizeof(FTBCI_publish_event_details_t))) == NULL) {
	    FTBU_INFO("Malloc error in FTB library");
            return (FTB_ERR_CLASS_FATAL + FTB_ERR_MALLOC);
	}
        strcpy(event_details->severity, event_table[i].severity);
        FTBU_INFO
            ("Inserting a declared event in the declared_event_map. Event name=%s and severity=%s\n",
             event_index->event_name, event_details->severity);
        FTBCI_lock_client(client_info);
        FTBU_map_insert(client_info->declared_events_map, FTBU_MAP_PTR_KEY(event_index),
                        (void *) event_details);
        client_info->total_publish_events++;
        FTBCI_unlock_client(client_info);
    }
    FTBU_INFO("Out FTBCI_store_declared_events");
    return FTB_SUCCESS;
}

static void FTBCI_util_cleanup_declared_events_map(FTBCI_client_info_t * client_info)
{
    FTBU_map_node_t *iter;

    FTBU_INFO("In FTBCI_util_cleanup_declared_events_map ");
    iter = FTBU_map_begin(client_info->declared_events_map);
    while (iter != FTBU_map_end(client_info->declared_events_map)) {
        FTBCI_publish_event_index_t *event_index =
            (FTBCI_publish_event_index_t *) FTBU_map_get_key(iter).key_ptr;
        FTBCI_publish_event_details_t *event_details =
            (FTBCI_publish_event_details_t *) FTBU_map_get_data(iter);
        free(event_index);
        free(event_details);
        iter = FTBU_map_next_node(iter);
    }
    FTBU_map_finalize(client_info->declared_events_map);
    FTBU_INFO("Out FTBCI_util_cleanup_declared_events_map ");

}


static void FTBCI_util_push_to_comp_polling_list(FTBCI_client_info_t * client_info,
                                                 FTBU_list_node_t * node)
{
    /*Assumes it holds the lock of that comp */
    FTBU_list_node_t *temp_node;

    if (client_info->subscription_type & FTB_SUBSCRIPTION_POLLING) {
        if (client_info->event_queue_size == client_info->max_polling_queue_len) {
            FTBU_WARNING("Event queue is full");
            /* Remove the first event in the component polling queue */
            temp_node = client_info->event_queue->next;
            FTBU_list_remove_node(temp_node);
            free(temp_node->data);
            free(temp_node);
            client_info->event_queue_size--;
        }
        FTBU_list_add_back(client_info->event_queue, node);
        client_info->event_queue_size++;
    }
    else {
        free(node->data);
        free(node);
    }
}

static int FTBCI_util_add_to_polling_map(FTBCI_client_info_t * client_info, const FTB_event_t * event)
{
    FTBU_INFO("In FTBCI_util_add_to_polling_map");
    FTBCI_polling_entry_t *entry;
    if ((entry = (FTBCI_polling_entry_t *) malloc(sizeof(FTBCI_polling_entry_t))) == NULL) {
	FTBU_INFO("Malloc error in FTB library");
        return (FTB_ERR_CLASS_FATAL + FTB_ERR_MALLOC);
    }
    if ((entry->mask = (FTB_event_t *) malloc(sizeof(FTB_event_t))) == NULL) {
	FTBU_INFO("Malloc error in FTB library");
        return (FTB_ERR_CLASS_FATAL + FTB_ERR_MALLOC);
    }
    memcpy(entry->mask, event, sizeof(FTB_event_t));

    FTBU_INFO
        ("Going to call FTBU_map_insert() to insert as key=polling_entry->mask, data=polling_entry map=client_info->polling_map");
    FTBCI_lock_client(client_info);
    FTBU_map_insert(client_info->polling_map, FTBU_MAP_PTR_KEY(entry->mask), (void *) entry);
    FTBCI_unlock_client(client_info);
    FTBU_INFO("Out FTBCI_util_add_to_polling_map");
    return FTB_SUCCESS;
}

static int FTBCI_util_add_to_callback_map(FTBCI_client_info_t * client_info, const FTB_event_t * event,
                                           int (*callback) (FTB_receive_event_t *, void *), void *arg)
{
    FTBU_INFO("In FTBCI_util_add_to_callback_map");
    FTBCI_callback_entry_t *entry;
    if ((entry = (FTBCI_callback_entry_t *) malloc(sizeof(FTBCI_callback_entry_t))) == NULL) {
	FTBU_INFO("Malloc error in FTB library");
        return (FTB_ERR_CLASS_FATAL + FTB_ERR_MALLOC);
    }
    if ((entry->mask = (FTB_event_t *) malloc(sizeof(FTB_event_t))) == NULL) {
        FTBU_INFO("Malloc error in FTB library");
        return (FTB_ERR_CLASS_FATAL + FTB_ERR_MALLOC);
    }
    memcpy(entry->mask, event, sizeof(FTB_event_t));
    entry->callback = callback;
    entry->arg = arg;
    FTBU_INFO
        ("Going to call FTBU_map_insert() to insert as key=callback_entry->mask, data=callback_entry map=client_info->callback_map");
    FTBCI_lock_client(client_info);
    FTBU_map_insert(client_info->callback_map, FTBU_MAP_PTR_KEY(entry->mask), (void *) entry);
    FTBCI_unlock_client(client_info);
    FTBU_INFO("Out FTBCI_util_add_to_callback_map");
    return FTB_SUCCESS;
}

#if 0
static void FTBCI_util_remove_from_callback_map(FTBCI_client_info_t * client_info,
                                                const FTB_event_t * event)
{
    if ((FTBCI_callback_entry_t *entry = (FTBCI_callback_entry_t *) malloc(sizeof(FTBCI_callback_entry_t))) == NULL) {
        FTBU_INFO("Malloc error in FTB library");
        return (FTB_ERR_CLASS_FATAL + FTB_ERR_MALLOC);
    }
    FTBU_INFO("In FTBCI_util_remove_from_callback_map");
    if ((entry->mask = (FTB_event_t *) malloc(sizeof(FTB_event_t))) == NULL) {
        FTBU_INFO("Malloc error in FTB library");
        return (FTB_ERR_CLASS_FATAL + FTB_ERR_MALLOC);
    }
    memcpy(entry->mask, event, sizeof(FTB_event_t));
    FTBCI_lock_client(client_info);
    FTBU_map_remove_key(client_info->callback_map, FTBU_MAP_PTR_KEY(entry->mask));
    FTBCI_unlock_client(client_info);
    /* The key should be found! If the subscribe handle's subscription_event
     * was invalid, it should have been caught before
     * if (ret == FTBU_NOT_EXIST) return FTB_ERR_INVALID_HANDLE;
     */
    free(entry->mask);
    free(entry);
    FTBU_INFO("Out FTBCI_util_remove_from_callback_map");
}
#endif

static void FTBCI_util_remove_from_subscription_map(FTBCI_client_info_t * client_info,
                                                const FTB_event_t * event)
{
    FTBU_INFO("In FTBCI_util_remove_from_subscription_map");
    FTBCI_lock_client(client_info);

    if ( client_info->polling_map != NULL )
      FTBU_map_remove_key(client_info->polling_map, FTBU_MAP_PTR_KEY(event));

    if ( client_info->callback_map != NULL )
      FTBU_map_remove_key(client_info->callback_map, FTBU_MAP_PTR_KEY(event));

    FTBCI_unlock_client(client_info);
    FTBU_INFO("Out FTBCI_util_remove_from_subscription_map");
}

static void FTBCI_util_reconnect()
{
    FTBCI_client_info_t *client_info;
    FTBCI_polling_entry_t *polling_entry;
    FTBCI_callback_entry_t *callback_entry;
    FTBU_map_node_t *iter, *siter;
    FTBM_msg_t msg;
    int ret;

    FTBCI_lock_client_lib();

    iter = FTBU_map_begin(FTBCI_client_info_map);
    
    while ( iter != FTBU_map_end(FTBCI_client_info_map)) {
      client_info = (FTBCI_client_info_t *) FTBU_map_get_data(iter);
      memcpy(&msg.src, client_info->id, sizeof(FTB_id_t));
      msg.msg_type = FTBM_MSG_TYPE_CLIENT_REG;
      FTBM_Get_parent_location_id(&msg.dst.location_id);
      ret = FTBM_Send(&msg);

      msg.msg_type = FTBM_MSG_TYPE_REG_SUBSCRIPTION;
      
      if ( client_info->polling_map != NULL ) {
	siter = FTBU_map_begin(client_info->polling_map);
	while ( siter != FTBU_map_end(client_info->polling_map) ) {
	  polling_entry = (FTBCI_polling_entry_t *) FTBU_map_get_data(siter);
	  memcpy(&msg.event, polling_entry->mask, sizeof(FTB_event_t));
	  ret = FTBM_Send(&msg);
	  siter = siter->next;
	}
      }

      if ( client_info->callback_map != NULL ) {
	siter = FTBU_map_begin(client_info->callback_map);
	while ( siter != FTBU_map_end(client_info->callback_map) ) {
	  callback_entry = (FTBCI_callback_entry_t *) FTBU_map_get_data(siter);
	  memcpy(&msg.event, callback_entry->mask, sizeof(FTB_event_t));
	  ret = FTBM_Send(&msg);
	  siter = siter->next;
	}
      }

      iter = iter->next;
    }

    FTBCI_unlock_client_lib();
}

static int FTBCI_util_handle_FTBM_msg(FTBM_msg_t * msg)
{
    FTB_client_handle_t client_handle;
    FTBCI_client_info_t *client_info;
    FTBU_map_node_t *iter;
    FTBU_list_node_t *node;
    FTBCI_event_inst_list_t *entry;

    FTBU_INFO("In FTBCI_util_handle_FTBM_msg");
    if (msg->msg_type != FTBM_MSG_TYPE_NOTIFY) {
        FTBU_WARNING("unexpected message type %d", msg->msg_type);
        return -1;
    }

    FTBCI_convert_clientid_to_clienthandle(msg->dst.client_id, &client_handle);

    FTBCI_lock_client_lib();
    iter = FTBU_map_find_key(FTBCI_client_info_map, FTBU_MAP_PTR_KEY(&client_handle));
    if (iter == FTBU_map_end(FTBCI_client_info_map)) {
        FTBU_WARNING("Message for unregistered component");
        FTBCI_unlock_client_lib();
        FTBU_INFO("Out FTBCI_util_remove_from_callback_map");
        return 0;
    }
    client_info = (FTBCI_client_info_t *) FTBU_map_get_data(iter);
    FTBCI_unlock_client_lib();

    if ((node = (FTBU_list_node_t *) malloc(sizeof(FTBU_list_node_t))) == NULL) {
        FTBU_INFO("Malloc error in FTB library");
        return (FTB_ERR_CLASS_FATAL + FTB_ERR_MALLOC);
    }
    if ((entry = (FTBCI_event_inst_list_t *) malloc(sizeof(FTBCI_event_inst_list_t))) == NULL) {
        FTBU_INFO("Malloc error in FTB library");
        return (FTB_ERR_CLASS_FATAL + FTB_ERR_MALLOC);
    }
    memcpy(&entry->event_inst, &msg->event, sizeof(FTB_event_t));
    memcpy(&entry->src, &msg->src, sizeof(FTB_id_t));
    node->data = (void *) entry;
    FTBCI_lock_client(client_info);
    if (client_info->subscription_type & FTB_SUBSCRIPTION_NOTIFY) {
        /*has notification thread */
        FTBU_INFO("Test whether belonging to any asynchronous notification subscription string");
        FTBU_map_node_t *iter;
        iter = FTBU_map_begin(client_info->callback_map);
        while (iter != FTBU_map_end(client_info->callback_map)) {
            FTBCI_callback_entry_t *callback_entry = (FTBCI_callback_entry_t *) FTBU_map_get_data(iter);
            if (FTBU_match_mask(&msg->event, callback_entry->mask)) {
		FTBU_list_node_t *node;
                if ((node = (FTBU_list_node_t *) malloc(sizeof(FTBU_list_node_t))) == NULL) {
		    FTBU_INFO("Malloc error in FTB library");
		    return (FTB_ERR_CLASS_FATAL + FTB_ERR_MALLOC);
		}
                if ((entry = (FTBCI_event_inst_list_t *) malloc(sizeof(FTBCI_event_inst_list_t))) == NULL) {
	            FTBU_INFO("Malloc error in FTB library");
		    return (FTB_ERR_CLASS_FATAL + FTB_ERR_MALLOC);
		}
                memcpy(&entry->event_inst, &msg->event, sizeof(FTB_event_t));
                memcpy(&entry->src, &msg->src, sizeof(FTB_id_t));
                node->data = (void *) entry;
                FTBU_INFO("Adding received event to the FTB client callback_event_queue");
                FTBU_list_add_back(client_info->callback_event_queue, node);
                pthread_cond_signal(&client_info->cond);
                FTBCI_unlock_client(client_info);
                FTBU_INFO("Out FTBCI_util_handle_FTBM_msg");
                return 0;
            }
            iter = FTBU_map_next_node(iter);
        }
    }
    FTBU_INFO("Did not find a matching asynchronous notification subscription string for the event");
    FTBU_INFO("Inserted event in polling queue for the client");
    /*
     * If event was not subscribed using an asynchronous notifcation subscription string,
     * then add the event to the polling queue.
     * Note that we are not currently checking if a mask matching an event was subscribed
     * using a polling subscription string - this needs to be taken care in the future
     */
    FTBCI_util_push_to_comp_polling_list(client_info, node);
    FTBCI_unlock_client(client_info);
    FTBU_INFO("Out FTBCI_util_handle_FTBM_msg");
    return 0;
}

int FTBCI_Check_progress()
{
    FTBM_msg_t msg;
    FTB_location_id_t incoming_src;
    int ret;

    FTBU_INFO("In FTBCI_Check_progress");
    if ( enable_callback > 0 ) {
        FTBU_INFO("Out FTBCI_Check_progress - a callback thread already present for progress check");
        return FTB_SUCCESS;
    }
    while ((ret = FTBM_Get(&msg, &incoming_src, 0)) == FTB_SUCCESS) {
        FTBCI_util_handle_FTBM_msg(&msg);
    }

    if ( ret == FTB_ERR_NETWORK_GENERAL ) {
      FTBU_WARNING("FTBM_Get failed %d", ret);
      FTBU_WARNING("Trying to reconnect ...");

      FTBCI_util_reconnect();
    }

    FTBU_INFO("Out FTBCI_Check_progress - after doing a progress check");
    return ret;
}


static void *FTBCI_callback_thread_client(void *arg)
{
    FTBM_msg_t msg;
    FTB_location_id_t incoming_src;
    int ret;

    FTBU_INFO("In FTBCI_callback_thread_client");

    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

    while (1) {
        ret = FTBM_Get(&msg, &incoming_src,1);

	if ( ret == FTB_ERR_NETWORK_GENERAL ) {
	  FTBU_WARNING("FTBM_Get failed %d", ret);
	  FTBU_WARNING("Trying to reconnect ...");
	  FTBU_INFO("Out FTBCI_callback_thread_client");
	  FTBCI_util_reconnect();
	  return NULL;
	}

        FTBCI_util_handle_FTBM_msg(&msg);
    }
    FTBU_INFO("Out FTBCI_callback_thread_client");
    return NULL;
}


static void *FTBCI_callback_component(void *arg)
{
    FTBCI_client_info_t *client_info = (FTBCI_client_info_t *) arg;
    FTBU_list_node_t *node;
    FTBCI_event_inst_list_t *entry;
    FTBU_map_node_t *iter;
    FTBCI_callback_entry_t *callback_entry;
    int callback_done;

    /* FTBCI_lock_client(client_info); */
    FTBU_INFO("In FTBCI_callback_component");

    while (1) {

        FTBCI_lock_client(client_info);
        while (client_info->callback_event_queue->next == client_info->callback_event_queue) {
            /*Callback event queue is empty */
            pthread_cond_wait(&client_info->cond, &client_info->lock);
            if (client_info->finalizing) {
                FTBCI_unlock_client(client_info);
                FTBU_INFO("Out FTBCI_callback_component");
                FTBU_INFO("Finalizing");
                return NULL;
            }
        }
        FTBCI_unlock_client(client_info);

        node = client_info->callback_event_queue->next;
        entry = (FTBCI_event_inst_list_t *) node->data;
        /*Try to match it with callback_map */
        callback_done = 0;

        iter = FTBU_map_begin(client_info->callback_map);
        while (iter != FTBU_map_end(client_info->callback_map)) {

            callback_entry = (FTBCI_callback_entry_t *) FTBU_map_get_data(iter);

            if (FTBU_match_mask(&entry->event_inst, callback_entry->mask)) {
                /*Make callback */
                FTB_receive_event_t receive_event;
                concatenate_strings(receive_event.event_space, entry->event_inst.region, ".",
                                    entry->event_inst.comp_cat, ".", entry->event_inst.comp, NULL);
                strcpy(receive_event.event_name, entry->event_inst.event_name);
                strcpy(receive_event.severity, entry->event_inst.severity);
                strcpy(receive_event.client_name, entry->event_inst.client_name);
                strcpy(receive_event.client_jobid, entry->event_inst.client_jobid);
                memcpy(&receive_event.client_extension, &entry->src.client_id.ext, sizeof(int));
                memcpy(&receive_event.seqnum, &entry->event_inst.seqnum, sizeof(int));
                memcpy(&receive_event.event_type, &entry->event_inst.event_type, sizeof(uint8_t));
                memcpy(receive_event.event_payload, entry->event_inst.event_payload,
                       FTB_MAX_PAYLOAD_DATA);
                memcpy(&receive_event.incoming_src, &entry->src.location_id, sizeof(FTB_location_id_t));
                (*callback_entry->callback) (&receive_event, callback_entry->arg);

                FTBCI_lock_client(client_info);
                FTBU_list_remove_node(node);
                FTBCI_unlock_client(client_info);

                free(entry);
                free(node);
                callback_done = 1;
                break;
            }
            iter = FTBU_map_next_node(iter);
        }

        if (!callback_done) {
            /*Move to polling event queue */
            FTBCI_lock_client(client_info);
            FTBU_list_remove_node(node);
            FTBCI_util_push_to_comp_polling_list(client_info, node);
            FTBCI_unlock_client(client_info);
            free(entry);
            free(node);
        }
    }

    /* FTBCI_unlock_client(client_info); */
    FTBU_INFO("Out FTBCI_callback_component");
    return NULL;
}


static void FTBCI_util_finalize_component(FTBCI_client_info_t * client_info)
{
    FTBU_INFO("In FTBCI_util_finalize_component");

    FTBM_Client_deregister(client_info->id);
    free(client_info->id);

    if (client_info->event_queue) {
        FTBU_list_node_t *temp;
        FTBU_list_node_t *pos;
        FTBU_list_for_each(pos, client_info->event_queue, temp) {
            FTBU_list_node_t *node = pos;
            free(node->data);
            free(node);
        }
        free(client_info->event_queue);
    }

    if (client_info->subscription_type & FTB_SUBSCRIPTION_NOTIFY) {
        FTBU_map_node_t *iter;
        client_info->finalizing = 1;

        FTBU_INFO("In FTBCI_util_finalize_component: signaling the callback subscriptions");
        pthread_cond_signal(&client_info->cond);
        FTBCI_unlock_client(client_info);
        pthread_join(client_info->callback, NULL);
        pthread_cond_destroy(&client_info->cond);

        FTBCI_lock_client(client_info);
        iter = FTBU_map_begin(client_info->callback_map);
        while (iter != FTBU_map_end(client_info->callback_map)) {
            FTBCI_callback_entry_t *entry = (FTBCI_callback_entry_t *) FTBU_map_get_data(iter);
            free(entry->mask);
            iter = FTBU_map_next_node(iter);
        }
        FTBU_map_finalize(client_info->callback_map);
        FTBCI_unlock_client(client_info);
    }

    FTBU_INFO("Out FTBCI_util_finalize_component");
}

int FTBC_Check_error_code (const int error_code, int *error_class, int *error_value) {
       *error_class = (error_code & FTBCI_NUM_ERROR_CLASS);
       *error_value = (error_code & FTBCI_NUM_ERROR_TYPE); 
       return FTB_SUCCESS;
}

int FTBC_Connect(FTB_client_t * cinfo, uint8_t extension, FTB_client_handle_t * client_handle)
{
    FTBCI_client_info_t *client_info;
    char category_name[FTB_MAX_EVENTSPACE];
    char component_name[FTB_MAX_EVENTSPACE];
    char region_name[FTB_MAX_EVENTSPACE];
    int ret;
    FTBM_msg_t msg;

    FTBU_INFO("FTBC_Connect In");

    if (client_handle == NULL) 
        return FTB_ERR_NULL_POINTER;

    if ((client_info = (FTBCI_client_info_t *) malloc(sizeof(FTBCI_client_info_t))) == NULL) {
	FTBU_INFO("Malloc error in FTB library");
        return (FTB_ERR_CLASS_FATAL + FTB_ERR_MALLOC);
    }
    memset(client_info, 0, sizeof(FTBCI_client_info_t));

    /* Set the subscription-related fields */
    if (strcasecmp(cinfo->client_subscription_style, "FTB_SUBSCRIPTION_POLLING") == 0) {
        client_info->subscription_type = FTB_SUBSCRIPTION_POLLING;
        if (cinfo->client_polling_queue_len > 0) {
            client_info->max_polling_queue_len = cinfo->client_polling_queue_len;
        }
        else {
            client_info->max_polling_queue_len = FTB_DEFAULT_POLLING_Q_LEN;
        }
    }
    else if (strcasecmp(cinfo->client_subscription_style, "FTB_SUBSCRIPTION_NONE") == 0) {
        client_info->subscription_type = FTB_SUBSCRIPTION_NONE;
        client_info->max_polling_queue_len = 0;
    }
    else if (strcasecmp(cinfo->client_subscription_style, "FTB_SUBSCRIPTION_NOTIFY") == 0) {
        client_info->subscription_type = FTB_SUBSCRIPTION_NOTIFY;
        client_info->max_polling_queue_len = 0;
    }
    else if (strcasecmp(cinfo->client_subscription_style, "FTB_SUBSCRIPTION_BOTH") == 0) {
        client_info->subscription_type = FTB_SUBSCRIPTION_POLLING + FTB_SUBSCRIPTION_NOTIFY;
        if (cinfo->client_polling_queue_len > 0) {
            client_info->max_polling_queue_len = cinfo->client_polling_queue_len;
        }
        else {
            client_info->max_polling_queue_len = FTB_DEFAULT_POLLING_Q_LEN;
        }
    }
    else {
        free(client_info);
        return FTB_ERR_SUBSCRIPTION_STYLE;
    }
    /* Expose the error handling as option to use */
    client_info->err_handling = FTB_ERR_HANDLE_NONE;

    if ((ret = FTBCI_split_namespace(cinfo->event_space, region_name, category_name,
                                     component_name)) != FTB_SUCCESS) {
        FTBU_WARNING("Invalid namespace format");
        free(client_info);
        FTBU_INFO("FTBC_Connect Out");
        return ret;
    }

    FTBCI_lock_client_lib();
    if (num_components == 0) {
	ret = 0;
	ret = FTBU_map_init(FTBCI_util_is_equal_clienthandle, &FTBCI_client_info_map);
	int ret1 = 0;
        if ((ret1 = FTBM_Init(1)) != FTB_SUCCESS) {
		FTBCI_unlock_client_lib();
        	free(client_info);
		return ret1;
	};
    }
    num_components++;
    FTBCI_unlock_client_lib();

    if ((client_info->id = (FTB_id_t *) malloc(sizeof(FTB_id_t))) == NULL) {
        free(client_info);
        FTBU_INFO("Malloc error in FTB library");
        return (FTB_ERR_CLASS_FATAL + FTB_ERR_MALLOC);
    }
    client_info->id->client_id.ext = extension;
    strcpy(client_info->id->client_id.region, region_name);
    strcpy(client_info->id->client_id.comp_cat, category_name);
    strcpy(client_info->id->client_id.comp, component_name);
    if (strlen(cinfo->client_name) >= FTB_MAX_CLIENT_NAME) {
        free(client_info->id);
        free(client_info);
        FTBU_INFO("FTBC_Connect Out");
        return FTB_ERR_INVALID_VALUE;
    }
    else {
        strcpy(client_info->id->client_id.client_name, cinfo->client_name);
    }

    FTBCI_convert_clientid_to_clienthandle(client_info->id->client_id, &client_info->client_handle);
    *client_handle = client_info->client_handle;

    FTBM_Get_location_id(&client_info->id->location_id);
    client_info->finalizing = 0;
    client_info->event_queue_size = 0;
    client_info->seqnum = 0;
    if (strlen(cinfo->client_jobid) >= FTB_MAX_CLIENT_JOBID) {
        free(client_info->id);
        free(client_info);
        FTBU_INFO("FTBC_Connect Out");
        return FTB_ERR_INVALID_VALUE;
    }
    else {
        strcpy(client_info->jobid, cinfo->client_jobid);
    }

    if (client_info->subscription_type & FTB_SUBSCRIPTION_POLLING) {
        if ((client_info->event_queue = (FTBU_list_node_t *) malloc(sizeof(FTBU_list_node_t))) == NULL) {
            FTBU_INFO("Malloc error in FTB library");
            return (FTB_ERR_CLASS_FATAL + FTB_ERR_MALLOC);
	}
        FTBU_list_init(client_info->event_queue);
	ret = 0;
	ret = FTBU_map_init(FTBCI_util_is_equal_event, &client_info->polling_map);
    }
    else {
        client_info->event_queue = NULL;
	client_info->polling_map = NULL;
    }

    pthread_mutex_init(&client_info->lock, NULL);
    ret = 0;
    ret = FTBU_map_init(FTBCI_util_is_equal_declared_event_index, &client_info->declared_events_map);
    if (client_info->subscription_type & FTB_SUBSCRIPTION_NOTIFY) {
        /* should this point to init and compare with equal_mask */
	int ret = 0;
	ret = FTBU_map_init(FTBCI_util_is_equal_event, &client_info->callback_map);
        if ((client_info->callback_event_queue = (FTBU_list_node_t *) malloc(sizeof(FTBU_list_node_t))) == NULL) {
            FTBU_INFO("Malloc error in FTB library");
            return (FTB_ERR_CLASS_FATAL + FTB_ERR_MALLOC);
	}
        FTBU_list_init(client_info->callback_event_queue);
        FTBCI_lock_client_lib();
        if (enable_callback == 0) {
            pthread_create(&callback_thread, NULL, FTBCI_callback_thread_client, NULL);
        }
        enable_callback++;
        FTBCI_unlock_client_lib();
        /*Create component level thread */
        {
            pthread_attr_t attr;
            pthread_attr_init(&attr);
            pthread_attr_setstacksize(&attr, 1024 * 1024);
            pthread_cond_init(&client_info->cond, NULL);
            pthread_create(&client_info->callback, &attr, FTBCI_callback_component,
                           (void *) client_info);
        }
    }
    else {
        client_info->callback_map = NULL;
    }

    FTBCI_lock_client_lib();
    FTBU_INFO
        ("Going to call FTBU_map_insert() to insert as key=client_info->client_handle, data=client_info map=FTBCI_client_info_map");

    if (FTBU_map_insert(FTBCI_client_info_map, FTBU_MAP_PTR_KEY(&client_info->client_handle),
                        (void *) client_info) == FTBU_EXIST) {
        FTBU_WARNING("This component has already been registered");
        FTBCI_unlock_client_lib();
        if (client_info->subscription_type & FTB_SUBSCRIPTION_POLLING)
            free(client_info->event_queue);
        if (client_info->subscription_type & FTB_SUBSCRIPTION_NOTIFY)
            free(client_info->callback_event_queue);
        free(client_info->id);
        free(client_info);
        FTBU_INFO("FTBC_Connect Out");
        return FTB_ERR_DUP_CALL;
    }
    FTBCI_unlock_client_lib();

    memcpy(&msg.src, client_info->id, sizeof(FTB_id_t));
    msg.msg_type = FTBM_MSG_TYPE_CLIENT_REG;
    FTBM_Get_parent_location_id(&msg.dst.location_id);
    FTBU_INFO("Parent: %s pid %d pid_starttime=%s", msg.dst.location_id.hostname,
              msg.dst.location_id.pid, msg.dst.location_id.pid_starttime);
    ret = FTBM_Send(&msg);
    if (ret != FTB_SUCCESS) {
        if (client_info->subscription_type & FTB_SUBSCRIPTION_POLLING)
            free(client_info->event_queue);
        if (client_info->subscription_type & FTB_SUBSCRIPTION_NOTIFY)
            free(client_info->callback_event_queue);
        free(client_info->id);
        free(client_info);

	/* Hoony: should clean up map_node of this client_info */
        FTBU_INFO("FTBC_Connect Out");
        return ret;
    }
    /* Check progress for other clients atleast */
    FTBCI_Check_progress();
    FTBU_INFO("FTBC_Connect Out");
    return FTB_SUCCESS;
}


int FTBCI_check_schema_file(const FTB_client_handle_t client_handle, const char *schema_file)
{
    typedef enum state { INIT, IN_STRUCT, FOUND_EVENTSPACE, FOUND_EVENTNAME, EMPTY, } state_t;
    FTB_eventspace_t comp_cat;
    FTB_eventspace_t comp;
    FTB_eventspace_t region;
    FTB_event_info_t einfo;
    char str[1024];
    FILE *fp;
    int ret, count;
    FTBCI_client_info_t *client_info;

    /* Set the schema script */
    char schema_file_script[512] = "cat ";
    strcat(schema_file_script, schema_file);
    strcat(schema_file_script, " | sed -e 's/#.*//g'");
    state_t state = EMPTY;

    fp = popen(schema_file_script, "r");
    while (!feof(fp)) {
        count = fscanf(fp, "%s", str);
        if (feof(fp))
            break;
        if ((state == INIT) || (state == EMPTY)) {
            /* Initial state: Ignore non-start strings */
            if (!strcmp(str, "start")) {
                state = IN_STRUCT;
            }
            else {
                FTBU_WARNING("Unexpected string (%s) found in schema file (%s)\n", str, schema_file);
            }
        }
        else {
            /* Started reading. Store strings till you hit end */
            if (!strcmp(str, "end")) {
                /* End of struct */
                state = INIT;
            }
            else {
                /* Within struct. Store eventspace string */
                if (state == IN_STRUCT) {
                    if (strlen(str) >= FTB_MAX_EVENTSPACE) {
                        FTBU_WARNING("Event space exceeds expected length\n");
                        return FTB_ERR_INVALID_SCHEMA_FILE;
                    }
                    ret = FTBCI_split_namespace(str, region, comp_cat, comp);
                    if (ret != FTB_SUCCESS) {
                        FTBU_WARNING("Event space if of incorrect format in file (%s)", schema_file);
                        return FTB_ERR_INVALID_SCHEMA_FILE;
                    }
                    /* Check if the schema file was submitted by the right
                     * client
                     */
                    if ((strcasecmp(client_handle.client_id.region, region) != 0)
                        || (strcasecmp(client_handle.client_id.comp_cat, comp_cat) != 0)
                        || (strcasecmp(client_handle.client_id.comp, comp) != 0))
                        return FTB_ERR_INVALID_SCHEMA_FILE;
                    state = FOUND_EVENTSPACE;
                }
                else if (state == FOUND_EVENTSPACE) {
                    /*Store the event name */
                    state = FOUND_EVENTNAME;
                    strcpy(einfo.event_name, str);
                }
                else if (state == FOUND_EVENTNAME) {
                    /*Store the event severity */
                    if (!FTBCI_check_severity_values(str)) {
                        FTBU_WARNING("Unrecognized severity (%s) in file(%s)", str, schema_file);
                        return FTB_ERR_INVALID_FIELD;
                    }
                    state = FOUND_EVENTSPACE;
                    strcpy(einfo.severity, str);
                    FTBCI_LOOKUP_CLIENT_INFO(client_handle, client_info);
                    if ((ret = FTBCI_store_declared_events(client_info, &einfo, 1)) != FTB_SUCCESS)
                        return ret;
                }
            }
        }
    }
    pclose(fp);
    if (state == EMPTY) {
        FTBU_WARNING("Schema file (%s) is either not present, or empty or of invalid format",
                     schema_file);
        return FTB_ERR_INVALID_SCHEMA_FILE;
    }
    else if (state != INIT) {
        FTBU_WARNING("Schema file (%s) is of invalid format ('end' missing)", schema_file);
        return FTB_ERR_INVALID_SCHEMA_FILE;
    }
    else
        return FTB_SUCCESS;
}


int FTBC_Declare_publishable_events(FTB_client_handle_t client_handle, const char *schema_file,
                                    const FTB_event_info_t * einfo, int num_events)
{
    int ret = 0;

    FTBU_INFO("FTBC_Declare_publishable_events In");
    if (client_handle.valid != 1) {
        FTBU_INFO("FTBC_Declare_publishable_events Out");
        return FTB_ERR_INVALID_HANDLE;
    }
    if (schema_file != NULL) {
        ret = FTBCI_check_schema_file(client_handle, schema_file);
        if (ret != FTB_SUCCESS) {
            FTBU_INFO("FTBC_Declare_publishable_events Out");
            return ret;
        }
    }
    else {
        FTBCI_client_info_t *client_info;
        FTBCI_LOOKUP_CLIENT_INFO(client_handle, client_info);
        ret = FTBCI_store_declared_events(client_info, einfo, num_events);
        if (ret != FTB_SUCCESS) {
            FTBU_INFO("FTBC_Declare_publishable_events routine was not-successful");
            return ret;
        }
        FTBU_INFO("FTBC_Declare_publishable_events Out");
        return FTB_SUCCESS;
    }
    /* Check progress for other clients atleast */
    FTBCI_Check_progress();
    return FTB_SUCCESS;
}


int FTBC_Subscribe_with_polling(FTB_subscribe_handle_t * subscribe_handle,
                                FTB_client_handle_t client_handle, const char *subscription_str)
{
    FTBM_msg_t msg;
    FTBCI_client_info_t *client_info;
    int ret;
    FTB_event_t *subscription_event;
    if ((subscription_event = (FTB_event_t *) malloc(sizeof(FTB_event_t))) == NULL) {
        FTBU_INFO("Malloc error in FTB library");
        return (FTB_ERR_CLASS_FATAL + FTB_ERR_MALLOC);
    }

    FTBU_INFO("FTBC_Subscribe_with_polling In");
    if (subscribe_handle == NULL) {
        free(subscription_event);
        FTBU_INFO("FTBC_Subscribe_with_polling Out");
        return FTB_ERR_NULL_POINTER;
    }

    if (client_handle.valid != 1) {
        free(subscription_event);
        FTBU_INFO("FTBC_Subscribe_with_polling Out");
        return FTB_ERR_INVALID_HANDLE;
    }

    FTBCI_LOOKUP_CLIENT_INFO(client_handle, client_info);
    if (!(client_info->subscription_type & FTB_SUBSCRIPTION_POLLING)) {
        free(subscription_event);
        FTBU_INFO("FTBC_Subscribe_with_polling Out");
        return FTB_ERR_NOT_SUPPORTED;
    }

    if ((ret = FTBCI_parse_subscription_string(subscription_str, subscription_event)) != FTB_SUCCESS) {
        free(subscription_event);
        FTBU_INFO("FTBC_Subscribe_with_polling Out");
        return ret;
    }

    /* Check progress for other clients atleast */
    FTBCI_Check_progress();

    memcpy(&subscribe_handle->client_handle, &client_handle, sizeof(FTB_client_handle_t));
    memcpy(&subscribe_handle->subscription_event, subscription_event, sizeof(FTB_event_t));
    subscribe_handle->subscription_type = FTB_SUBSCRIPTION_POLLING;
    subscribe_handle->valid = 1;

    /* Add subscription string into the polling_map. */
    if ((ret = FTBCI_util_add_to_polling_map(client_info, subscription_event)) != FTB_SUCCESS) {
	free(subscription_event);
        FTBU_INFO("FTBC_Subscribe_with_polling Out");
	return ret;
    }

    memcpy(&msg.event, subscription_event, sizeof(FTB_event_t));
    memcpy(&msg.src, client_info->id, sizeof(FTB_id_t));
    msg.msg_type = FTBM_MSG_TYPE_REG_SUBSCRIPTION;
    FTBM_Get_parent_location_id(&msg.dst.location_id);
    ret = FTBM_Send(&msg);
    free(subscription_event);

    FTBU_INFO("FTBC_Subscribe_with_polling Out");
    return ret;
}


int FTBC_Subscribe_with_callback(FTB_subscribe_handle_t * subscribe_handle,
                                 FTB_client_handle_t client_handle, const char *subscription_str,
                                 int (*callback) (FTB_receive_event_t *, void *), void *arg)
{
    FTBM_msg_t msg;
    FTBCI_client_info_t *client_info;
    int ret;
    FTB_event_t *subscription_event;
    if ((subscription_event = (FTB_event_t *) malloc(sizeof(FTB_event_t))) == NULL) {
        FTBU_INFO("Malloc error in FTB library");
        return (FTB_ERR_CLASS_FATAL + FTB_ERR_MALLOC);
    }

    FTBU_INFO("FTBC_Subscribe_with_callback In");
    if (subscribe_handle == NULL) {
        free(subscription_event);
        FTBU_INFO("FTBC_Subscribe_with_callback Out");
        return FTB_ERR_NULL_POINTER;
    }

    if (client_handle.valid != 1) {
        free(subscription_event);
        FTBU_INFO("FTBC_Subscribe_with_callback Out");
        return FTB_ERR_INVALID_HANDLE;
    }

    FTBCI_LOOKUP_CLIENT_INFO(client_handle, client_info);

    if (!(client_info->subscription_type & FTB_SUBSCRIPTION_NOTIFY)) {
        free(subscription_event);
        FTBU_INFO("FTBC_Subscribe_with_callback Out");
        return FTB_ERR_NOT_SUPPORTED;
    }
    if ((ret = FTBCI_parse_subscription_string(subscription_str, subscription_event)) != FTB_SUCCESS) {
        free(subscription_event);
        FTBU_INFO("FTBC_Subscribe_with_callback Out");
        return ret;
    }

    /* Check progress for other clients atleast */
    FTBCI_Check_progress();

    /*
     * Set the subscribe_handle to correct parameters in case the arg for
     * callback is using the subscribe handle itself
     */
    memcpy(&subscribe_handle->client_handle, &client_handle, sizeof(FTB_client_handle_t));
    memcpy(&subscribe_handle->subscription_event, subscription_event, sizeof(FTB_event_t));
    subscribe_handle->subscription_type = FTB_SUBSCRIPTION_NOTIFY;
    subscribe_handle->valid = 1;

    if ((ret = FTBCI_util_add_to_callback_map(client_info, subscription_event, callback, arg)) != FTB_SUCCESS) {
    	free(subscription_event);
    	FTBU_INFO("FTBC_Subscribe_with_callback Out");
	return ret;
    }

    memcpy(&msg.event, subscription_event, sizeof(FTB_event_t));
    memcpy(&msg.src, client_info->id, sizeof(FTB_id_t));
    msg.msg_type = FTBM_MSG_TYPE_REG_SUBSCRIPTION;
    FTBM_Get_parent_location_id(&msg.dst.location_id);
    ret = FTBM_Send(&msg);
    free(subscription_event);
    FTBU_INFO("FTBC_Subscribe_with_callback Out");
    return ret;
}


int FTBC_Get_event_handle(const FTB_receive_event_t receive_event, FTB_event_handle_t * event_handle)
{
    int ret;

    FTBU_INFO("FTBC_Get_event_handle In");
    if ((ret =
         FTBCI_split_namespace(receive_event.event_space, event_handle->client_id.region,
                               event_handle->client_id.comp_cat,
                               event_handle->client_id.comp)) != FTB_SUCCESS) {
        return ret;
    }
    strcpy(event_handle->client_id.client_name, receive_event.client_name);
    event_handle->client_id.ext = receive_event.client_extension;
    strcpy(event_handle->location_id.hostname, receive_event.incoming_src.hostname);
    memcpy(&event_handle->location_id.pid, &receive_event.incoming_src.pid, sizeof(pid_t));
    strcpy(event_handle->location_id.pid_starttime, receive_event.incoming_src.pid_starttime);
    strcpy(event_handle->event_name, receive_event.event_name);
    strcpy(event_handle->severity, receive_event.severity);
    event_handle->seqnum = receive_event.seqnum;
    /* Check progress for other clients atleast */
    FTBCI_Check_progress();
    FTBU_INFO("FTBC_Get_event_handle Out");
    return FTB_SUCCESS;
}


int FTBC_Compare_event_handles(const FTB_event_handle_t event_handle1,
                               const FTB_event_handle_t event_handle2)
{
    /* Check progress for other clients atleast */
    FTBCI_Check_progress();
    if ((strcasecmp(event_handle1.client_id.region, event_handle2.client_id.region) == 0)
        && (strcasecmp(event_handle1.client_id.comp_cat, event_handle2.client_id.comp_cat) == 0)
        && (strcasecmp(event_handle1.client_id.comp, event_handle2.client_id.comp) == 0)
        && (strcasecmp(event_handle1.client_id.client_name, event_handle2.client_id.client_name) == 0)
        && (event_handle1.client_id.ext == event_handle2.client_id.ext)
        && (strcasecmp(event_handle1.location_id.hostname, event_handle2.location_id.hostname) == 0)
        && (event_handle1.location_id.pid == event_handle2.location_id.pid)
        && (strcasecmp(event_handle1.location_id.pid_starttime, event_handle2.location_id.pid_starttime)
            == 0)
        && (strcasecmp(event_handle1.event_name, event_handle2.event_name) == 0)
        && (strcasecmp(event_handle1.severity, event_handle2.severity) == 0)
        && (event_handle1.seqnum == event_handle2.seqnum)) {
        FTBU_INFO("FTBC_Compare_event_handles In");
        return FTB_SUCCESS;
    }
    else {
        FTBU_INFO("FTBC_Compare_event_handles Out");
        return FTB_FAILURE;
    }
}


int FTBC_Publish(FTB_client_handle_t client_handle, const char *event_name,
                 const FTB_event_properties_t * event_properties, FTB_event_handle_t * event_handle)
{
    FTBM_msg_t msg;
    FTBCI_client_info_t *client_info;
    FTB_event_properties_t *temp_event_properties = NULL;
    FTBU_map_node_t *iter;
    FTBCI_publish_event_index_t *event_index;
    FTBCI_publish_event_details_t *event_details;
    int ret;

    FTBU_INFO("FTBC_Publish In");

    if (client_handle.valid != 1) {
        FTBU_INFO("FTBC_Publish Out");
        return FTB_ERR_INVALID_HANDLE;
    }
    if (event_handle == NULL) {
        FTBU_INFO("FTBC_Publish Out");
        return FTB_ERR_NULL_POINTER;
    }
    if ((strlen(event_name) == 0) || (strlen(event_name) >= FTB_MAX_EVENT_NAME)) {
        FTBU_INFO("FTBC_Publish Out");
        return FTB_ERR_INVALID_EVENT_NAME;
    }

    FTBCI_Check_progress();
    FTBCI_LOOKUP_CLIENT_INFO(client_handle, client_info);
    if ((event_index = (FTBCI_publish_event_index_t *) malloc(sizeof(FTBCI_publish_event_index_t))) == NULL) {
        FTBU_INFO("Malloc error in FTB library");
        return (FTB_ERR_CLASS_FATAL + FTB_ERR_MALLOC);
    }
    strcpy(event_index->event_name, event_name);

    FTBCI_lock_client(client_info);
    iter = FTBU_map_find_key(client_info->declared_events_map, FTBU_MAP_PTR_KEY(event_index));
    if (iter == FTBU_map_end(client_info->declared_events_map)) {
        free(event_index);
        FTBCI_unlock_client(client_info);
        FTBU_INFO("FTBC_Publish Out - event name has not been declared");
        return FTB_ERR_INVALID_EVENT_NAME;
    }
    event_details = (FTBCI_publish_event_details_t *) FTBU_map_get_data(iter);
    FTBCI_unlock_client(client_info);
    strcpy(msg.event.event_name, event_index->event_name);
    strcpy(msg.event.severity, event_details->severity);
    free(event_index);
    strcpy(msg.event.region, client_handle.client_id.region);
    strcpy(msg.event.comp_cat, client_handle.client_id.comp_cat);
    strcpy(msg.event.comp, client_handle.client_id.comp);
    memcpy(&msg.src, client_info->id, sizeof(FTB_id_t));

    if (event_properties == NULL) {
        if ((temp_event_properties = (FTB_event_properties_t *) malloc(sizeof(FTB_event_properties_t))) == NULL) {
            FTBU_INFO("Malloc error in FTB library");
            return (FTB_ERR_CLASS_FATAL + FTB_ERR_MALLOC);
	}
        temp_event_properties->event_type = 1;
        event_properties = temp_event_properties;
    }
    else if ((event_properties->event_type != 1) && (event_properties->event_type != 2)) {
        FTBU_INFO("FTBC_Publish Out");
        return FTB_ERR_INVALID_EVENT_TYPE;
    }

    FTBCI_lock_client(client_info);
    client_info->seqnum += 1;
    FTBCI_unlock_client(client_info);

    msg.msg_type = FTBM_MSG_TYPE_NOTIFY;
    strcpy(msg.event.hostname, msg.src.location_id.hostname);
    strcpy(msg.event.client_name, client_info->id->client_id.client_name);
    strcpy(msg.event.client_jobid, client_info->jobid);
    msg.event.seqnum = client_info->seqnum;
    memcpy(&msg.event.event_type, &event_properties->event_type, sizeof(uint8_t));
    memcpy(msg.event.event_payload, event_properties->event_payload, FTB_MAX_PAYLOAD_DATA);
    FTBM_Get_parent_location_id(&msg.dst.location_id);
    ret = FTBM_Send(&msg);

    strcpy(event_handle->client_id.region, msg.event.region);
    strcpy(event_handle->client_id.comp_cat, msg.event.comp_cat);
    strcpy(event_handle->client_id.comp, msg.event.comp);
    strcpy(event_handle->client_id.client_name, msg.event.client_name);
    event_handle->client_id.ext = msg.src.client_id.ext;
    strcpy(event_handle->location_id.hostname, msg.src.location_id.hostname);
    memcpy(&event_handle->location_id.pid, &msg.src.location_id.pid, sizeof(pid_t));
    strcpy(event_handle->location_id.pid_starttime, msg.src.location_id.pid_starttime);
    strcpy(event_handle->event_name, msg.event.event_name);
    strcpy(event_handle->severity, msg.event.severity);
    event_handle->seqnum = msg.event.seqnum;

    FTBU_INFO("FTBC_Publish Out");
    return ret;
}

int FTBC_Poll_event(FTB_subscribe_handle_t subscribe_handle, FTB_receive_event_t * receive_event)
{
    FTBCI_client_info_t *client_info;
    FTB_client_handle_t client_handle;
    FTBCI_event_inst_list_t *entry;
    int z = 0, ret = 0;

    FTBU_INFO("FTBC_Poll_event In");

    if (receive_event == NULL) {
        FTBU_INFO("FTBC_Poll_event Out");
        return FTB_ERR_NULL_POINTER;
    }

    if (subscribe_handle.valid == 0) {
        FTBU_INFO("FTBC_Poll_event Out");
        return FTB_ERR_INVALID_HANDLE;
    }

    memcpy(&client_handle, &subscribe_handle.client_handle, sizeof(FTB_client_handle_t));
    FTBCI_LOOKUP_CLIENT_INFO(client_handle, client_info);

    if (!(client_info->subscription_type & FTB_SUBSCRIPTION_POLLING)) {
        FTBU_INFO("FTBC_Poll_event Out");
        return FTB_ERR_NOT_SUPPORTED;
    }

    for (z = 0; z < 2; z++) {
        ret = FTBCI_Check_progress();
        FTBCI_lock_client(client_info);
        if (client_info->event_queue_size > 0) {

            int event_found = 0;
            /* FTBU_list_node_t *start = client_info->event_queue->next */
            FTBU_list_node_t *start = client_info->event_queue;
            FTBU_list_node_t *current = client_info->event_queue->next;

            entry = (FTBCI_event_inst_list_t *) current->data;
            do {
                if (FTBU_match_mask(&entry->event_inst, &subscribe_handle.subscription_event)) {
                    event_found = 1;
                    concatenate_strings(receive_event->event_space, entry->event_inst.region, ".",
                                        entry->event_inst.comp_cat, ".", entry->event_inst.comp, NULL);
                    strcpy(receive_event->event_name, entry->event_inst.event_name);
                    strcpy(receive_event->severity, entry->event_inst.severity);
                    strcpy(receive_event->client_jobid, entry->event_inst.client_jobid);
                    strcpy(receive_event->client_name, entry->event_inst.client_name);
                    memcpy(&receive_event->client_extension, &entry->src.client_id.ext, sizeof(int));
                    memcpy(&receive_event->seqnum, &entry->event_inst.seqnum, sizeof(int));
                    memcpy(&receive_event->event_type, &entry->event_inst.event_type, sizeof(uint8_t));
                    memcpy(receive_event->event_payload, entry->event_inst.event_payload,
                           FTB_MAX_PAYLOAD_DATA);
                    memcpy(&receive_event->incoming_src, &entry->src.location_id,
                           sizeof(FTB_location_id_t));
                    break;
                }
                else {
                    current = current->next;
                    entry = (FTBCI_event_inst_list_t *) current->data;
                }
            } while (current != start);

            if (event_found) {
                FTBU_list_remove_node(current);
                client_info->event_queue_size--;
                FTBCI_unlock_client(client_info);
                free(current->data);
                free(current);
                FTBU_INFO("FTBC_Poll_event Out");
                return FTB_SUCCESS;
            }
        }
        FTBCI_unlock_client(client_info);
    }
    FTBCI_unlock_client(client_info);
    FTBU_INFO("FTBC_Poll_event Out");
    return FTB_GOT_NO_EVENT;
}


int FTBC_Unsubscribe(FTB_subscribe_handle_t * subscribe_handle)
{
    FTBCI_client_info_t *client_info;
    FTBM_msg_t msg;
    int ret;

    FTBU_INFO("FTBC_Unsubscribe In");

    FTBCI_LOOKUP_CLIENT_INFO(subscribe_handle->client_handle, client_info);

    if ((subscribe_handle == NULL) || (subscribe_handle->valid == 0)) {
        FTBU_INFO("FTBC_Unsubscribe Out");
        return FTB_ERR_INVALID_HANDLE;
    }

    memcpy(&msg.event, &subscribe_handle->subscription_event, sizeof(FTB_event_t));
    memcpy(&msg.src, client_info->id, sizeof(FTB_id_t));
    msg.msg_type = FTBM_MSG_TYPE_SUBSCRIPTION_CANCEL;
    FTBM_Get_parent_location_id(&msg.dst.location_id);
    subscribe_handle->valid = 0;
    ret = FTBM_Send(&msg);
    if (ret != FTB_SUCCESS) {
        FTBU_INFO("FTBC_Unsubscribe Out");
        return ret;
    }

    /* This is a potential bug. 
     * A client may subscribe to both, and unsubscribe one with polling.
     * Hoony:
     */
    //    if (subscribe_handle->subscription_type & FTB_SUBSCRIPTION_NOTIFY) {
        /* Subscription was registered for callback mechanism */
    //        FTBCI_util_remove_from_callback_map(client_info, &subscribe_handle->subscription_event);
    //    }

    FTBCI_util_remove_from_subscription_map(client_info, &subscribe_handle->subscription_event);

    /* Nothing needs to be done if subscription was registered
     * using polling mechanism. Only the subscribe_handle->valid
     * needs to be set to 0
     */
    /* Check progress for other clients atleast */
    FTBCI_Check_progress();
    FTBU_INFO("FTBC_Unsubscribe Out");
    return ret;
}


int FTBC_Disconnect(FTB_client_handle_t client_handle)
{
    FTBCI_client_info_t *client_info;
    FTBM_msg_t msg;
    int ret;

    FTBU_INFO("FTBC_Disconnect In");

    if (client_handle.valid != 1) {
        FTBU_INFO("FTBC_Disconnect Out");
        return FTB_ERR_INVALID_HANDLE;
    }

    FTBCI_LOOKUP_CLIENT_INFO(client_handle, client_info);

    memcpy(&msg.src, client_info->id, sizeof(FTB_id_t));
    msg.msg_type = FTBM_MSG_TYPE_CLIENT_DEREG;
    FTBM_Get_parent_location_id(&msg.dst.location_id);
    ret = FTBM_Send(&msg);
    if (ret != FTB_SUCCESS) {
        FTBU_INFO("FTBC_Disconnect Out");
        return ret;
    }

    FTBCI_lock_client(client_info);
    FTBCI_util_cleanup_declared_events_map(client_info);
    FTBCI_util_finalize_component(client_info);
    client_info->finalizing = 1;
    FTBCI_unlock_client(client_info);

    FTBCI_lock_client_lib();
    pthread_mutex_destroy(&client_info->lock);
    FTBU_map_remove_key(FTBCI_client_info_map, FTBU_MAP_PTR_KEY(&client_handle));
    if (client_info->subscription_type & FTB_SUBSCRIPTION_NOTIFY) {
        enable_callback--;
        if (enable_callback == 0) {
            /*Last callback component finalized */
            pthread_cancel(callback_thread);
            pthread_join(callback_thread, NULL);
        }
    }
    free(client_info);
    num_components--;
    if (num_components == 0) {
        FTBM_Finalize();
        FTBU_map_finalize(FTBCI_client_info_map);
        FTBCI_client_info_map = NULL;
    }
    FTBCI_unlock_client_lib();
    /* Check progress for other clients atleast */
    FTBCI_Check_progress();

    FTBU_INFO("FTBC_Disconnect Out");
    return FTB_SUCCESS;
}
