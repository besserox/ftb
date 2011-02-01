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

/*
 * This file provides a set of utility routines that are used
 * throughout the FTB code. All routines in this file start
 * with FTBU_ to indicate that they are utility routines.
 */

#include "ftb_util.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

extern FILE *FTBU_log_file_fp;

/*
 * The FTBU_match_mask matches an event against a mask.
 * A mask may contain specific values or wildcards.
 */
int FTBU_match_mask(const FTB_event_t * event, const FTB_event_t * mask)
{
    FTBU_INFO("In FTBU_match_mask");

    FTBU_INFO("FTBU_match_mask: event->client_name=%s and mask->client_name=%s", event->client_name,
              mask->client_name);
    FTBU_INFO("FTBU_match_mask: event->region=%s and mask->region=%s", event->region, mask->region);
    FTBU_INFO("FTBU_match_mask: event->hostname=%s and mask->hostname=%s", event->hostname,
              mask->hostname);
    FTBU_INFO("FTBU_match_mask: event->event_name=%s and mask->event_name=%s", event->event_name,
              mask->event_name);


    if (((strcasecmp(event->region, mask->region) == 0)
         || (strcasecmp(mask->region, "ALL") == 0))
        && ((strcasecmp(event->client_jobid, mask->client_jobid) == 0)
            || (strcasecmp(mask->client_jobid, "ALL") == 0))
        && ((strcasecmp(event->client_name, mask->client_name) == 0)
            || (strcasecmp(mask->client_name, "ALL") == 0))
        && ((strcasecmp(event->hostname, mask->hostname) == 0)
            || (strcasecmp(mask->hostname, "ALL") == 0))) {

        /*
         * If the mask does not have a wildcard for event_name, then check if the
         * event names match and return a success on a match and failure otherwise.
         * If mask has a  wild card for event_name, then check other attributes
         */

        if (strcasecmp(mask->event_name, "ALL") != 0) {
            if (strcasecmp(event->event_name, mask->event_name) == 0) {
                FTBU_INFO("Out FTBU_match_mask");
                return 1;
            }
        }
        else {
            if (((strcasecmp(event->severity, mask->severity) == 0)
                 || (strcasecmp(mask->severity, "ALL") == 0))
                && ((strcasecmp(event->comp_cat, mask->comp_cat) == 0)
                    || (strcasecmp(mask->comp_cat, "ALL") == 0))
                && ((strcasecmp(event->comp, mask->comp) == 0)
                    || (strcasecmp(mask->comp, "ALL") == 0))) {

                FTBU_INFO("Out FTBU_match_mask");
                return 1;

            }
        }
    }
    FTBU_INFO("Out FTBU_match_mask");
    return 0;
}


/*
 * FTBU_is_equal_location_id compares two location ids. Currently location ids are
 * distinguised based on pid, host ip address and pid starttime
 */
int FTBU_is_equal_location_id(const FTB_location_id_t * lhs, const FTB_location_id_t * rhs)
{
    if (lhs->pid == rhs->pid) {
        if ((strncasecmp(lhs->hostname, rhs->hostname, FTB_MAX_HOST_ADDR) == 0)
            && (strncasecmp(lhs->pid_starttime, rhs->pid_starttime, FTB_MAX_PID_TIME) == 0))
            return 1;
    }
    return 0;
}


/*
 * The ftb_id identifies a client, especially when they are a part of the same process
 * FTBU_is_equal_ftb_id compares the clients based on component category, component name,
 * user-defined client-name and extension parameter (useful for IBM BG)
 */
int FTBU_is_equal_ftb_id(const FTB_id_t * lhs, const FTB_id_t * rhs)
{
    if ((strcasecmp(lhs->client_id.comp_cat, rhs->client_id.comp_cat) == 0)
        && (strcasecmp(lhs->client_id.comp, rhs->client_id.comp) == 0)
        && (strcasecmp(lhs->client_id.client_name, rhs->client_id.client_name) == 0)
        && (lhs->client_id.ext == rhs->client_id.ext)) {
        return FTBU_is_equal_location_id(&lhs->location_id, &rhs->location_id);
    }
    else
        return 0;
}


