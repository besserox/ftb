#include "ftb_util.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

extern FILE* FTBU_log_file_fp;

int FTBU_match_mask(const FTB_event_t *event, const FTB_event_t *mask)
{  
    FTB_INFO("In FTBU_match_mask"); 

    if (((strcasecmp(event->region, mask->region) == 0) ||
            (strcasecmp(mask->region, "ALL") == 0)) &&
            ((strcasecmp(event->client_jobid, mask->client_jobid) == 0) ||
             (strcasecmp(mask->client_jobid, "ALL") ==0)) &&
            //((strcasecmp(event->client_name, mask->client_name) == 0) ||
            // (strcasecmp(mask->client_name, "ALL") == 0)) &&
            ((strcasecmp(event->hostname, mask->hostname) == 0) ||
             (strcasecmp(mask->hostname, "ALL") == 0))) {
                if (strcasecmp(mask->event_name, "ALL") != 0) {
                    if (strcasecmp(event->event_name, mask->event_name) == 0) {
                        return 1;
                    }
                }
                else {
                    if (((strcasecmp(event->severity, mask->severity) == 0) || 
                        (strcasecmp(mask->severity, "ALL") == 0)) &&
                        ((strcasecmp(event->comp_cat, mask->comp_cat) == 0) || 
                        (strcasecmp(mask->comp_cat, "ALL") == 0)) &&
                        ((strcasecmp(event->comp, mask->comp) == 0) || 
                        (strcasecmp(mask->comp, "ALL") == 0))) {
                            return 1;
                    }
                }
    }
    return 0;
}

int FTBU_is_equal_location_id(const FTB_location_id_t *lhs, const FTB_location_id_t *rhs)
{
    if (lhs->pid == rhs->pid) {
       if ((strncasecmp(lhs->hostname, rhs->hostname, FTB_MAX_HOST_NAME) == 0)
            && (strncasecmp(lhs->pid_starttime, rhs->pid_starttime, FTB_MAX_PID_STARTTIME) == 0))
            return 1;
    }
    return 0;
}

int FTBU_is_equal_ftb_id(const FTB_id_t *lhs, const FTB_id_t *rhs)
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

int FTBU_is_equal_event(const FTB_event_t *lhs, const FTB_event_t *rhs)
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

int FTBU_is_equal_clienthandle(const FTB_client_handle_t *lhs, const FTB_client_handle_t *rhs)
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

#define FTB_BOOTSTRAP_UTIL_MAX_STR_LEN  128
void FTBU_get_output_of_cmd(const char *cmd, char *output, size_t len)
{
    
    if (strcasecmp(cmd, "hostname") == 0) {
   	 char *name = (char *)malloc(len);
	 if (gethostname(name, len) == 0) {
		strncpy(output, name, len);
	 }
         else {
	     fprintf(stderr,"gethostname command failed\n");
    	 }
    }
    else if (strcasecmp(cmd, "date +%m-%d-%H-%M-%S") == 0) {
            char *myDate = (char *)malloc(len);
            time_t myTime = time(NULL);
            if(strftime(myDate, len, "%m-%d-%H-%M-%S", gmtime(&myTime)) != 0) {
                strncpy(output, myDate, len);
            }
            else {
                fprintf(stderr,"strftime command failed\n");
            }
    }
   else if (strcasecmp(cmd, "grep ^BGL_IP /proc/personality.sh | cut -f2 -d=") == 0) {
        FILE *fp;
        char *pos;
        char str[32], substr[32];
        int i=0, index=0;

        fp = fopen("/proc/personality.sh", "r");
        while (!feof(fp)) {
                fgets(str, 32, fp);
                if ((pos=strstr(str, "BGL_IP=")) != NULL) {
                        index = str-pos;
                        while (str[index++] != '=');
                        while (str[index] != '\n') {output[i++]= str[index++];}
                        output[i]= '\0';
                        break;
                }
                else {
                   fprintf(stderr, "Could not find BGL_IP parameter in file /proc/personality.sh on the BGL machine");
                } 
        }
        fclose(fp);
    }
    else {
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
}

   
static inline int util_key_match(const FTBU_map_node_t * headnode, FTBU_map_key_t key1, FTBU_map_key_t key2)
{
    /*if (headnode->key.key_int == 1)
        return (key1.key_int == key2.key_int);
    else {
        int (*is_equal)(const void *, const void *) = (int (*)(const void*, const void *)) headnode->data;
        assert(is_equal!= NULL);
        return (*is_equal)(key1.key_ptr, key2.key_ptr);
    }
    */
        int (*is_equal)(const void *, const void *) = (int (*)(const void*, const void *)) headnode->data;
        assert(is_equal!= NULL);
        return (*is_equal)(key1.key_ptr, key2.key_ptr);
}

FTBU_map_node_t *FTBU_map_init(int (*is_equal)(const void *, const void *))
{
    FTBU_map_node_t *node = (FTBU_map_node_t *)malloc(sizeof(FTBU_map_node_t));
    /*
    if (is_equal == NULL)
        node->key.key_int = 1;
    else
        node->data = (void*)is_equal;
    */
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
