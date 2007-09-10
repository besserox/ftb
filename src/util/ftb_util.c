#include "ftb_util.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern FILE* FTBU_log_file_fp;

int FTBU_match_mask(const FTB_event_t *event, const FTB_event_t *mask)
{
    FTB_event_name_t temp = mask->event_name+1;
    if (temp != 0) {
        /* event name is not masked to match all */
        return (mask->event_name == event->event_name);
    }
    if ( (event->severity & mask->severity) != 0
     && (event->severity & ~mask->severity) == 0
     && (event->comp_cat & mask->comp_cat) != 0
     && (event->comp_cat & ~mask->comp_cat) == 0
     && (event->comp & mask->comp) != 0
     && (event->comp & ~mask->comp) == 0
     && (event->event_cat & mask->event_cat) != 0
     && (event->event_cat & ~mask->event_cat) == 0)
        return 1;
    else 
        return 0;
}

int FTBU_is_equal_location_id(const FTB_location_id_t *lhs, const FTB_location_id_t *rhs)
{
    if (lhs->pid == rhs->pid) {
        if (strncmp(lhs->hostname, rhs->hostname, FTB_MAX_HOST_NAME) == 0)
            return 1;
    }
    return 0;
}

int FTBU_is_equal_ftb_id(const FTB_id_t *lhs, const FTB_id_t *rhs)
{
    if (lhs->client_id.comp == rhs->client_id.comp
   && lhs->client_id.comp_cat == rhs->client_id.comp_cat
   && lhs->client_id.ext == rhs->client_id.ext) {
        return FTBU_is_equal_location_id(&lhs->location_id, &rhs->location_id);
   }
   return 0;     
}

int FTBU_is_equal_event(const FTB_event_t *lhs, const FTB_event_t *rhs)
{
    if ( (lhs->severity == rhs->severity)
     && (lhs->comp_cat == rhs->comp_cat) 
     && (lhs->comp == rhs->comp) 
     && (lhs->event_cat == rhs->event_cat)
     && (lhs->event_name == rhs->event_name) )
        return 1;
    else 
        return 0;
}

#define FTB_BOOTSTRAP_UTIL_MAX_STR_LEN  128

void FTBU_get_output_of_cmd(const char *cmd, char *output, size_t len)
{
    /*Not very neat implementation*/
    char filename[FTB_BOOTSTRAP_UTIL_MAX_STR_LEN];
    char temp[FTB_BOOTSTRAP_UTIL_MAX_STR_LEN];
    FILE *fp;
    
    sprintf(filename,"/tmp/temp_file.%d",getpid());
    sprintf(temp,"%s > %s",cmd, filename);
    if (system(temp)) {
        fprintf(stderr,"execute command failed\n");
    }
    fp = fopen(filename, "r");
    fscanf(fp,"%s",temp);
    fclose(fp);
    unlink(filename);
    strncpy(output,temp,len);
}

   
static inline int util_key_match(const FTBU_map_node_t * headnode, FTBU_map_key_t key1, FTBU_map_key_t key2)
{
    if (headnode->key.key_int == 1)
        return (key1.key_int == key2.key_int);
    else {
        int (*is_equal)(const void *, const void *) = (int (*)(const void*, const void *)) headnode->data;
        assert(is_equal!= NULL);
        return (*is_equal)(key1.key_ptr, key2.key_ptr);
    }
}

FTBU_map_node_t *FTBU_map_init(int (*is_equal)(const void *, const void *))
{
    FTBU_map_node_t *node = (FTBU_map_node_t *)malloc(sizeof(FTBU_map_node_t));
    if (is_equal == NULL)
        node->key.key_int = 1;
    else
        node->data = (void*)is_equal;
    FTBU_list_init((FTBU_list_node_t*)node);
    return node;
}

inline FTBU_map_iterator_t FTBU_map_begin(const FTBU_map_node_t * headnode)
{
    return (FTBU_map_iterator_t)(headnode->next);
}

inline FTBU_map_iterator_t FTBU_map_end(const FTBU_map_node_t * headnode)
{
    return (FTBU_map_iterator_t)headnode;
}

inline FTBU_map_key_t FTBU_map_get_key(FTBU_map_iterator_t iterator)
{
    return ((FTBU_map_node_t *)iterator)->key;
}

inline void* FTBU_map_get_data(FTBU_map_iterator_t iterator)
{
    return ((FTBU_map_node_t *)iterator)->data;
}

inline FTBU_map_iterator_t FTBU_map_next_iterator(FTBU_map_iterator_t iterator)
{
    return (FTBU_map_iterator_t)(((FTBU_map_node_t *)iterator)->next);
}

int FTBU_map_insert(FTBU_map_node_t * headnode, FTBU_map_key_t key, void* data)
{
    FTBU_map_node_t* new_node;
    FTBU_list_node_t* pos;
    FTBU_list_node_t* head = (FTBU_list_node_t*)headnode;
    
    FTBU_list_for_each_readonly(pos, head) {
        FTBU_map_node_t *map_node = (FTBU_map_node_t*)pos;
        if (util_key_match(headnode,map_node->key,key))
            return FTBU_EXIST;
    }
    new_node = (FTBU_map_node_t*)malloc(sizeof(FTBU_map_node_t));
    FTBU_list_add_back(head, (FTBU_list_node_t *)new_node);
    new_node->key = key;
    new_node->data = data;
    return FTBU_SUCCESS;
}

FTBU_map_iterator_t FTBU_map_find(const FTBU_map_node_t * headnode, FTBU_map_key_t key)
{
    FTBU_list_node_t* pos;
    FTBU_list_node_t* head = (FTBU_list_node_t*)headnode;

    FTBU_list_for_each_readonly(pos, head) {
        FTBU_map_node_t *map_node = (FTBU_map_node_t*)pos;
        if (util_key_match(headnode,map_node->key,key))
            return (FTBU_map_iterator_t)map_node;
    }
    return (FTBU_map_iterator_t)headnode;
}

int FTBU_map_remove_key(FTBU_map_node_t * headnode, FTBU_map_key_t key)
{
    FTBU_list_node_t* pos;
    FTBU_list_node_t* temp;
    FTBU_list_node_t* head = (FTBU_list_node_t*)headnode;

    FTBU_list_for_each(pos, head, temp) {
        FTBU_map_node_t *map_node = (FTBU_map_node_t*)pos;
        if (util_key_match(headnode,map_node->key,key)) {
            FTBU_list_remove_entry(pos);
            free(map_node);
            return FTBU_SUCCESS;
        }
    }
    return FTBU_NOT_EXIST;
}

int FTBU_map_remove_iter(FTBU_map_iterator_t iter)
{
    FTBU_map_node_t* map_node = (FTBU_map_node_t*)iter;
    FTBU_list_remove_entry((FTBU_list_node_t*)map_node);
    free(map_node);
    return FTBU_SUCCESS;
}

int FTBU_map_is_empty(const FTBU_map_node_t * headnode)
{
    if ((FTBU_map_node_t *)headnode->next == headnode)
        return 1;
    else
        return 0;
}

int FTBU_map_finalize(FTBU_map_node_t * headnode)
{
    FTBU_list_node_t* pos;
    FTBU_list_node_t* temp;
    FTBU_list_node_t* head = (FTBU_list_node_t*)headnode;

    FTBU_list_for_each(pos, head, temp) {
        FTBU_map_node_t *map_node = (FTBU_map_node_t*)pos;
        FTBU_list_remove_entry(pos);
        free(map_node);
    }
    free(headnode);
    return FTBU_SUCCESS;
}
