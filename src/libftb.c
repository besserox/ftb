#include "libftb.h"

typedef struct FTB_client_event_mask_list {
    FTB_list_node_t *next;
    FTB_list_node_t *prev;
    FTB_event_mask_t mask;
#ifndef FTB_CLIENT_POLLING_ONLY
    int (*callback)(FTB_event_inst_t *, void*);
    void *callback_arg;
#endif    
}FTB_client_event_mask_list_t;

typedef struct FTB_client_runtime {
    int fd; /*0 means not funcitonal*/
    FTB_component_properties_t properties;
    FTB_config_t *config;
#ifndef FTB_CLIENT_POLLING_ONLY
    pthread_t catch_thread;
    pthread_mutex_t lock;
    FTB_list_node_t *event_notify_list;
#endif    
    FTB_list_node_t *event_polling_list;
}FTB_client_runtime_t;

static FTB_client_runtime_t *FTB_client_runtime = NULL;

/*In case of an error with FTB, the client module will stop work,
 *but the client component should continue by default */

#define UTIL_READ_SHORT(fd_read,buf,len)  do { \
    if (fd_read != 0) {\
        if (read(fd_read, buf, len) != len) {  \
            FTB_WARNING("read error %s (%d)\n", strerror(errno), errno); \
            close(fd_read);\
            FTB_client_runtime->fd = 0; \
        } \
   } \
}while(0)

#define UTIL_WRITE_SHORT(fd_write,buf,len)  do { \
    if (fd_write != 0) {\
        if (write(fd_write, buf, len) != len) {  \
            FTB_WARNING("write error %s (%d)\n", strerror(errno), errno);        \
            close(fd_write);\
            FTB_client_runtime->fd = 0; \
        } \
    }\
}while(0)

#define UTIL_ERROR_CHECKING do {\
    if (FTB_client_runtime->fd == 0) {\
        return 1;\
    }\
}while(0)

static inline void lock_ifneeded()
{
#ifndef FTB_CLIENT_POLLING_ONLY
    pthread_mutex_lock(&FTB_client_runtime->lock);
#endif
}

static inline void unlock_ifneeded()
{
#ifndef FTB_CLIENT_POLLING_ONLY
    pthread_mutex_unlock(&FTB_client_runtime->lock);
#endif
}

int connect_server(FTB_client_runtime_t *runtime)
{
    int optval = 1;
    struct hostent *hp;
    struct sockaddr_in sa;
    char buf[FTB_MAX_EVENT_VERSION_NAME];

    runtime->fd = socket(AF_INET, SOCK_STREAM, 0 );
    if (runtime->fd < 0) {
        FTB_ERR_ABORT("socket creation error");
    }

    hp = gethostbyname(runtime->config->host_name);
    memset( (void *)&sa, 0, sizeof(sa) );
    memcpy( (void *)&sa.sin_addr, (void *)hp->h_addr, hp->h_length);
    sa.sin_family = AF_INET;
    sa.sin_port = htons(runtime->config->port);
    if (setsockopt( runtime->fd, IPPROTO_TCP, TCP_NODELAY, (char *)&optval, sizeof(optval) ))
    	FTB_ERR_ABORT("setsockopt failed\n");
    if (connect( runtime->fd, (struct sockaddr *)&sa, sizeof(sa) ) < 0)
    	FTB_ERR_ABORT("connect %s:%d failed\n",runtime->config->host_name, runtime->config->port);
    UTIL_WRITE_SHORT(runtime->fd, &(runtime->config->FTB_id), sizeof(runtime->config->FTB_id));
    strncpy(buf,FTB_EVENT_VERSION,FTB_MAX_EVENT_VERSION_NAME);
    UTIL_WRITE_SHORT(runtime->fd, buf, FTB_MAX_EVENT_VERSION_NAME);
    UTIL_WRITE_SHORT(runtime->fd, &(runtime->properties), sizeof(FTB_component_properties_t));
    FTB_INFO("server connected");
    return 0;
}

