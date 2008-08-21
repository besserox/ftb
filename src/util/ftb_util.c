
/*
 * This file provides a set of utility routines that are used
 * throughout the FTB code. All routines in this file start
 * with FTBU_ to indicate that they are utility routines.
 */

#include "ftb_util.h"
#include <assert.h>
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
    FTB_INFO("In FTBU_match_mask");

    if (((strcasecmp(event->region, mask->region) == 0)
	 || (strcasecmp(mask->region, "ALL") == 0))
	&& ((strcasecmp(event->client_jobid, mask->client_jobid) == 0)
	    || (strcasecmp(mask->client_jobid, "ALL") == 0))
	&& ((strcasecmp(event->hostname, mask->hostname) == 0)
	    || (strcasecmp(mask->hostname, "ALL") == 0))) {

	/*
	 * If the mask does not have a wildcard for event_name, then check if the
	 * event names match and return a success on a match and failure otherwise.
	 * If mask has a  wild card for event_name, then check other attributes
	 */

	if (strcasecmp(mask->event_name, "ALL") != 0) {
	    if (strcasecmp(event->event_name, mask->event_name) == 0) {
		FTB_INFO("Out FTBU_match_mask");
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

		FTB_INFO("Out FTBU_match_mask");
		return 1;

	    }
	}
    }
    FTB_INFO("Out FTBU_match_mask");
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
 */
#define FTB_BOOTSTRAP_UTIL_MAX_STR_LEN  128

void FTBU_get_output_of_cmd(const char *cmd, char *output, size_t len)
{
    if (strcasecmp(cmd, "hostname") == 0) {
	char *name = (char *) malloc(len);
	if (gethostname(name, len) == 0) {
	    strncpy(output, name, len);
	}
	else {
	    fprintf(stderr, "gethostname command failed\n");
	}
    }
    else if (strcasecmp(cmd, "date +%m-%d-%H-%M-%S") == 0) {
	char *myDate = (char *) malloc(len);
	time_t myTime = time(NULL);
	if (strftime(myDate, len, "%m-%d-%H-%M-%S", gmtime(&myTime)) != 0) {
	    strncpy(output, myDate, len);
	}
	else {
	    fprintf(stderr, "strftime command failed\n");
	}
    }
    else if (strcasecmp(cmd, "grep ^BG_IP /proc/personality.sh | cut -f2 -d=") == 0) {
	FILE *fp;
	char *pos;
	char str[32];
	int found = 0;

	fp = fopen("/proc/personality.sh", "r");

	if (!fp) {
	    fprintf(stderr, "Could not find /proc/personality.sh\n");
	    return;
	}

	while (!feof(fp)) {
	    fgets(str, 32, fp);

	    if (((pos = strstr(str, "BG_IP=")) != NULL) || ((pos = strstr(str, "BGL_IP=")) != NULL)) {
		while (*pos++ != '=');
		strcpy(output, pos);
		found = 1;
		break;
	    }
	}

	if (!found)
	    fprintf(stderr,
		    "Could not find BG_IP parameter in file /proc/personality.sh on the BG machine");

	fclose(fp);
    }
    else {
	char filename[FTB_BOOTSTRAP_UTIL_MAX_STR_LEN];
	char temp[FTB_BOOTSTRAP_UTIL_MAX_STR_LEN];
	FILE *fp;

	sprintf(filename, "/tmp/temp_file.%d", getpid());
	sprintf(temp, "%s > %s", cmd, filename);
	if (system(temp)) {
	    fprintf(stderr, "execute command failed\n");
	}
	fp = fopen(filename, "r");
	fscanf(fp, "%s", temp);
	fclose(fp);
	unlink(filename);
	strncpy(output, temp, len);
    }
}


/*
 * Checks if the keys for the internal maps (organized as linked lists) match
 */
static inline int util_key_match(const FTBU_map_node_t * headnode, FTBU_map_key_t key1,
				 FTBU_map_key_t key2)
{
    int (*is_equal) (const void *, const void *) = (int (*)(const void *, const void *)) headnode->data;
    assert(is_equal != NULL);
    return (*is_equal) (key1.key_ptr, key2.key_ptr);
}


/*
 * Intialize a map
 */
FTBU_map_node_t *FTBU_map_init(int (*is_equal) (const void *, const void *))
{
    FTBU_map_node_t *node = (FTBU_map_node_t *) malloc(sizeof(FTBU_map_node_t));

    node->data = (void *) is_equal;
    FTBU_list_init((FTBU_list_node_t *) node);
    return node;
}


