#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <assert.h>
#include <pthread.h>
#include <string.h>
#include <netdb.h>
#include <netinet/tcp.h>
#include <errno.h>

#include "ftb_def.h"
#include "ftb_manager_lib.h"
#include "ftb_network.h"
#include "ftb_util.h"

extern FILE* FTBU_log_file_fp;

typedef struct FTB_connection_entry {
    FTBU_list_node_t *next;
    FTBU_list_node_t *prev;
    pthread_mutex_t lock;
    int err_flag;
    int fd;
    FTB_location_id_t *dst;
    volatile int ref_count;
}FTB_connection_entry_t;

static FTBU_list_node_t *FTBN_connection_table;
static pthread_mutex_t FTBN_conn_table_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t FTBN_recv_lock = PTHREAD_MUTEX_INITIALIZER;
static int FTBN_listen_fd = -1;
static FTBN_addr_sock_t FTBN_my_addr;
static FTB_location_id_t FTBN_my_location_id;
static FTBN_config_info_t FTBN_config_location;
static FTBN_config_sock_t FTBN_config;

static inline void lock_conn_table()
{
//    FTB_INFO("table lock req");
    pthread_mutex_lock(&FTBN_conn_table_lock);
//    FTB_INFO("table lock acq");
}

static inline void unlock_conn_table()
{
//    FTB_INFO("table lock rel");
    pthread_mutex_unlock(&FTBN_conn_table_lock);
}

static inline void lock_conn_entry(FTB_connection_entry_t *conn)
{
//    FTB_INFO("conn lock req %p",conn);
    pthread_mutex_lock(&conn->lock);
//    FTB_INFO("conn lock acq %p",conn);
}

static inline void unlock_conn_entry(FTB_connection_entry_t *conn)
{
//    FTB_INFO("conn lock rel %p",conn);
    pthread_mutex_unlock(&conn->lock);
}

static inline void lock_recv()
{
//    FTB_INFO("recv lock req");
    pthread_mutex_lock(&FTBN_recv_lock);
//    FTB_INFO("recv lock acq");
}

static inline void unlock_recv()
{
//    FTB_INFO("recv lock rel");
    pthread_mutex_unlock(&FTBN_recv_lock);
}

#define UTIL_READ(conn,buf,len)  do { \
    if ((conn)->err_flag == 0) {\
        int temp;\
        int offset = 0;\
        while (offset < (len)) {\
            temp = read((conn)->fd, (char*)(buf)+offset, (len)-offset);   \
            if (temp == 0) { \
                (conn)->err_flag = 1; \
                close((conn)->fd);\
                break; \
            } \
            else if (temp == -1) {\
                if (errno == EINTR || errno == EAGAIN){\
                    continue;\
                }\
                (conn)->err_flag = 1; \
                close((conn)->fd);\
                break;\
            }\
            offset+=temp; \
        } \
    }\
}while(0)

#define UTIL_WRITE(conn,buf,len) do { \
    if ((conn)->err_flag == 0) {\
        int temp;\
        int offset = 0;\
        while (offset < (len)) {\
            temp = write((conn)->fd, (char*)(buf)+offset, (len)-offset);  \
            if (temp == -1) {\
                if (errno == EINTR || errno == EAGAIN){\
                    continue;\
                }\
                (conn)->err_flag = 1; \
                close((conn)->fd);\
                break;\
            }\
            offset+=temp; \
        } \
    }\
}while(0)


static inline FTB_connection_entry_t* util_find_connection_to_location(const FTB_location_id_t *location_id)
{
    FTBU_list_node_t *pos;
    FTBU_list_for_each_readonly(pos, FTBN_connection_table) {
        FTB_connection_entry_t *entry = (FTB_connection_entry_t *)pos;
        if (FTBU_is_equal_location_id(entry->dst, location_id)) {
            unlock_conn_table();
            return entry;
        }
    }
    return NULL;
}

static int util_listen()
{
    struct sockaddr_in sa;
    FTBN_listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (FTBN_listen_fd < 0) {
        return FTB_ERR_NETWORK_GENERAL;
    }
    sa.sin_family = AF_INET;
    sa.sin_port = htons(FTBN_config.agent_port);
    sa.sin_addr.s_addr = INADDR_ANY;
    if (bind(FTBN_listen_fd, (struct sockaddr*) &sa, sizeof(struct sockaddr_in))< 0) {
        FTB_WARNING("bind failed");
        return FTB_ERR_NETWORK_GENERAL;
    }
    if (listen(FTBN_listen_fd, 5) < 0) {
        FTB_WARNING("listen failed");
        return FTB_ERR_NETWORK_GENERAL;
    }

    return FTB_SUCCESS;
}

