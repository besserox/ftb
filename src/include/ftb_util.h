#ifndef FTB_UTIL_H
#define FTB_UTIL_H

#include <stdio.h>
#include "ftb_def.h"

#ifdef __cplusplus
extern "C" {
#endif 

#define FTB_ERR_ABORT(args...)  do {\
    fprintf(FTBU_log_file_fp, "[FTB_ERROR][%s: line %d]", __FILE__, __LINE__); \
    fprintf(FTBU_log_file_fp, args); \
    fprintf(FTBU_log_file_fp, "\n"); \
    fflush(FTBU_log_file_fp);\
    exit(-1);\
}while(0)

#define FTB_WARNING(args...)  do {\
    fprintf(FTBU_log_file_fp, "[FTB_WARNING][%s: line %d]", __FILE__, __LINE__); \
    fprintf(FTBU_log_file_fp, args); \
    fprintf(FTBU_log_file_fp, "\n"); \
    fflush(FTBU_log_file_fp);\
}while(0)

#ifdef FTB_DEBUG
#define FTB_INFO(args...)  do {\
    fprintf(FTBU_log_file_fp, "[FTB_INFO][%s: line %d]", __FILE__, __LINE__); \
    fprintf(FTBU_log_file_fp, args); \
    fprintf(FTBU_log_file_fp, "\n"); \
    fflush(FTBU_log_file_fp);\
}while(0)
#else
#define FTB_INFO(...)
#endif

typedef struct FTBU_list_node {
    struct FTBU_list_node *next;
    struct FTBU_list_node *prev;
}FTBU_list_node_t;

static inline void FTBU_list_init(FTBU_list_node_t *list)
{
    list->next = list->prev = list;
}

static inline void FTBU_list_add_front(FTBU_list_node_t *list, FTBU_list_node_t *entry)
{
     list->next->prev = entry;
     entry->next = list->next;
     entry->prev = list;
     list->next = entry;
}

static inline void FTBU_list_add_back(FTBU_list_node_t *list, FTBU_list_node_t *entry)
{
     list->prev->next = entry;
     entry->next = list;
     entry->prev = list->prev;
     list->prev = entry;
}

static inline void FTBU_list_remove_entry(FTBU_list_node_t *entry)
{
    entry->next->prev = entry->prev;
    entry->prev->next = entry->next;
}

#define FTBU_list_for_each_readonly(pos, head) \
    for (pos=head->next; pos!=head; pos=pos->next)
    
#define FTBU_list_for_each(pos, head, temp) \
    for (pos=head->next, temp=pos->next; pos!=head; pos=temp, temp=temp->next)

/*Simple list-based implementation of map, set is just map with data as key*/
#define FTBU_SUCCESS       0
#define FTBU_EXIST            2
#define FTBU_NOT_EXIST    3
    
typedef union FTBU_map_key{
    void* key_ptr;
    uint32_t key_int;
}FTBU_map_key_t;

typedef struct FTBU_map_node{
    struct FTBU_list_node *next;
    struct FTBU_list_node *prev;
    FTBU_map_key_t key;
    void* data; /*For head node, data will be pointing to the is_equal function*/
}FTBU_map_node_t;

typedef FTBU_map_node_t* FTBU_map_iterator_t;

#define FTBU_MAP_UINT_KEY(x)        ((FTBU_map_key_t)(uint32_t)(x))
#define FTBU_MAP_PTR_KEY(x)        ((FTBU_map_key_t)(void*)(x))
    
/*Initialize the map, if is_equal == NULL, use the integ*/
FTBU_map_node_t *FTBU_map_init(int (*is_equal)(const void *, const void *));

/*Get the iterator to first element*/
FTBU_map_iterator_t FTBU_map_begin(const FTBU_map_node_t * headnode);

/*Get the iterator to the end symbol*/
FTBU_map_iterator_t FTBU_map_end(const FTBU_map_node_t * headnode);

/*Get map_key from the iterator*/
FTBU_map_key_t FTBU_map_get_key(FTBU_map_iterator_t iterator);

/*Get data from the iterator*/
void* FTBU_map_get_data(FTBU_map_iterator_t iterator);

/*Get the next iterator*/
FTBU_map_iterator_t FTBU_map_next_iterator(FTBU_map_iterator_t iterator);

/*Insert val to the map. Returns FTBU_SUCCESS if new element inserted or FTBU_EXIST if the same element is already in the map*/
int FTBU_map_insert(FTBU_map_node_t * headnode, FTBU_map_key_t key, void *data);

/*Find whether a same element is in the map. Returns its iterator if it exists or the iterator pointing to FTBU_map_end if not*/
FTBU_map_iterator_t FTBU_map_find(const FTBU_map_node_t * headnode, FTBU_map_key_t key);

/*Remove one element from the map. Returns FTBU_SUCCESS if a same element is removed or FTBU_NOT_EXIST if no such element is in the map*/
int FTBU_map_remove_key(FTBU_map_node_t * headnode, FTBU_map_key_t key);

/*Remove the element pointed by iter from the map*/
int FTBU_map_remove_iter(FTBU_map_iterator_t iter);

/*Test wether the map is empty. Returns non-zero when it is empty, 0 if it is not*/
int FTBU_map_is_empty(const FTBU_map_node_t * headnode);

/*Finalize the map, will free all map node. Note: the data will not be freed*/
int FTBU_map_finalize(FTBU_map_node_t * headnode);

int FTBU_match_mask(const FTB_event_t *event, const FTB_event_t *mask);

int FTBU_is_equal_location_id(const FTB_location_id_t *lhs, const FTB_location_id_t *rhs);

int FTBU_is_equal_ftb_id(const FTB_id_t *lhs, const FTB_id_t *rhs);

int FTBU_is_equal_event(const FTB_event_t *lhs, const FTB_event_t *rhs);

void FTBU_get_output_of_cmd(const char *cmd, char *output, size_t len);

#ifdef __cplusplus
} /*extern "C"*/
#endif 

#endif