/*
 * FTBU_is_equal_event checks if two events match
 */
int FTBU_is_equal_event(const FTB_event_t * lhs, const FTB_event_t * rhs)
{
    if ((strcasecmp(lhs->severity, rhs->severity) == 0)
        && (strcasecmp(lhs->comp_cat, rhs->comp_cat) == 0)
        && (strcasecmp(lhs->comp, rhs->comp) == 0)
        && (strcasecmp(lhs->event_name, rhs->event_name) == 0)) {
        return 1;
    }
    else
        return 0;
}


/*
 * FTBU_is_equal_event checks if two event masks match
 */
int FTBU_is_equal_event_mask(const FTB_event_t * lhs, const FTB_event_t * rhs)
{

    if ((strcasecmp(lhs->severity, rhs->severity) == 0)
        && (strcasecmp(lhs->comp_cat, rhs->comp_cat) == 0)
        && (strcasecmp(lhs->comp, rhs->comp) == 0)
        && (strcasecmp(lhs->client_name, rhs->client_name) == 0)
        && (strcasecmp(lhs->client_jobid, rhs->client_jobid) == 0)
        && (strcasecmp(lhs->hostname, rhs->hostname) == 0)
        && (strcasecmp(lhs->region, rhs->region) == 0)
        && (strcasecmp(lhs->event_name, rhs->event_name) == 0)) {
        return 1;
    }
    else
        return 0;
}


/*
 * FTBU_is_equal_clienthandle checks if two client handles match
 */
int FTBU_is_equal_clienthandle(const FTB_client_handle_t * lhs, const FTB_client_handle_t * rhs)
{
    if ((strcasecmp(lhs->client_id.comp_cat, rhs->client_id.comp_cat) == 0)
        && (strcasecmp(lhs->client_id.comp, rhs->client_id.comp) == 0)
        && (strcasecmp(lhs->client_id.client_name, rhs->client_id.client_name) == 0)
        && (lhs->client_id.ext == rhs->client_id.ext)) {
        return 1;
    }
    else {
        return 0;
    }
}


/*
 * FTBU_get_output_of_cmd function returns the output of a command.
 * FIXME: This function needs to be reimplemented cleanly
 * Many calling functions will ignore the return value. Implement this with void return value
 */
#define FTB_BOOTSTRAP_UTIL_MAX_STR_LEN  128

int FTBU_get_output_of_cmd(const char *cmd, char *output, size_t len)
{
    if (strcasecmp(cmd, "hostname") == 0) {
        char *name;
	if ((name= (char *) malloc(len)) == NULL) {
            FTBU_INFO("Malloc error in FTB library");
            return (FTB_ERR_CLASS_FATAL + FTB_ERR_MALLOC);
	}
        if (gethostname(name, len) == 0) {
            strncpy(output, name, len);
        }
        else {
            FTBU_INFO("gethostname command failed");
        }
    }
    else if (strcasecmp(cmd, "date +%m-%d-%H-%M-%S") == 0) {
        char *myDate;
	if ((myDate = (char *) malloc(len)) == NULL) {
            FTBU_INFO("Malloc error in FTB library");
            return (FTB_ERR_CLASS_FATAL + FTB_ERR_MALLOC);
	}
        time_t myTime = time(NULL);
        if (strftime(myDate, len, "%m-%d-%H-%M-%S", gmtime(&myTime)) != 0) {
            strncpy(output, myDate, len);
        }
        else {
            FTBU_WARNING("strftime command failed");
        }
    }
    else if (strcasecmp(cmd, "grep ^BG_IP /proc/personality.sh | cut -f2 -d=") == 0) {
        FILE *fp;
        char *pos, *track;
        char str[32];
        int found = 0;

        fp = fopen("/proc/personality.sh", "r");

        if (!fp) {
            FTBU_WARNING("Could not find /proc/personality.sh");
/* FIXME: We need to return a specific error here */
            return FTB_ERR_GENERAL;
        }

        while (!feof(fp)) {
            track = fgets(str, 32, fp);

            if (((pos = strstr(str, "BG_IP=")) != NULL) || ((pos = strstr(str, "BGL_IP=")) != NULL)) {
                while (*pos++ != '=');
                strcpy(output, pos);
                found = 1;
                break;
            }
        }

        if (!found)
            FTBU_WARNING("Could not find BG_IP parameter in file /proc/personality.sh on the BG machine");

        fclose(fp);
    }
    else {
        char filename[FTB_BOOTSTRAP_UTIL_MAX_STR_LEN];
        char temp[FTB_BOOTSTRAP_UTIL_MAX_STR_LEN];
        FILE *fp;
        int ret;

        sprintf(filename, "/tmp/temp_file.%d", getpid());
        sprintf(temp, "%s > %s", cmd, filename);
        if (system(temp)) {
            FTBU_INFO("Execute command failed\n");
        }
        fp = fopen(filename, "r");
        ret = fscanf(fp, "%s", temp);
        fclose(fp);
        unlink(filename);
        strncpy(output, temp, len);
    }
    return FTB_SUCCESS;
}


