#include "ftb.h"
#include "ftb_util.h"
#include "ftb_built_in_event_list.h"

using namespace std;

#include <vector>
#include <set>
#include <map>

typedef map<uint32_t, FTB_event_t*> FTB_event_map_t;
typedef vector<FTB_event_mask_t *> FTB_event_mask_list_t;
typedef vector<FTB_event_inst_t *> FTB_event_inst_list_t;

typedef struct FTB_component{
    int fd;
    FTB_component_properties_t properties;
    FTB_event_map_t *throw_event_map;
    FTB_event_mask_list_t *catch_event_notify_list;
    FTB_event_mask_list_t *catch_event_polling_list;
    FTB_event_inst_list_t *event_queue;
}FTB_component_t;

typedef map<uint32_t, FTB_component_t*> FTB_component_map_t;

typedef struct FTB_runtime{
    pthread_mutex_t lock;
    pthread_t listen_thread;
    FTB_config_t *config;
    FTB_component_map_t *component_map;
}FTB_runtime_t;

static FTB_runtime_t *FTB_runtime;

void clean_event_queue(FTB_event_inst_list_t *list)
{
    unsigned int i;
    for (i = 0; i<list->size();i++) {
        FTB_event_inst_t *evt = list->at(i);
        delete evt;
    }
}

void clean_event_mask_queue(FTB_event_mask_list_t *list)
{
    unsigned int i;
    for (i = 0; i<list->size();i++) {
        FTB_event_mask_t *evt = list->at(i);
        delete evt;
    }
}

void clean_event_map(FTB_event_map_t *map)
{
    FTB_event_map_t::iterator iter;
    for (iter=map->begin();iter!=map->end();iter++) {
        delete iter->second;
    }
}

void clean_component(FTB_component_t *com)
{
    FTB_runtime->component_map->erase(com->fd);
    clean_event_mask_queue(com->catch_event_notify_list);
    delete com->catch_event_notify_list;
    clean_event_mask_queue(com->catch_event_polling_list);
    delete com->catch_event_polling_list;
    clean_event_queue(com->event_queue);
    delete com->event_queue;
    clean_event_map(com->throw_event_map);
    delete com->throw_event_map;
    delete com;
}

static int err_flag = 0;

void handle_fd_err(int fd)
{
    FTB_component_map_t::iterator it_com = FTB_runtime->component_map->find(fd);
    FTB_WARNING("socket error %d",fd);
    if (it_com != FTB_runtime->component_map->end()) {
        FTB_component_t *com = it_com->second;
        clean_component(com);
        FTB_runtime->component_map->erase(fd);
    }
}

#define UTIL_READ_SHORT(fd,buf,len)  do { \
    err_flag = 0;\
    if (read(fd, buf, len) != len) {  \
        FTB_WARNING("read error %d\n",errno); \
        handle_fd_err(fd);\
        err_flag = 1; \
    } \
}while(0)

#define UTIL_WRITE_SHORT(fd,buf,len)  do { \
    err_flag = 0;\
    if (write(fd, buf, len) != len) {  \
        FTB_WARNING("write error %d\n",errno); \
        handle_fd_err(fd);\
        err_flag = 1; \
    } \
}while(0)\

int version_match(char *v1, char* v2)
{
    if (strncmp(v1, v2, FTB_MAX_EVENT_VERSION_NAME)==0)
    return 1;
    else
        return 0;
}
    