#ifndef FTB_CLIENT_POLLING_ONLY
void *catch_thread(void *arg)
{
    uint32_t msg_type;
    FTB_event_inst_t event_inst;
    while (1) {
        fd_set fds;
        UTIL_ERROR_CHECKING;
        FD_ZERO(&fds);
        FD_SET(FTB_client_runtime->fd, &fds);
        if (select(FTB_client_runtime->fd+1, &fds, NULL, NULL, NULL)<0) {
            if (errno == EINTR)
                continue;
            FTB_ERR_ABORT("select failed\n");
        }
        if (!FD_ISSET(FTB_client_runtime->fd, &fds))
            continue;
        lock_ifneeded();
        UTIL_READ_SHORT(FTB_client_runtime->fd,&msg_type,sizeof(msg_type));
        if (msg_type==  FTB_MSG_TYPE_NOTIFY) {
            UTIL_READ_SHORT(FTB_client_runtime->fd, &event_inst,sizeof(FTB_event_inst_t));
            FTB_INFO("caught event id: %d, name: %s",event_inst.event_id, event_inst.name);
            /*find first match of callback*/
            {
                FTB_list_node_t *node;
                FTB_list_for_each_readonly(node, FTB_client_runtime->event_notify_list) 
                {
                    FTB_client_event_mask_list_t *entry = (FTB_client_event_mask_list_t *)node;
                    if (FTB_Util_match_mask(&event_inst, &(entry->mask))) {
                        if (entry->callback != NULL) {
                            FTB_INFO("calling callback function");
                            (*entry->callback)(&event_inst,entry->callback_arg);
                        }
                        break;
                    }
                }
            }
        }
        else {
    	    FTB_WARNING("Unknown message type %d", msg_type);
        }
        unlock_ifneeded();
    }
    return NULL;
}
#endif

int FTB_Init(FTB_component_properties_t *properties)
{
    if (FTB_client_runtime != NULL) {
        FTB_WARNING("already initialized");
        return 1;
    }
    if (properties->com_namespace == 0
    || properties->id == 0) {
        FTB_WARNING("namespace or id cannot be 0");
        return 1;
    }
#ifdef FTB_CLIENT_POLLING_ONLY
    properties->polling_only = 1;
#endif
    FTB_client_runtime = (FTB_client_runtime_t*)malloc(sizeof(FTB_client_runtime_t));
    memcpy(&(FTB_client_runtime->properties),properties,sizeof(FTB_component_properties_t));
    FTB_client_runtime->config = (FTB_config_t*)malloc(sizeof(FTB_config_t));
    FTB_Util_setup_configuration(FTB_client_runtime->config);
    connect_server(FTB_client_runtime);
#ifndef FTB_CLIENT_POLLING_ONLY
    if (!properties->polling_only) {
        pthread_create(&(FTB_client_runtime->catch_thread), NULL, catch_thread, NULL);
        pthread_mutex_init(&(FTB_client_runtime->lock),NULL);
        FTB_client_runtime->event_notify_list = (FTB_list_node_t*)malloc(sizeof(FTB_list_node_t));
        FTB_list_init(FTB_client_runtime->event_notify_list);
    }
#endif
    FTB_client_runtime->event_polling_list = (FTB_list_node_t*)malloc(sizeof(FTB_list_node_t));
    FTB_list_init(FTB_client_runtime->event_polling_list);
    return 0; 
}

int FTB_Finalize()
{
    uint32_t msg_type = FTB_MSG_TYPE_FIN;
    FTB_INFO("FTB_Finalize");
    UTIL_WRITE_SHORT(FTB_client_runtime->fd, &msg_type, sizeof(msg_type));
    close(FTB_client_runtime->fd);
#ifndef FTB_CLIENT_POLLING_ONLY
    if (!FTB_client_runtime->properties.polling_only) {
        pthread_mutex_destroy(&(FTB_client_runtime->lock));
        pthread_cancel(FTB_client_runtime->catch_thread);
        pthread_join(FTB_client_runtime->catch_thread, NULL);
        {
            FTB_list_node_t *temp;
            FTB_list_node_t *node;
            FTB_list_for_each(node, FTB_client_runtime->event_notify_list, temp)
            {
                FTB_client_event_mask_list_t *entry = (FTB_client_event_mask_list_t *)node;
                free(entry);
            }
            free(FTB_client_runtime->event_notify_list);
        }
    }
#endif
    {
        FTB_list_node_t *temp;
        FTB_list_node_t *node;
        FTB_list_for_each(node, FTB_client_runtime->event_polling_list, temp)
        {
            FTB_client_event_mask_list_t *entry = (FTB_client_event_mask_list_t *)node;
            free(entry);
        }
        free(FTB_client_runtime->event_polling_list);
    }
    free(FTB_client_runtime->config);
    free(FTB_client_runtime);
    return 0;
}

int FTB_Reg_throw(FTB_event_t *event)
{
    uint32_t msg_type = FTB_MSG_TYPE_REG_THROW;
    UTIL_ERROR_CHECKING;
    FTB_INFO("FTB_Reg_Throw");
    lock_ifneeded();
    UTIL_WRITE_SHORT(FTB_client_runtime->fd, &msg_type, sizeof(msg_type));
    UTIL_WRITE_SHORT(FTB_client_runtime->fd, event, sizeof(FTB_event_t));
    unlock_ifneeded();
    return 0;
}