static int util_connect_to(const FTBN_addr_sock_t *addr, const FTBM_msg_t *reg_msg, FTB_location_id_t *id)
{
    /*Connect to parent at addr, if successful, parent id will be filled*/
    int optval = 1;
    struct hostent *hp;
    struct sockaddr_in sa;
    FTB_connection_entry_t *entry = (FTB_connection_entry_t *)malloc(sizeof(FTB_connection_entry_t));
    entry->err_flag = 0;
    entry->ref_count = 0;
    
    entry->fd = socket(AF_INET, SOCK_STREAM, 0 );
    if (entry->fd < 0) {
        return FTB_ERR_NETWORK_GENERAL;
    }

    hp = gethostbyname(addr->name);
    memset( (void *)&sa, 0, sizeof(sa) );
    memcpy( (void *)&sa.sin_addr, (void *)hp->h_addr, hp->h_length);
    sa.sin_family = AF_INET;
    sa.sin_port = htons(addr->port);
    if (setsockopt(entry->fd, IPPROTO_TCP, TCP_NODELAY, (char *)&optval, sizeof(optval) )) {
        return FTB_ERR_NETWORK_GENERAL;
    }
    if (connect(entry->fd, (struct sockaddr *)&sa, sizeof(sa) ) < 0) {
        return FTB_ERR_NETWORK_NO_ROUTE;
    }
    entry->dst = (FTB_location_id_t*)malloc(sizeof(FTB_location_id_t));
    UTIL_WRITE(entry, &(FTBN_config_location.FTB_system_id), sizeof(uint32_t));
    UTIL_WRITE(entry,&FTBN_my_location_id,sizeof(FTB_location_id_t));
    UTIL_WRITE(entry, reg_msg, sizeof(FTBM_msg_t));
    UTIL_READ(entry, entry->dst, sizeof(FTB_location_id_t));
    if (entry->err_flag) {
        close(entry->fd);
        free(entry);
        return FTB_ERR_NETWORK_GENERAL;
    }
        
    /*since using tree mode, no indirect connection*/
    pthread_mutex_init(&(entry->lock),NULL);
    memcpy(id, entry->dst, sizeof(FTB_location_id_t));
    lock_conn_table();
    FTBU_list_add_front(FTBN_connection_table, (FTBU_list_node_t*)entry);
    unlock_conn_table();
    
    return FTB_SUCCESS;
}


int FTBN_Init(const FTB_location_id_t* my_id, const FTBN_config_info_t *config_info)
{
    int ret;
    FTB_INFO("FTBN_Init In");
    FTBN_connection_table = (FTBU_list_node_t *)malloc(sizeof(FTBU_list_node_t));
    FTBU_list_init(FTBN_connection_table);
    memcpy(&FTBN_my_location_id, my_id, sizeof(FTB_location_id_t));
    memcpy(&FTBN_config_location,config_info,sizeof(FTBN_config_info_t));
    
    ret = FTBN_Bootstrap_init(&FTBN_config_location, &FTBN_config, &FTBN_my_addr);
    if (ret != FTB_SUCCESS)
        return ret;
    FTB_INFO("FTBN_Init Out"); 
    return FTB_SUCCESS;
}

