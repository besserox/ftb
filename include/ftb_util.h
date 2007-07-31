#ifndef FTB_UTIL_H
#define FTB_UTIL_H

#include "ftb.h"
#include "ftb_conf.h"

typedef struct FTB_list_node {
	struct FTB_list_node *next;
	struct FTB_list_node *prev;
}FTB_list_node_t;

static inline void FTB_list_init(FTB_list_node_t *list)
{
    list->next = list->prev = list;
}

static inline void FTB_list_add_front(FTB_list_node_t *list, FTB_list_node_t *entry)
{
     list->next->prev = entry;
     entry->next = list->next;
     entry->prev = list;
     list->next = entry;
}

static inline void FTB_list_add_back(FTB_list_node_t *list, FTB_list_node_t *entry)
{
     list->prev->next = entry;
     entry->next = list;
     entry->prev = list->prev;
     list->prev = entry;
}

static inline void FTB_list_remove_entry(FTB_list_node_t *entry)
{
    entry->next->prev = entry->prev;
    entry->prev->next = entry->next;
}

#define FTB_list_for_each_readonly(pos, head) \
    for (pos=head->next; pos!=head; pos=pos->next)
    
#define FTB_list_for_each(pos, head, temp) \
    for (pos=head->next, temp=pos->next; pos!=head; pos=temp, temp=temp->next)

int FTB_Util_match_mask(FTB_event_inst_t *event_inst, FTB_event_mask_t *mask);

int FTB_Util_setup_configuration(FTB_config_t* config);

#endif