int FTB_Reg_throw_id(uint32_t event_id)
{
    FTB_event_t event;
    if (FTB_get_event_by_id(event_id, &event))
    {
        FTB_WARNING("Failed to look up event");
        return 1;
    }
    return FTB_Reg_throw(&event);
}

#ifndef FTB_CLIENT_POLLING_ONLY
int FTB_Reg_catch_notify(FTB_event_mask_t *event_mask, int (*callback)(FTB_event_inst_t *, void*), void *arg)
{
    uint32_t msg_type = FTB_MSG_TYPE_REG_CATCH_NOTIFY;
    UTIL_ERROR_CHECKING;
    FTB_INFO("FTB_Reg_catch_notify");

    /*add to the front of local list, does not care duplicate, anyway the first hit will match*/
    FTB_client_event_mask_list_t *entry = (FTB_client_event_mask_list_t*)malloc(sizeof(FTB_client_event_mask_list_t));
    entry->callback = callback;
    entry->callback_arg = arg;
    memcpy(&(entry->mask),event_mask,sizeof(FTB_event_mask_t));
    FTB_list_add_front(FTB_client_runtime->event_notify_list, (FTB_list_node_t *)entry);

    /*register to server*/
    lock_ifneeded();
    UTIL_WRITE_SHORT(FTB_client_runtime->fd, &msg_type, sizeof(msg_type));
    UTIL_WRITE_SHORT(FTB_client_runtime->fd, event_mask, sizeof(FTB_event_mask_t));
    unlock_ifneeded();
    return 0;
}

int FTB_Reg_catch_notify_single(uint32_t event_id, int (*callback)(FTB_event_inst_t *, void*), void *arg)
{
    FTB_event_mask_t mask;
    FTB_EVENT_CLR_ALL(mask);
    FTB_EVENT_MARK_ALL_SRC_NAMESPACE(mask);
    FTB_EVENT_MARK_ALL_SRC_ID(mask);
    FTB_EVENT_MARK_ALL_SEVERITY(mask);
    FTB_EVENT_SET_EVENT_ID(mask,event_id);
    return FTB_Reg_catch_notify(&mask,callback,arg);
}

int FTB_Reg_catch_notify_all(int (*callback)(FTB_event_inst_t *, void*), void *arg)
{
    FTB_event_mask_t mask;
    FTB_EVENT_CLR_ALL(mask);
    FTB_EVENT_MARK_ALL_SRC_NAMESPACE(mask);
    FTB_EVENT_MARK_ALL_SRC_ID(mask);
    FTB_EVENT_MARK_ALL_SEVERITY(mask);
    FTB_EVENT_MARK_ALL_EVENT_ID(mask);
    return FTB_Reg_catch_notify(&mask,callback,arg);
}

#endif

int FTB_Reg_catch_polling(FTB_event_mask_t *event_mask)
{
    uint32_t msg_type = FTB_MSG_TYPE_REG_CATCH_POLLING;
    UTIL_ERROR_CHECKING;
    FTB_INFO("FTB_Reg_catch_polling");

    /*add to the front of local list, does not care duplicate, anyway the first hit will match*/
    FTB_client_event_mask_list_t *entry = (FTB_client_event_mask_list_t*)malloc(sizeof(FTB_client_event_mask_list_t));
#ifndef FTB_CLIENT_POLLING_ONLY
    entry->callback = NULL;
    entry->callback_arg = NULL;
#endif
    memcpy(&(entry->mask),event_mask,sizeof(FTB_event_mask_t));
    FTB_list_add_front(FTB_client_runtime->event_polling_list, (FTB_list_node_t *)entry);

    /*register to server*/
    lock_ifneeded();
    UTIL_WRITE_SHORT(FTB_client_runtime->fd, &msg_type, sizeof(msg_type));
    UTIL_WRITE_SHORT(FTB_client_runtime->fd, event_mask, sizeof(FTB_event_mask_t));
    unlock_ifneeded();
    return 0;
}

int FTB_Reg_catch_polling_single(uint32_t event_id)
{
    FTB_event_mask_t mask;
    FTB_EVENT_CLR_ALL(mask);
    FTB_EVENT_MARK_ALL_SRC_NAMESPACE(mask);
    FTB_EVENT_MARK_ALL_SRC_ID(mask);
    FTB_EVENT_MARK_ALL_SEVERITY(mask);
    FTB_EVENT_SET_EVENT_ID(mask,event_id);
    return FTB_Reg_catch_polling(&mask);
}

