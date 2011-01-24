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

#ifndef FTB_UTIL_H
#define FTB_UTIL_H

#include <stdio.h>
#include "ftb_def.h"

/* *INDENT-OFF* */
#ifdef __cplusplus
extern "C" {
#endif
/* *INDENT-ON* */

#define FTBU_ERR_ABORT(args...)  do {\
	char hostname[32]; \
	FTBU_get_output_of_cmd("hostname", hostname, 32); \
    fprintf(FTBU_log_file_fp, "[FTB_ERROR][%s: line %d][hostname:%s]", __FILE__, __LINE__,hostname); \
    fprintf(FTBU_log_file_fp, args); \
    fprintf(FTBU_log_file_fp, "\n"); \
    fflush(FTBU_log_file_fp);\
    exit(-1);\
} while (0)

#define FTBU_WARNING(args...)  do {\
	char hostname[32]; \
	FTBU_get_output_of_cmd("hostname", hostname, 32); \
    fprintf(FTBU_log_file_fp, "[FTBU_WARNING][%s: line %d][hostname:%s]", __FILE__, __LINE__,hostname); \
    fprintf(FTBU_log_file_fp, args); \
    fprintf(FTBU_log_file_fp, "\n"); \
    fflush(FTBU_log_file_fp);\
} while (0)

#ifdef FTB_DEBUG
#define FTBU_INFO(args...)  do {\
	char hostname[32]; \
	FTBU_get_output_of_cmd("hostname", hostname, 32); \
    fprintf(FTBU_log_file_fp, "[FTBU_INFO][%s: line %d][hostname:%s]", __FILE__, __LINE__,hostname); \
    fprintf(FTBU_log_file_fp, args); \
    fprintf(FTBU_log_file_fp, "\n"); \
    fflush(FTBU_log_file_fp);\
} while (0)
#else
#define FTBU_INFO(...)
#endif

/* Simple list-based implementation of map, set is just map with data as key */
#define FTBU_SUCCESS       0
#define FTBU_EXIST         2
#define FTBU_NOT_EXIST     3

#define FTBU_NULL_PTR	 (-1)

#define FTBU_MAP_PTR_KEY(x)        ((FTBU_map_key_t)(void*)(x))

typedef union FTBU_map_key {
    void *key_ptr;
} FTBU_map_key_t;

/*
*  The FTB_map_node_t data structure is used the create nodes in a
*  'map-type' (ftb-terminology) doubly linked list. Typically, all nodes
*  in a linked list contain data of similar semantics. In the 'map-type'
*  doubly linked list, the elements of the first/head node of the linked
*  list have unique semantics. More specifically:
*  (1) The 'FTBU_map_key_t key' will be ignored for the first node of the
*  linked list.
*  (2) The 'void *data' will actually point to a 'comparison' function
*  that will be used for comparing data available on the other nodes of
*  the linked list. Different instances of the FTBU_map_node will thus
*  contain data whose type and comparison criteria will be defined in the
*  function available in this 'data' element of the head node.
*
*  The other nodes of the 'map-type' linked list will be semantically
*  homogenous and will some information in the  'key' and 'data' elements.
*
*/

typedef struct FTBU_map_node {
    struct FTBU_map_node *next;
    struct FTBU_map_node *prev;
    FTBU_map_key_t key;
    void *data;                 /*For head node, data is a func ptr that points to a comparison function */
} FTBU_map_node_t;


/*
 * The FTBU_list_node_t will be used to create instances of the doubly linked
 * list; where the semantics of all nodes are same
 * This data type is created is order to clearly distinguish it from the
 * 'map-type' linked list
 */
typedef struct FTBU_list_node {
    struct FTBU_list_node *next;
    struct FTBU_list_node *prev;
    void *data;
} FTBU_list_node_t;

#define FTBU_list_for_each_readonly(pos, head) \
    for (pos=head->next; pos!=head; pos=pos->next)

#define FTBU_list_for_each(pos, head, temp) \
    for (pos=head->next, temp=pos->next; pos!=head; pos=temp, temp=temp->next)

/* Initialize a map */
FTBU_map_node_t *FTBU_map_init(int (*is_equal) (const void *, const void *));

/* Return the beginning of the map linked list */
FTBU_map_node_t *FTBU_map_begin(const FTBU_map_node_t * head);

/* Return the last node of the map linked list */
FTBU_map_node_t *FTBU_map_end(const FTBU_map_node_t * head);

/* Get the key from the node*/
FTBU_map_key_t FTBU_map_get_key(FTBU_map_node_t * node);

/* Get data from the node */
void *FTBU_map_get_data(FTBU_map_node_t * node);

/* Get the next node */
FTBU_map_node_t *FTBU_map_next_node(FTBU_map_node_t * node);

/*
 * Insert value in the map. Returns FTBU_SUCCESS if new element inserted
 * or FTBU_EXIST if the same element is already in the map
 */
int FTBU_map_insert(FTBU_map_node_t * head, FTBU_map_key_t key, void *data);

/*
 * Find whether a same element is in the map. Returns the position of the match node if
 * it exists or a pointer pointing to the head node if it does not exist
 */
FTBU_map_node_t *FTBU_map_find_key(const FTBU_map_node_t * head, FTBU_map_key_t key);

/*
 * Remove one element from the map. Returns FTBU_SUCCESS if a same
 * element is removed or FTBU_NOT_EXIST if no such element is in the map
 */
int FTBU_map_remove_key(FTBU_map_node_t * head, FTBU_map_key_t key);

/* Remove the element pointed by node from the map */
int FTBU_map_remove_node(FTBU_map_node_t * node);

/* Test whether the map is empty. Returns 1 when it is empty, 0 if it is not */
int FTBU_map_is_empty(const FTBU_map_node_t * head);

/* Finalize the map, will free all map node. Note: the data will not be freed */
int FTBU_map_finalize(FTBU_map_node_t * head);

/* Initialize a list */
void FTBU_list_init(FTBU_list_node_t * list);

/* Add node in front of list node */
void FTBU_list_add_front(FTBU_list_node_t * list, FTBU_list_node_t * node);

/* Add node behind list node */
void FTBU_list_add_back(FTBU_list_node_t * list, FTBU_list_node_t * node);

/* Remove a node from a list */
void FTBU_list_remove_node(FTBU_list_node_t * node);

int FTBU_match_mask(const FTB_event_t * event, const FTB_event_t * mask);

int FTBU_is_equal_location_id(const FTB_location_id_t * lhs, const FTB_location_id_t * rhs);

int FTBU_is_equal_ftb_id(const FTB_id_t * lhs, const FTB_id_t * rhs);

int FTBU_is_equal_event(const FTB_event_t * lhs, const FTB_event_t * rhs);

int FTBU_is_equal_event_mask(const FTB_event_t * lhs, const FTB_event_t * rhs);

int FTBU_is_equal_clienthandle(const FTB_client_handle_t * lhs, const FTB_client_handle_t * rhs);

void FTBU_get_output_of_cmd(const char *cmd, char *output, size_t len);

/* *INDENT-OFF* */
#ifdef __cplusplus
} /*extern "C"*/
#endif
/* *INDENT-ON* */

#endif