/*
 * Checks if the keys for the internal maps (organized as doubly linked lists) match
 */
static int util_key_match(const FTBU_map_node_t * head, FTBU_map_key_t key1, FTBU_map_key_t key2)
{
    int (*is_equal_func_ptr) (const void *, const void *) =
        (int (*)(const void *, const void *)) head->data;
    if (is_equal_func_ptr == NULL)
        return FTBU_NULL_PTR;
    return (*is_equal_func_ptr) (key1.key_ptr, key2.key_ptr);
}


/*
 * Intialize a map (doubly linked list)
 * Note that for 'map' linked list, we need to insert the 'unique
 * function pointer' (supplied as an argument to below function call) in the first
 * node of the linked list
 */
FTBU_map_node_t *FTBU_map_init(int (*is_equal_func_ptr) (const void *, const void *))
{
    FTBU_map_node_t *node;
    if ((node = (FTBU_map_node_t *) malloc(sizeof(FTBU_map_node_t))) == NULL) {
            FTBU_INFO("Malloc error in FTB library");
//            return (FTB_ERR_CLASS_FATAL + FTB_ERR_MALLOC);
     }
    /* Point data of first node of the map to the function passed as argument */
    node->data = (void *) is_equal_func_ptr;
    node->next = node->prev = node;
    return node;
}


/*
 * The FTBU_map_insert() checks if the 'key' is already present in the
 * map. If not, a new node with the new key and data are inserted in the map.
 */
int FTBU_map_insert(FTBU_map_node_t * head, FTBU_map_key_t key, void *data)
{
    FTBU_map_node_t *new_node;
    FTBU_map_node_t *pos;
    FTBU_list_for_each_readonly(pos, head) {
        if (util_key_match(head, pos->key, key))
            return FTBU_EXIST;
    }
    if ((new_node = (FTBU_map_node_t *) malloc(sizeof(FTBU_map_node_t))) == NULL) {
        FTBU_INFO("Malloc error in FTB library");
        return (FTB_ERR_CLASS_FATAL + FTB_ERR_MALLOC);
    }
    new_node->key = key;
    new_node->data = data;
    head->prev->next = new_node;
    new_node->next = head;
    new_node->prev = head->prev;
    head->prev = new_node;
    return FTB_SUCCESS;
}


/*
 * The FTBU_map_begin() routine returns the first node of the map linked
 * list; Note that the head/real-first node of the map actually does not
 * contain any data; but rather contains a comparison function. Hence,
 * we return head->next in the below routine
 */
inline FTBU_map_node_t *FTBU_map_begin(const FTBU_map_node_t * head)
{
    return head->next;
}