int FTB_Reg_catch_polling_all()
{
    FTB_event_mask_t mask;
    FTB_EVENT_CLR_ALL(mask);
    FTB_EVENT_MARK_ALL_SRC_NAMESPACE(mask);
    FTB_EVENT_MARK_ALL_SRC_ID(mask);
    FTB_EVENT_MARK_ALL_SEVERITY(mask);
    FTB_EVENT_MARK_ALL_EVENT_ID(mask);
    return FTB_Reg_catch_polling(&mask);
}

int FTB_Throw(FTB_event_t *event, char *data, uint32_t data_len)
{
    uint32_t msg_type = FTB_MSG_TYPE_THROW;
    FTB_event_inst_t event_inst;
    UTIL_ERROR_CHECKING;
    event_inst.event_id = event->event_id;
    strncpy(event_inst.name,event->name,FTB_MAX_EVENT_NAME);
    event_inst.severity = event->severity;
    event_inst.src_namespace = FTB_client_runtime->properties.com_namespace;
    event_inst.src_id = FTB_client_runtime->properties.id;
    if (data != NULL) {
        if (data_len > FTB_MAX_EVENT_IMMEDIATE_DATA)
            data_len = FTB_MAX_EVENT_IMMEDIATE_DATA;
        memcpy(event_inst.immediate_data, data, data_len);
        if (data_len <FTB_MAX_EVENT_IMMEDIATE_DATA)
            event_inst.immediate_data[data_len]='\0';
    }
    else {
        memset(event_inst.immediate_data, 0, FTB_MAX_EVENT_IMMEDIATE_DATA);
    }
    lock_ifneeded();
    UTIL_WRITE_SHORT(FTB_client_runtime->fd, &msg_type, sizeof(msg_type));
    UTIL_WRITE_SHORT(FTB_client_runtime->fd, &event_inst, sizeof(FTB_event_inst_t));
    unlock_ifneeded();
    return 0;
}

int FTB_Throw_id(uint32_t event_id, char *data, uint32_t data_len)
{
    FTB_event_t event;
    
    FTB_INFO("FTB_Throw");
    if (FTB_get_event_by_id(event_id, &event))
    {
        FTB_WARNING("Failed to look up event");
        return 1;
    }
    return FTB_Throw(&event, data, data_len);
}

int FTB_Catch(FTB_event_inst_t * event_inst)
{
    uint32_t msg_type = FTB_MSG_TYPE_CATCH;
    int valid = 0;
    int queued_events;
    UTIL_ERROR_CHECKING;
    FTB_INFO("FTB_Catch");
    lock_ifneeded();
    UTIL_WRITE_SHORT(FTB_client_runtime->fd, &msg_type, sizeof(msg_type));
    UTIL_READ_SHORT(FTB_client_runtime->fd, &queued_events, sizeof(int));
    if (queued_events > 0) {
        UTIL_READ_SHORT(FTB_client_runtime->fd, event_inst, sizeof(FTB_event_inst_t));
    }
    unlock_ifneeded();
    if (queued_events == 0)
        return 1;
    /*check the validatiy*/
    {
        FTB_list_node_t *node;
        FTB_list_for_each_readonly(node, FTB_client_runtime->event_polling_list) 
        {
            FTB_client_event_mask_list_t *entry = (FTB_client_event_mask_list_t *)node;
            if (FTB_Util_match_mask(event_inst, &(entry->mask))) {
                valid = 1;
                break;
            }
        }
    }
    if (valid)
        return 0;
    else
        return 1;
}

int FTB_List_events(FTB_event_t *events, int *len)
{
    uint32_t msg_type = FTB_MSG_TYPE_LIST_EVENTS;
    int i;
    int num_events;
    FTB_event_t dummy;
    UTIL_ERROR_CHECKING;
    FTB_INFO("FTB_List_events");
    lock_ifneeded();
    UTIL_WRITE_SHORT(FTB_client_runtime->fd, &msg_type, sizeof(msg_type));
    UTIL_READ_SHORT(FTB_client_runtime->fd, &num_events, sizeof(int));
    if (num_events > *len) {
        FTB_WARNING("More events (%d) than given length (%d)",num_events, *len);
    }
    else {
        *len = num_events;
    }
    for (i=0;i<num_events;i++) {
        if (i<*len)
            UTIL_READ_SHORT(FTB_client_runtime->fd, &(events[i]), sizeof(FTB_event_t));
        else
            UTIL_READ_SHORT(FTB_client_runtime->fd, &(dummy), sizeof(FTB_event_t));
    }
    unlock_ifneeded();
    return 0;
}