/*
 * Return the begining node of the map (link list)
 */
inline FTBU_map_iterator_t FTBU_map_begin(const FTBU_map_node_t * headnode)
{
    return (FTBU_map_iterator_t) (headnode->next);
}


/*
 * Return the end (last node) of the map
 */
inline FTBU_map_iterator_t FTBU_map_end(const FTBU_map_node_t * headnode)
{
    return (FTBU_map_iterator_t) headnode;
}


/*
 * Extract the key from the node
 */
inline FTBU_map_key_t FTBU_map_get_key(FTBU_map_iterator_t iterator)
{
    return ((FTBU_map_node_t *) iterator)->key;
}


/*
 * Extract the data from the node
 */
inline void *FTBU_map_get_data(FTBU_map_iterator_t iterator)
{
    return ((FTBU_map_node_t *) iterator)->data;
}


/*
 * Return the next node in the map
 */
inline FTBU_map_iterator_t FTBU_map_next_iterator(FTBU_map_iterator_t iterator)
{
    return (FTBU_map_iterator_t) (((FTBU_map_node_t *) iterator)->next);
}


/*
 * Insert a node in the map
 */
int FTBU_map_insert(FTBU_map_node_t * headnode, FTBU_map_key_t key, void *data)
{
    FTBU_map_node_t *new_node;
    FTBU_list_node_t *pos;
    FTBU_list_node_t *head = (FTBU_list_node_t *) headnode;

    /* Check if the key already exists in the map indicated by headnode */
    FTBU_list_for_each_readonly(pos, head) {
	FTBU_map_node_t *map_node = (FTBU_map_node_t *) pos;
	if (util_key_match(headnode, map_node->key, key))
	    return FTBU_EXIST;
    }

    /* Add a node */
    new_node = (FTBU_map_node_t *) malloc(sizeof(FTBU_map_node_t));
    FTBU_list_add_back(head, (FTBU_list_node_t *) new_node);
    new_node->key = key;
    new_node->data = data;
    return FTBU_SUCCESS;
}


/*
 * Find a node in the map (indictaed by head node) based on the key.
 */
FTBU_map_iterator_t FTBU_map_find(const FTBU_map_node_t * headnode, FTBU_map_key_t key)
{
    FTBU_list_node_t *pos;
    FTBU_list_node_t *head = (FTBU_list_node_t *) headnode;

    FTBU_list_for_each_readonly(pos, head) {
	FTBU_map_node_t *map_node = (FTBU_map_node_t *) pos;
	if (util_key_match(headnode, map_node->key, key))
	    return (FTBU_map_iterator_t) map_node;
    }
    return (FTBU_map_iterator_t) headnode;
}


/*
 * Remove a node from the map based on a key, indicated by headnode
 */
int FTBU_map_remove_key(FTBU_map_node_t * headnode, FTBU_map_key_t key)
{
    FTBU_list_node_t *pos;
    FTBU_list_node_t *temp;
    FTBU_list_node_t *head = (FTBU_list_node_t *) headnode;

    FTBU_list_for_each(pos, head, temp) {
	FTBU_map_node_t *map_node = (FTBU_map_node_t *) pos;
	if (util_key_match(headnode, map_node->key, key)) {
	    FTBU_list_remove_entry(pos);
	    free(map_node);
	    return FTBU_SUCCESS;
	}
    }
    return FTBU_NOT_EXIST;
}


int FTBU_map_remove_iter(FTBU_map_iterator_t iter)
{
    FTBU_map_node_t *map_node = (FTBU_map_node_t *) iter;
    FTBU_list_remove_entry((FTBU_list_node_t *) map_node);
    free(map_node);
    return FTBU_SUCCESS;
}


/*
 * Check if map is empty
 */
int FTBU_map_is_empty(const FTBU_map_node_t * headnode)
{
    if ((FTBU_map_node_t *) headnode->next == headnode)
	return 1;
    else
	return 0;
}


/*
 * Remove all nodes and free memory
 */
int FTBU_map_finalize(FTBU_map_node_t * headnode)
{
    FTBU_list_node_t *pos;
    FTBU_list_node_t *temp;
    FTBU_list_node_t *head = (FTBU_list_node_t *) headnode;

    FTBU_list_for_each(pos, head, temp) {
	FTBU_map_node_t *map_node = (FTBU_map_node_t *) pos;
	FTBU_list_remove_entry(pos);
	free(map_node);
    }
    free(headnode);
    return FTBU_SUCCESS;
}