/*
 * The FTBU_map_find_key() routine checks if a 'key' is present in the
 * map. The comparison function for checking for duplicate key is to be
 * found from the data element of the head node of the map.
 */
FTBU_map_node_t *FTBU_map_find_key(const FTBU_map_node_t * head, FTBU_map_key_t key)
{
    FTBU_map_node_t *pos;
    FTBU_list_for_each_readonly(pos, head) {
        if (util_key_match(head, pos->key, key))
            return pos;
    }
    return (FTBU_map_node_t *) head;
}


/*
 * The routine FTBU_map_get_key() extracts the key from the node
 */
inline FTBU_map_key_t FTBU_map_get_key(FTBU_map_node_t * node)
{
    return node->key;
}


/*
 * The routine FTBU_map_get_data() extracts the data from the node
 */
inline void *FTBU_map_get_data(FTBU_map_node_t * node)
{
    return node->data;
}


/*
 * The FTBU_map_remove_key() routine remove a node from a map if it matches the
 * supplied key
 */
int FTBU_map_remove_key(FTBU_map_node_t * head, FTBU_map_key_t key)
{
    FTBU_map_node_t *pos;
    FTBU_map_node_t *temp;

    FTBU_list_for_each(pos, head, temp) {
        if (util_key_match(head, pos->key, key)) {
            pos->next->prev = pos->prev;
            pos->prev->next = pos->next;
            free(pos);
            return FTB_SUCCESS;
        }
    }
    return FTBU_NOT_EXIST;
}

/*
 * The routine FTBU_map_remove_iter() removes the specified node from the map
 */
int FTBU_map_remove_node(FTBU_map_node_t * node)
{
    node->next->prev = node->prev;
    node->prev->next = node->next;
    free(node);
    return FTB_SUCCESS;
}


/*
 * The FTBU_map_finalize() routine frees every node in the map, including
 * the headnode
 */
int FTBU_map_finalize(FTBU_map_node_t * head)
{
    FTBU_map_node_t *pos;
    FTBU_map_node_t *temp;

    FTBU_list_for_each(pos, head, temp) {
        pos->next->prev = pos->prev;
        pos->prev->next = pos->next;
        free(pos);
    }
    free(head);
    return FTB_SUCCESS;
}


/*
 * The routine FTBU_map_end() returns the head node of the map. The head
 * node of the map is treated like the last node (since it is a special
 * node not containing any data)
 */
inline FTBU_map_node_t *FTBU_map_end(const FTBU_map_node_t * head)
{
    return (FTBU_map_node_t *) head;
}


/*
 * The routine FTBU_map_next_node() returns the next node in the map
 */
inline FTBU_map_node_t *FTBU_map_next_node(FTBU_map_node_t * node)
{
    return node->next;
}


/*
 * The FTBU_map_is_empty() routine checks if the supplied map is empty
 */
int FTBU_map_is_empty(const FTBU_map_node_t * head)
{
    if (head->next == head)
        return 1;
    else
        return 0;
}


/*
 * The FTBU_list_init() initializes a list
 */
inline void FTBU_list_init(FTBU_list_node_t * list)
{
    list->next = list->prev = list;
}


/*
 * The routine FTBU_list_add_front() adds a node immediately following
 * list, such that list->next points to the new node
 */
inline void FTBU_list_add_front(FTBU_list_node_t * list, FTBU_list_node_t * node)
{
    list->next->prev = node;
    node->next = list->next;
    node->prev = list;
    list->next = node;
}


/*
 * The routine FTBU_list_add_back() adds a node before list, such that
 * node->next points to list
 */
inline void FTBU_list_add_back(FTBU_list_node_t * list, FTBU_list_node_t * node)
{
    list->prev->next = node;
    node->next = list;
    node->prev = list->prev;
    list->prev = node;
}


/*
 * The routine FTBU_list_add_back() removes a node from the list
 */
inline void FTBU_list_remove_node(FTBU_list_node_t * node)
{
    node->next->prev = node->prev;
    node->prev->next = node->next;
}