void *listen_thread(void* arg) 
{
    int listen_fd;
    FTB_INFO("In listen_thread");
    {
        struct sockaddr_in sa;
        listen_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (listen_fd < 0) {
        	FTB_ERR_ABORT("socket failed");
        }
        sa.sin_family = AF_INET;
        sa.sin_port = htons(FTB_runtime->config->port);
        FTB_INFO("Listen to port %d", FTB_runtime->config->port);
        sa.sin_addr.s_addr = INADDR_ANY;
        if (bind(listen_fd, (struct sockaddr*) &sa, sizeof(struct sockaddr_in))< 0) {
            FTB_ERR_ABORT("bind failed");
        }
        if (listen(listen_fd, 5) < 0) {
            FTB_ERR_ABORT("listen failed");
        }
    }

    FTB_INFO("Accepting new client");
    while (1) {
        int temp_fd;
        char version_buf[FTB_MAX_EVENT_VERSION_NAME];
        uint32_t temp_int;
        temp_fd = accept(listen_fd, NULL, NULL);
        if (temp_fd < 0) {
            FTB_ERR_ABORT("accept failed");
        }

        UTIL_READ_SHORT(temp_fd, &temp_int, sizeof(uint32_t));

        if (temp_int != FTB_runtime->config->FTB_id) {
            FTB_WARNING("FTB id doesn't match");
            close(temp_fd);
            continue;
        }

        UTIL_READ_SHORT(temp_fd, version_buf, FTB_MAX_EVENT_VERSION_NAME);
        if (!version_match(version_buf,FTB_EVENT_VERSION)) {
            FTB_WARNING("FTB event version doesn't match");
            close(temp_fd);
            continue;
        }

        {
            FTB_component_t *com = new FTB_component_t();
            UTIL_READ_SHORT(temp_fd, &(com->properties), sizeof(FTB_component_properties_t));

            com->fd = temp_fd;
            com->throw_event_map = new FTB_event_map_t();
            com->catch_event_notify_list = new FTB_event_mask_list_t();
            com->catch_event_polling_list = new FTB_event_mask_list_t();
            com->event_queue = new FTB_event_inst_list_t();

            pthread_mutex_lock(&FTB_runtime->lock);
            FTB_runtime->component_map->insert(pair<int, FTB_component_t*>(com->fd,com));
            pthread_mutex_unlock(&FTB_runtime->lock);

            FTB_INFO("new client %s registered, namespace %d, id %d",
                com->properties.name, com->properties.com_namespace, com->properties.id);
        }
    }
    return NULL;
}

void handle_event(FTB_event_inst_t *evt)
{
    FTB_component_map_t::iterator it_com;
    for (it_com=FTB_runtime->component_map->begin(); it_com
	    !=FTB_runtime->component_map->end(); it_com++) {
	 int delivered = 0; 
        FTB_component_t *reg_com = it_com->second;
        FTB_event_mask_list_t::iterator it_mask;

        /*going over notification queue*/
        for (it_mask=reg_com->catch_event_notify_list->begin();
               it_mask!=reg_com->catch_event_notify_list->end();
               it_mask++) {
            FTB_event_mask_t *mask = *it_mask;
            if (FTB_Util_match_mask(evt, mask)) {
                delivered = 1;
                FTB_INFO("Notifying component %s, namespace %d, id %d about event: id %d, name %s",
                		reg_com->properties.name, reg_com->properties.com_namespace, reg_com->properties.id,
                		evt->event_id, evt->name);
                uint32_t msg_type = FTB_MSG_TYPE_NOTIFY;
                UTIL_WRITE_SHORT(reg_com->fd, &msg_type, sizeof(msg_type));
                UTIL_WRITE_SHORT(reg_com->fd, evt, sizeof(FTB_event_inst_t));
                break;
            }
        }
        if (delivered)
            continue;
        
        /*going over polling queue*/
        for (it_mask=reg_com->catch_event_polling_list->begin();
               it_mask!=reg_com->catch_event_polling_list->end();
               it_mask++) {
            FTB_event_mask_t *mask = *it_mask;
            if (FTB_Util_match_mask(evt, mask)) {
                FTB_event_inst_t *inst_entry = new FTB_event_inst_t();
                FTB_INFO("Adding event queue of component %s, namespace %d, id %d: event: id %d, name %s",
                		reg_com->properties.name, reg_com->properties.com_namespace, reg_com->properties.id,
                		evt->event_id, evt->name);
                if (reg_com->event_queue->size() >= (unsigned int)reg_com->properties.polling_queue_len) {
                    FTB_INFO("Component buffer full, removing the oldest event");
                    delete reg_com->event_queue->front();
                    reg_com->event_queue->erase(reg_com->event_queue->begin());
                }
                memcpy(inst_entry, evt, sizeof(FTB_event_inst_t));
                reg_com->event_queue->push_back(inst_entry);
                break;
            }
        }
    }
}