int FTBN_Connect(const FTBM_msg_t *reg_msg, FTB_location_id_t *parent_location_id)
{
    int ret;
    int retry = 0;
    FTBN_addr_sock_t addr;
    
    strcpy(addr.name,"localhost");
    addr.port = FTBN_config.agent_port;
    
    FTB_INFO("FTBN_Connect In");
    if (FTBN_config_location.leaf) {
        FTB_INFO("leaf: try connect to local");
        /*Try to connect the local agent*/
        ret = util_connect_to(&addr, reg_msg, parent_location_id);
        FTB_INFO("FTBN_Connect done");
        if (ret == FTB_ERR_NETWORK_GENERAL) {
            FTB_INFO("FTBN_Connect Out");
            return ret;
        }
        if (ret == FTB_SUCCESS){
            FTB_INFO("FTBN_Connect Out");
            return ret;
        }
        /*Otherwise try to get address from data base server*/
    }

    do {
        ret = FTBN_Bootstrap_get_parent_addr(&addr);
        if (ret == FTB_ERR_NETWORK_NO_ROUTE) {
            continue;
        }
        if (addr.port == 0) {/*It is the root*/
            parent_location_id->pid = 0;
            break;
        }
        ret = util_connect_to(&addr, reg_msg, parent_location_id);
    } while (ret != FTB_SUCCESS && retry++ < 10);
    
    if (ret != FTB_SUCCESS){
        FTB_INFO("FTBN_Connect Out");
        return ret;
    }

    if (!FTBN_config_location.leaf) {
        if (util_listen() != FTB_SUCCESS)
            FTB_ERR_ABORT("cannot listen for new connection");
        /*Register to allow other agent connect in*/
        FTBN_Bootstrap_register_addr();
    }
    FTB_INFO("FTBN_Connect Out");
    
    return FTB_SUCCESS;
}

int FTBN_Finalize()
{
    FTBU_list_node_t *pos;
    FTBU_list_node_t *temp;

    FTB_INFO("FTBN_Finalize In");
    FTBN_Bootstrap_finalize();

    FTBU_list_for_each(pos, FTBN_connection_table, temp) {
        FTB_connection_entry_t *entry = (FTB_connection_entry_t *)pos;
        free(entry->dst);
        free(entry);
    }
    if (FTBN_listen_fd != -1)
        close(FTBN_listen_fd);
    free(FTBN_connection_table);
    FTB_INFO("FTBN_Finalize Out");
    return FTB_SUCCESS;
}

int FTBN_Abort()
{
    FTB_INFO("FTBN_Abort In");
    FTBN_Bootstrap_abort();
    if (FTBN_listen_fd != -1)
        close(FTBN_listen_fd);
    FTB_INFO("FTBN_Abort Out");
    return FTB_SUCCESS;
}

int FTBN_Disconnect_peer(const FTB_location_id_t * peer_location_id)
{
    FTB_connection_entry_t *conn;
    FTB_INFO("FTBN_Disconnect_peer In");
    lock_conn_table();
    conn = util_find_connection_to_location(peer_location_id);
    if (conn == NULL) {
        unlock_conn_table();
        FTB_INFO("FTBN_Disconnect_peer Out");
        return FTB_ERR_INVALID_PARAMETER;
    }

    while (conn->ref_count != 0) {
        unlock_conn_table();
        lock_conn_table();
    }

    free(conn->dst);
    close(conn->fd);
    FTBU_list_remove_entry((FTBU_list_node_t*)conn);
    free(conn);
    unlock_conn_table();
    FTB_INFO("FTBN_Disconnect_peer Out");
    return FTB_SUCCESS;
}

int FTBN_Send_msg(const FTBM_msg_t *msg)
{
    FTB_connection_entry_t *conn;
    FTB_INFO("FTBN_Send_msg In");
    lock_conn_table();
    conn = util_find_connection_to_location(&msg->dst.location_id);
    if (conn == NULL) {
        unlock_conn_table();
        FTB_INFO("FTBN_Send_msg Out");
        return FTB_ERR_NETWORK_NO_ROUTE;
    }
    conn->ref_count++;
    unlock_conn_table();

    lock_conn_entry(conn);
    UTIL_WRITE(conn, msg, sizeof(FTBM_msg_t));
    if (conn->err_flag) {
        unlock_conn_entry(conn);
        lock_conn_table();
        conn->ref_count--;
        if (conn->ref_count == 0) {
            free(conn->dst);
            FTBU_list_remove_entry((FTBU_list_node_t *)conn);
            unlock_conn_table();
            close(conn->fd);
            free(conn);
        }
        else {
            unlock_conn_table();
        }
        FTB_INFO("FTBN_Send_msg Out");
        return FTB_ERR_NETWORK_GENERAL;
    }
    unlock_conn_entry(conn);
    lock_conn_table();
    conn->ref_count--;
    unlock_conn_table();
    FTB_INFO("FTBN_Send_msg Out");
    return FTB_SUCCESS;
}

