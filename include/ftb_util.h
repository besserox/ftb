#ifndef FTB_UTIL_H
#define FTB_UTIL_H

#include "ftb.h"

#ifdef __cplusplus
extern "C" {
#endif 

#define FTB_ERR_ABORT(args...)  do {\
    fprintf(stderr, "[FTB_ERROR][%s: line %d]", __FILE__, __LINE__); \
    fprintf(stderr, args); \
    fprintf(stderr, "\n"); \
    exit(-1);\
}while(0)

#define FTB_WARNING(args...)  do {\
    fprintf(stderr, "[FTB_WARNING][%s: line %d]", __FILE__, __LINE__); \
    fprintf(stderr, args); \
    fprintf(stderr, "\n"); \
}while(0)

#define FTB_DEBUG
#ifdef FTB_DEBUG
#define FTB_INFO(args...)  do {\
    fprintf(stderr, "[FTB_INFO][%s: line %d]", __FILE__, __LINE__); \
    fprintf(stderr, args); \
    fprintf(stderr, "\n"); \
}while(0)
#else
#define FTB_INFO(...)
#endif

typedef struct FTB_util_list_node {
    struct FTB_util_list_node *next;
    struct FTB_util_list_node *prev;
}FTB_util_list_node_t;

static inline void FTBU_list_init(FTB_util_list_node_t *list)
{
    list->next = list->prev = list;
}

static inline void FTBU_list_add_front(FTB_util_list_node_t *list, FTB_util_list_node_t *entry)
{
     list->next->prev = entry;
     entry->next = list->next;
     entry->prev = list;
     list->next = entry;
}

static inline void FTBU_list_add_back(FTB_util_list_node_t *list, FTB_util_list_node_t *entry)
{
     list->prev->next = entry;
     entry->next = list;
     entry->prev = list->prev;
     list->prev = entry;
}

static inline void FTBU_list_remove_entry(FTB_util_list_node_t *entry)
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
#define FTBU_BREAK           1
#define FTBU_EXIST            2
#define FTBU_NOT_EXIST    3
    
typedef union FTB_util_map_key{
    void* key_ptr;
    uint32_t key_int;
}FTB_util_map_key_t;

typedef struct FTB_util_map_node{
    struct FTB_util_list_node *next;
    struct FTB_util_list_node *prev;
    FTB_util_map_key_t key;
    void* data; /*For head node, data will be pointing to the is_equal function*/
}FTB_util_map_node_t;

typedef FTB_util_map_node_t* FTB_util_map_iterator_t;
    
/*Initialize the map, if is_equal == NULL, use the integ*/
FTB_util_map_node_t *FTBU_map_init(int (*is_equal)(const void *, const void *));

/*Get the iterator to first element*/
FTB_util_map_iterator_t FTBU_map_begin(FTB_util_map_node_t * headnode);

/*Get the iterator to the end symbol*/
FTB_util_map_iterator_t FTBU_map_end(FTB_util_map_node_t * headnode);

/*Get map_key from the iterator*/
FTB_util_map_key_t FTBU_map_get_key(FTB_util_map_iterator_t iterator);

/*Get data from the iterator*/
void* FTBU_map_get_data(FTB_util_map_iterator_t iterator);

/*Get the next iterator*/
FTB_util_map_iterator_t FTBU_map_next_iterator(FTB_util_map_iterator_t iterator);

/*Insert val to the map. Returns FTBU_SUCCESS if new element inserted or FTBU_EXIST if the same element is already in the map*/
int FTBU_map_insert(FTB_util_map_node_t * headnode, FTB_util_map_key_t key, void *data);

/*Find whether a same element is in the map. Returns its iterator if it exists or the iterator pointing to FTBU_map_end if not*/
FTB_util_map_iterator_t FTBU_map_find(FTB_util_map_node_t * headnode, FTB_util_map_key_t key);

/*Remove one element from the map. Returns FTBU_SUCCESS if a same element is removed or FTBU_NOT_EXIST if no such element is in the map*/
int FTBU_map_remove(FTB_util_map_node_t * headnode, FTB_util_map_key_t key);

/*Test wether the map is empty. Returns non-zero when it is empty, 0 if it is not*/
int FTBU_map_is_empty(FTB_util_map_node_t * headnode);

/*Finalize the map, will free all map node. Note: the data will not be freed*/
int FTBU_map_finalize(FTB_util_map_node_t * headnode);


int FTBU_match_mask(const FTB_event_inst_t *event_inst, const FTB_event_mask_t *mask);

#ifdef __cplusplus
} /*extern "C"*/
#endif 

#endif