void list_all_thrown_event(FTB_event_map_t *map)
{
    FTB_component_map_t::iterator it_com;
    for (it_com=FTB_runtime->component_map->begin(); 
           it_com!=FTB_runtime->component_map->end(); it_com++) {
        FTB_component_t *com = it_com->second;
        FTB_event_map_t::iterator it_map;
        for (it_map=com->throw_event_map->begin();
               it_map!=com->throw_event_map->end(); it_map++) {
            FTB_event_t *event =it_map->second;
            map->insert(pair<uint32_t, FTB_event_t*>(event->event_id, event));
        }
    }
}

int main_loop() 
{
    int client_fd_msg;
    FTB_component_map_t::iterator it_com;
    uint32_t temp_int;
    while (1) {
        struct timeval timeout;
        fd_set component_fds;
        fd_set error_fds;
        int max_fd = -1;
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;
        FD_ZERO(&component_fds);
        FD_ZERO(&error_fds);
        pthread_mutex_lock(&FTB_runtime->lock);
        for (it_com = FTB_runtime->component_map->begin(); 
            it_com !=FTB_runtime->component_map->end(); it_com++) {
            FTB_component_t *com = it_com->second;
            FD_SET(com->fd, &component_fds);
            FD_SET(com->fd, &error_fds);
            if (max_fd < com->fd)
               max_fd = com->fd;
        }
		
        pthread_mutex_unlock(&FTB_runtime->lock);

        if (select(max_fd+1, &component_fds, NULL, &error_fds, &timeout)<0) {
            if (errno == EINTR)
                continue;
            FTB_ERR_ABORT("select failed\n");
        }

        for (it_com=FTB_runtime->component_map->begin(); 
         it_com!=FTB_runtime->component_map->end(); it_com++) 
        {
            FTB_component_t *com = it_com->second;
            if (FD_ISSET(com->fd,&error_fds)) {
                FTB_WARNING("Component %s error", com->properties.name);
                clean_component(com);
            }

            if (!FD_ISSET(com->fd,&component_fds))
               continue;

            client_fd_msg = com->fd;

            UTIL_READ_SHORT(client_fd_msg,&temp_int, sizeof(uint32_t));
            if (err_flag)
                continue;
            
            if (temp_int == FTB_MSG_TYPE_REG_THROW) {
                pair<FTB_event_map_t::iterator,bool> ret;
                FTB_INFO("FTB_MSG_TYPE_REG_THROW");
                FTB_event_t *new_event = new FTB_event_t();
                UTIL_READ_SHORT(client_fd_msg, new_event, sizeof(FTB_event_t));
                ret = com->throw_event_map->insert(pair<uint32_t, FTB_event_t *>(new_event->event_id,new_event));
                if (!(ret.second)) {
                    FTB_WARNING("Already registered same event id %d",new_event->event_id);
                    delete new_event;
                }
            }
            else if (temp_int == FTB_MSG_TYPE_REG_CATCH_NOTIFY) {
                FTB_INFO("FTB_MSG_TYPE_REG_CATCH_NOTIFY");
                if (com->properties.polling_only) {
                    FTB_WARNING("Polling-only components trying to register notify events");
                    continue;
                }
                FTB_event_mask_t *event_mask = new FTB_event_mask_t();
                UTIL_READ_SHORT(client_fd_msg, event_mask, sizeof(FTB_event_mask_t));
                /*duplicate insert to vector is fine since anyway the event instance will only be put once*/
                com->catch_event_notify_list->push_back(event_mask);
            } 
            else if (temp_int == FTB_MSG_TYPE_REG_CATCH_POLLING) {
                FTB_INFO("FTB_MSG_TYPE_REG_CATCH_POLLING");
                FTB_event_mask_t *event_mask = new FTB_event_mask_t();
                UTIL_READ_SHORT(client_fd_msg, event_mask, sizeof(FTB_event_mask_t));
                /*duplicate insert to vector is fine since anyway the event instance will only be put once*/
                com->catch_event_polling_list->push_back(event_mask);
            } 
            else if (temp_int == FTB_MSG_TYPE_THROW) {
                FTB_INFO("FTB_MSG_TYPE_THROW");
                FTB_event_inst_t *event_inst = new FTB_event_inst_t();
                UTIL_READ_SHORT(client_fd_msg, event_inst, sizeof(FTB_event_inst_t));
                FTB_INFO("component %s, namespace %d, id %d: throwing event: id %d, name %s",
                		com->properties.name, com->properties.com_namespace, com->properties.id,
                		event_inst->event_id, event_inst->name);
                handle_event(event_inst);
                delete event_inst;
            }
            else if (temp_int == FTB_MSG_TYPE_CATCH) {
                FTB_INFO("FTB_MSG_TYPE_CATCH");
                int queued_events = com->event_queue->size();
                UTIL_WRITE_SHORT(client_fd_msg, &queued_events, sizeof(int));
                if (queued_events >0) {
                    FTB_event_inst_t *event_inst = com->event_queue->front();
                    UTIL_WRITE_SHORT(client_fd_msg, event_inst, sizeof(FTB_event_inst_t));
                    FTB_INFO("component %s, namespace %d, id %d: catching event: id %d, name %s",
                    		com->properties.name, com->properties.com_namespace, com->properties.id,
                    		event_inst->event_id, event_inst->name);
                    com->event_queue->erase(com->event_queue->begin());
                    delete event_inst;
                }
            }
            else if (temp_int == FTB_MSG_TYPE_LIST_EVENTS) {
                FTB_INFO("FTB_MSG_TYPE_LIST_EVENTS");
                FTB_event_map_t *map = new FTB_event_map_t();
                list_all_thrown_event(map);
                int number = map->size();
                FTB_INFO("totally %d events",number);
                UTIL_WRITE_SHORT(client_fd_msg, &number, sizeof(int));
                FTB_event_map_t::iterator it_map;
                for (it_map=map->begin();it_map!=map->end();it_map++) {
                    FTB_event_t *event = it_map->second;
                    UTIL_WRITE_SHORT(client_fd_msg, event, sizeof(FTB_event_t));
                }
                delete map;
            }
            else if (temp_int == FTB_MSG_TYPE_FIN) {
                FTB_INFO("FTB_MSG_TYPE_FIN");
                clean_component(com);
                FTB_runtime->component_map->erase(client_fd_msg);
            } 
            else {
                FTB_ERR_ABORT("Unknown message type");
            }
        }
    }
    return 0;
}

int main(int argc, char *argv[]) {
	int ret;
	FTB_config_t *config = (FTB_config_t *)malloc(sizeof(FTB_config_t));
	FTB_runtime = (FTB_runtime_t *)malloc(sizeof(FTB_runtime_t));

	ret = FTB_Util_setup_configuration(config);
	if (ret)
		return ret;

	pthread_mutex_init(&FTB_runtime->lock, NULL);
	FTB_runtime->component_map = new FTB_component_map_t();
	FTB_runtime->config = config;

	pthread_create(&(FTB_runtime->listen_thread), NULL, listen_thread, NULL);

	main_loop();

	/*Actually it will never reach here*/
	delete FTB_runtime->component_map;
	pthread_mutex_destroy(&FTB_runtime->lock);
	pthread_cancel(FTB_runtime->listen_thread);
	pthread_join(FTB_runtime->listen_thread, NULL);
	free(config);
	free(FTB_runtime);

	return 0;
}