/*Blocking receive*/
int FTBN_Recv_msg(FTBM_msg_t *msg, FTB_location_id_t *incoming_src, int blocking)
{
    struct timeval timeout_zero;
    fd_set fds;
    FTBU_list_node_t *pos;

    FTB_INFO("FTBN_Recv_msg In");
    if (msg == NULL) {
        FTB_INFO("FTBN_Recv_msg Out");
        return FTB_ERR_NULL_POINTER;
    }

    lock_recv();
    while (1) {
        int max_fd = -1;
        struct timeval *timeout;
        timeout_zero.tv_sec = 0;
        timeout_zero.tv_usec = 0;
        if (blocking) {
            timeout = NULL;
        }
        else {
            timeout = &timeout_zero;
        }
        FD_ZERO(&fds);

        if (FTBN_listen_fd != -1) {
            max_fd = FTBN_listen_fd;
            FD_SET(FTBN_listen_fd, &fds);
        }
        
        lock_conn_table();
        FTBU_list_for_each_readonly(pos, FTBN_connection_table){
            FTB_connection_entry_t *conn = (FTB_connection_entry_t *)pos;
            FD_SET(conn->fd, &fds);
            if (max_fd < conn->fd)
               max_fd = conn->fd;
        }
        unlock_conn_table();

        if (select(max_fd+1, &fds, NULL, NULL, timeout)<0) {
            if (errno == EINTR || errno == EAGAIN) {
                if (blocking) continue;
                else break; 
            }
            unlock_recv();
            FTB_INFO("FTBN_Recv_msg Out");
            return FTB_ERR_NETWORK_GENERAL;
        }

        if (FTBN_listen_fd != -1 && FD_ISSET(FTBN_listen_fd,&fds)) {
            /*New connection*/
            uint32_t system_id;
            FTB_connection_entry_t *entry = (FTB_connection_entry_t *)malloc(sizeof(FTB_connection_entry_t));
            entry->err_flag = 0;
            entry->ref_count = 0;
            entry->fd = accept(FTBN_listen_fd, NULL, NULL);
            if (entry->fd < 0) {
                printf("%d\n",(errno == EINVAL));
                perror("accpet");
                free(entry);
                FTB_ERR_ABORT("accept failed");
            }
            UTIL_READ(entry, &system_id, sizeof(uint32_t));
            if (system_id != FTBN_config_location.FTB_system_id) {
                FTB_WARNING("FTB system id doesn't match");
                close(entry->fd);
                free(entry);
                if (blocking) continue;
                else break;
            }
            pthread_mutex_init(&(entry->lock),NULL);
            entry->dst = (FTB_location_id_t *)malloc(sizeof(FTB_location_id_t));
            UTIL_READ(entry, entry->dst, sizeof(FTB_location_id_t));
            UTIL_READ(entry, msg, sizeof(FTBM_msg_t));
            memcpy(incoming_src, entry->dst, sizeof(FTB_location_id_t));
            
            if (entry->err_flag) {
                close(entry->fd);
                free(entry->dst);
                free(entry);
                if (blocking) continue;
                else break;
            }
            UTIL_WRITE(entry, &FTBN_my_location_id, sizeof(FTB_location_id_t));
                
            lock_conn_table();
            FTBU_list_add_front(FTBN_connection_table, (FTBU_list_node_t*)entry);
            unlock_conn_table();
            unlock_recv();
            FTB_INFO("FTBN_Recv_msg Out");
            return FTB_SUCCESS;
        }
        
        lock_conn_table();
        FTBU_list_for_each_readonly(pos, FTBN_connection_table){
            FTB_connection_entry_t *conn = (FTB_connection_entry_t *)pos;
            if (!FD_ISSET(conn->fd,&fds)) {
                continue;
            }

            lock_conn_entry(conn);
            UTIL_READ(conn, msg, sizeof(FTBM_msg_t));
            if (conn->err_flag) {
                unlock_conn_entry(conn);
                unlock_conn_table();
                free(conn->dst);
                FTBU_list_remove_entry((FTBU_list_node_t *)conn);
                close(conn->fd);
                free(conn);
                unlock_recv();
                FTB_INFO("FTBN_Recv_msg Out");
                return FTB_ERR_NETWORK_GENERAL;
            }
            memcpy(incoming_src, conn->dst, sizeof(FTB_location_id_t));
            unlock_conn_entry(conn);
            unlock_conn_table();
            unlock_recv();
            FTB_INFO("FTBN_Recv_msg Out");
            return FTB_SUCCESS;
        }
        unlock_conn_table();
        
        if (!blocking)
            break;
    }
    unlock_recv();
    FTB_INFO("FTBN_Recv_msg Out");
    return FTBN_NO_MSG;
}
