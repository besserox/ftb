/***********************************************************************************/
/* FTB:ftb-info */
/* This software is licensed under BSD. See the file FTB/misc/license.BSD for
 * complete details on your rights to copy, modify, and use this software.
 */
/* FTB:ftb-info */
/* FTB:ftb-fillin */
/* FTB_Version: 0.6.1
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
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <string.h>
#include <netdb.h>
#include <netinet/tcp.h>
#include <errno.h>
#include <time.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <fcntl.h>

#include "ftb_def.h"
#include "ftb_client_lib_defs.h"
#include "ftb_manager_lib.h"
#include "ftb_network.h"
#include "ftb_util.h"

extern FILE *FTBU_log_file_fp;

typedef struct FTB_connection_entry {
    pthread_mutex_t lock;
    int err_flag;
    int fd;
    FTB_location_id_t *dst;
    volatile int ref_count;
} FTB_connection_entry_t;

static FTBU_list_node_t *FTBNI_connection_table;
static pthread_mutex_t FTBNI_conn_table_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t FTBNI_recv_lock = PTHREAD_MUTEX_INITIALIZER;
static int FTBNI_listen_fd = -1;
static FTBN_addr_sock_t FTBN_my_addr;
static FTBN_addr_sock_t FTBN_parent_addr;
static FTB_location_id_t FTBN_my_location_id;
static FTBN_config_info_t FTBN_config_location;
static FTBNI_config_sock_t FTBN_config;
static uint16_t FTBN_my_level = 0 - 1;

static inline void FTBNI_lock_conn_table()
{
    pthread_mutex_lock(&FTBNI_conn_table_lock);
}

static inline void FTBNI_unlock_conn_table()
{
    pthread_mutex_unlock(&FTBNI_conn_table_lock);
}

static inline void FTBNI_lock_conn_entry(FTB_connection_entry_t * conn)
{
    pthread_mutex_lock(&conn->lock);
}

static inline void FTBNI_unlock_conn_entry(FTB_connection_entry_t * conn)
{
    pthread_mutex_unlock(&conn->lock);
}

static inline void FTBNI_lock_recv()
{
    pthread_mutex_lock(&FTBNI_recv_lock);
}

static inline void FTBNI_unlock_recv()
{
    pthread_mutex_unlock(&FTBNI_recv_lock);
}

#define FTBNI_UTIL_READ(conn,buf,len)  do { \
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
} while (0)

#define FTBNI_UTIL_WRITE(conn,buf,len) do { \
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
} while (0)


static inline FTBU_list_node_t *FTBNI_util_find_connection_to_location(const FTB_location_id_t *
                                                                             location_id)
{
    FTBU_list_node_t *pos;

    FTBU_list_for_each_readonly(pos, FTBNI_connection_table) {
		FTBU_list_node_t *node = pos;
        FTB_connection_entry_t *entry = (FTB_connection_entry_t *) node->data;
        if (FTBU_is_equal_location_id(entry->dst, location_id)) {
            FTBNI_unlock_conn_table();  /* Is this needed ? */
            return pos;
        }
    }
    return NULL;
}

static int FTBNI_util_listen()
{
    struct sockaddr_in sa;
    int optval = 1;
    FTBNI_listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (FTBNI_listen_fd < 0) {
        return FTB_ERR_NETWORK_GENERAL;
    }
    sa.sin_family = AF_INET;
    sa.sin_port = htons(FTBN_config.agent_port);
    sa.sin_addr.s_addr = INADDR_ANY;
    if (setsockopt(FTBNI_listen_fd, SOL_SOCKET, SO_REUSEADDR, (char *) &optval, sizeof(optval))) {
        FTB_WARNING("setsockopt failed");
        return FTB_ERR_NETWORK_GENERAL;
    }
    if (bind(FTBNI_listen_fd, (struct sockaddr *) &sa, sizeof(struct sockaddr_in)) < 0) {
        FTB_WARNING("bind failed");
        return FTB_ERR_NETWORK_GENERAL;
    }
    if (listen(FTBNI_listen_fd, 5) < 0) {
        FTB_WARNING("listen failed");
        return FTB_ERR_NETWORK_GENERAL;
    }

    return FTB_SUCCESS;
}

static int FTBNI_util_connect_to(const FTBN_addr_sock_t * addr, const FTBM_msg_t * reg_msg,
                                 FTB_location_id_t * id)
{
    /*Connect to parent at addr, if successful, parent id will be filled */
    int optval = 1;
    struct hostent *hp = NULL;
    struct sockaddr_in sa;
	FTBU_list_node_t *node = (FTBU_list_node_t *) malloc(sizeof(FTBU_list_node_t));
    FTB_connection_entry_t *entry = (FTB_connection_entry_t *) malloc(sizeof(FTB_connection_entry_t));
    entry->err_flag = 0;
    entry->ref_count = 0;

    entry->fd = socket(AF_INET, SOCK_STREAM, 0);
    if (entry->fd < 0) {
        return FTB_ERR_NETWORK_GENERAL;
    }

    hp = FTBNI_gethostbyname(addr->name);
    if (hp == NULL) {
        FTB_WARNING("cannot find host %s", addr->name);
        close(entry->fd);
        return FTB_ERR_NETWORK_NO_ROUTE;
    }
    memset((void *) &sa, 0, sizeof(sa));
    memcpy((void *) &sa.sin_addr, (void *) hp->h_addr, hp->h_length);
    sa.sin_family = AF_INET;
    sa.sin_port = htons(addr->port);
    if (setsockopt(entry->fd, IPPROTO_TCP, TCP_NODELAY, (char *) &optval, sizeof(optval))) {
	close(entry->fd);
        return FTB_ERR_NETWORK_GENERAL;
    }
    if (connect(entry->fd, (struct sockaddr *) &sa, sizeof(sa)) < 0) {
	close(entry->fd);
        return FTB_ERR_NETWORK_NO_ROUTE;
    }
#ifdef FD_CLOEXEC
    else {
        int rc = fcntl(entry->fd, F_GETFD);
        if ((rc < 0) || (fcntl(entry->fd, F_SETFD, rc | FD_CLOEXEC) < 0)) {
            FTB_WARNING("Failed to set FD_CLOEXEC");
            /* non-fatal */
        }
    }
#endif
    entry->dst = (FTB_location_id_t *) malloc(sizeof(FTB_location_id_t));
    FTBNI_UTIL_WRITE(entry, &(FTBN_config_location.FTB_system_id), sizeof(uint32_t));
    FTBNI_UTIL_WRITE(entry, &FTBN_my_location_id, sizeof(FTB_location_id_t));
    FTBNI_UTIL_WRITE(entry, reg_msg, sizeof(FTBM_msg_t));
    FTBNI_UTIL_READ(entry, entry->dst, sizeof(FTB_location_id_t));
    if (entry->err_flag) {
        close(entry->fd);
        free(entry);
        return FTB_ERR_NETWORK_GENERAL;
    }

    /*since using tree mode, no indirect connection */
    pthread_mutex_init(&(entry->lock), NULL);
    memcpy(id, entry->dst, sizeof(FTB_location_id_t));
	node->data = (void *)entry;
    FTBNI_lock_conn_table();
    FTBU_list_add_front(FTBNI_connection_table, node);
    FTBNI_unlock_conn_table();

    return FTB_SUCCESS;
}


int FTBN_Init(const FTB_location_id_t * my_id, const FTBN_config_info_t * config_info)
{
    int ret;
    FTB_INFO("FTBN_Init In");
    FTBNI_connection_table = (FTBU_list_node_t *) malloc(sizeof(FTBU_list_node_t));
    FTBU_list_init(FTBNI_connection_table);
    memcpy(&FTBN_my_location_id, my_id, sizeof(FTB_location_id_t));
    memcpy(&FTBN_config_location, config_info, sizeof(FTBN_config_info_t));

    ret = FTBNI_Bootstrap_init(&FTBN_config_location, &FTBN_config, &FTBN_my_addr);
    if (ret != FTB_SUCCESS)
        return ret;
    FTB_INFO("FTBN_Init Out");
    return FTB_SUCCESS;
}

int FTBN_Connect(const FTBM_msg_t * reg_msg, FTB_location_id_t * parent_location_id)
{
    int ret;
    int retry = 0;
    uint16_t parent_level;
    FTBN_addr_sock_t addr;
    struct timespec delay;

    delay.tv_sec = 0;
    delay.tv_nsec = FTBN_CONNECT_BACKOFF_INIT_TIMEOUT * 1E6;

    strcpy(addr.name, "localhost");
    addr.port = FTBN_config.agent_port;

    FTB_INFO("FTBN_Connect In");
    if (FTBN_config_location.leaf) {
        FTB_INFO("leaf: try connect to local");

        /*Try to connect the local agent */
        ret = FTBNI_util_connect_to(&addr, reg_msg, parent_location_id);
        FTB_INFO("FTBN_Connect done");
        if (ret == FTB_ERR_NETWORK_GENERAL) {
            FTB_INFO("FTBN_Connect Out");
            return ret;
        }
        if (ret == FTB_SUCCESS) {
            FTB_INFO("FTBN_Connect Out");
            return ret;
        }
        /*Otherwise try to get address from data base server */
        FTBN_parent_addr.port = 0;
    }
	else { /* For Agents, open listening port */ 
		if (FTBNI_listen_fd == -1) {
			if (FTBNI_util_listen() != FTB_SUCCESS) {
				FTB_ERR_ABORT("Agent cannot listen for new connection");
			}
		}
	}

    do {
        if (retry > 0) {
            nanosleep(&delay, NULL);
            delay.tv_sec *= FTBN_CONNECT_BACKOFF_RATIO;
            delay.tv_nsec *= FTBN_CONNECT_BACKOFF_RATIO;
            if (delay.tv_nsec > 1E9) {
                delay.tv_sec += delay.tv_nsec / 1E9;
                delay.tv_nsec = delay.tv_nsec - delay.tv_sec * 1E9;
            }
        }

        /*Pass current parent_addr to this function in the case of reconnecting */
        ret = FTBNI_Bootstrap_get_parent_addr(FTBN_my_level, &FTBN_parent_addr, &parent_level);
        if (ret == FTB_ERR_NETWORK_NO_ROUTE) {
            FTB_WARNING("Failed to contact database server. Return code = %d", ret);
            FTBN_parent_addr.port = 0;
            if (retry < FTBN_CONNECT_RETRY_COUNT) {
	 	        FTB_INFO("Trying to contact database server again retry number = %d", retry);	
                continue;
            }
            else {
				close(FTBNI_listen_fd);
				FTB_ERR_ABORT("Failed to contact database server..Exiting\n");
	        }
        }

        if (FTBN_parent_addr.port == 0) {       /*It is the root */
        	parent_location_id->pid = 0;
			FTBN_my_level = parent_level + 1;
			ret = FTBNI_Bootstrap_register_addr(FTBN_my_level);
			if (ret != FTB_SUCCESS) {
				/* Reset FTBN_my_level to initial value and get new parent from database server*/
				FTBN_my_level = 0-1;
				retry = 0;
				FTB_INFO("Root registration failed, Retrying ...");
				continue;
			}
        }
		else {
			/* Non-root agent or a component tried to connect to its parent */
			ret = FTBNI_util_connect_to(&FTBN_parent_addr, reg_msg, parent_location_id);
        	if (ret == FTB_ERR_NETWORK_NO_ROUTE) {
            	FTBNI_Bootstrap_report_conn_failure(&FTBN_parent_addr);
            	FTBN_parent_addr.port = 2;
				retry = 0;
        	}
			else if (ret == FTB_SUCCESS) {
				if (!FTBN_config_location.leaf) {
					FTBN_my_level = parent_level + 1;
					ret = FTBNI_Bootstrap_register_addr(FTBN_my_level);
					if (ret != FTB_SUCCESS) {
						FTB_WARNING("Agent could not register itself as parent. Ignoring");
						FTB_INFO("FTBN_Connect Out");
						return FTB_SUCCESS;
					}
				}
			}
		}
    } while (ret != FTB_SUCCESS && retry++ < FTBN_CONNECT_RETRY_COUNT);

    if (ret != FTB_SUCCESS) {
		close(FTBNI_listen_fd);
        FTB_INFO("FTBN_Connect Out");
        return ret;
    }

	FTB_INFO("FTBN_Connect Out");
    return FTB_SUCCESS;
}

int FTBN_Finalize()
{
    FTBU_list_node_t *pos;
    FTBU_list_node_t *temp;

    FTB_INFO("FTBN_Finalize In");
    FTBNI_Bootstrap_finalize();

    FTBU_list_for_each(pos, FTBNI_connection_table, temp) {
		FTBU_list_node_t *node = pos;
		FTB_connection_entry_t *entry = (FTB_connection_entry_t *)node->data;
        close(entry->fd);
        free(entry->dst);
        free(entry);
		free(node);
    }
    if (FTBNI_listen_fd != -1)
        close(FTBNI_listen_fd);
    free(FTBNI_connection_table);
    FTB_INFO("FTBN_Finalize Out");
    return FTB_SUCCESS;
}

int FTBN_Abort()
{
    FTB_INFO("FTBN_Abort In");
    FTBNI_Bootstrap_abort();
    if (FTBNI_listen_fd != -1)
        close(FTBNI_listen_fd);
    FTB_INFO("FTBN_Abort Out");
    return FTB_SUCCESS;
}

int FTBN_Disconnect_peer(const FTB_location_id_t * peer_location_id)
{
	FTBU_list_node_t *node;
    FTB_connection_entry_t *conn;
    FTB_INFO("FTBN_Disconnect_peer In");
    FTBNI_lock_conn_table();
    node = FTBNI_util_find_connection_to_location(peer_location_id);
    if (node == NULL) {
        FTBNI_unlock_conn_table();
        FTB_INFO("FTBN_Disconnect_peer Out");
        return FTB_ERR_INVALID_PARAMETER;
    }
	conn = (FTB_connection_entry_t *)node->data;

    while (conn->ref_count != 0) {
        FTBNI_unlock_conn_table();
        FTBNI_lock_conn_table();
    }

    free(conn->dst);
    close(conn->fd);
    free(conn);
    FTBU_list_remove_node(node);
	free(node);
    FTBNI_unlock_conn_table();
    FTB_INFO("FTBN_Disconnect_peer Out");
    return FTB_SUCCESS;
}

int FTBN_Send_msg(const FTBM_msg_t * msg)
{
	FTBU_list_node_t *node;
    FTB_connection_entry_t *conn;

    FTB_INFO("FTBN_Send_msg In");
    FTBNI_lock_conn_table();
    node = FTBNI_util_find_connection_to_location(&msg->dst.location_id);
    if (node == NULL) {
        FTBNI_unlock_conn_table();
        FTB_INFO("FTBN_Send_msg Out");
        return FTB_ERR_NETWORK_NO_ROUTE;
    }
	conn = (FTB_connection_entry_t *) node->data;
    conn->ref_count++;
    FTBNI_unlock_conn_table();

    FTBNI_lock_conn_entry(conn);
    FTBNI_UTIL_WRITE(conn, msg, sizeof(FTBM_msg_t));
    if (conn->err_flag) {
        FTBNI_unlock_conn_entry(conn);
        FTBNI_lock_conn_table();
        conn->ref_count--;
        if (conn->ref_count == 0) {
            free(conn->dst);
            FTBU_list_remove_node(node);
            FTBNI_unlock_conn_table();
            close(conn->fd);
            free(conn);
			free(node);
        }
        else {
            FTBNI_unlock_conn_table();
        }
        FTB_INFO("FTBN_Send_msg Out");
        return FTB_ERR_NETWORK_GENERAL;
    }
    FTBNI_unlock_conn_entry(conn);
    FTBNI_lock_conn_table();
    conn->ref_count--;
    FTBNI_unlock_conn_table();
    FTB_INFO("FTBN_Send_msg Out");
    return FTB_SUCCESS;
}

/*Blocking receive*/
int FTBN_Recv_msg(FTBM_msg_t * msg, FTB_location_id_t * incoming_src, int blocking)
{
    struct timeval timeout_zero;
    fd_set fds;
    FTBU_list_node_t *pos;

    FTB_INFO("FTBN_Recv_msg In");
    if (msg == NULL) {
        FTB_INFO("FTBN_Recv_msg Out");
        return FTB_ERR_NULL_POINTER;
    }

    FTBNI_lock_recv();
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

        if (FTBNI_listen_fd != -1) {
            max_fd = FTBNI_listen_fd;
            FD_SET(FTBNI_listen_fd, &fds);
        }

        FTBNI_lock_conn_table();
        FTBU_list_for_each_readonly(pos, FTBNI_connection_table) {
			FTBU_list_node_t *node = pos;
            FTB_connection_entry_t *conn = (FTB_connection_entry_t *) node->data;
            FD_SET(conn->fd, &fds);
            if (max_fd < conn->fd)
                max_fd = conn->fd;
        }
        FTBNI_unlock_conn_table();

        if (select(max_fd + 1, &fds, NULL, NULL, timeout) < 0) {
            if (errno == EINTR || errno == EAGAIN) {
                if (blocking)
                    continue;
                else
                    break;
            }
            FTBNI_unlock_recv();
            FTB_INFO("FTBN_Recv_msg Out");
            return FTB_ERR_NETWORK_GENERAL;
        }

        if (FTBNI_listen_fd != -1 && FD_ISSET(FTBNI_listen_fd, &fds)) {
            /*New connection */
            uint32_t system_id;
            FTB_connection_entry_t *entry =
                (FTB_connection_entry_t *) malloc(sizeof(FTB_connection_entry_t));
            entry->err_flag = 0;
            entry->ref_count = 0;
            entry->fd = accept(FTBNI_listen_fd, NULL, NULL);
            if (entry->fd < 0) {
                perror("accept");
                free(entry);
                FTB_ERR_ABORT("accept failed");
            }
            FTBNI_UTIL_READ(entry, &system_id, sizeof(uint32_t));
            if (system_id != FTBN_config_location.FTB_system_id) {
                FTB_WARNING("FTB system id doesn't match");
                close(entry->fd);
                free(entry);
                if (blocking)
                    continue;
                else
                    break;
            }
            pthread_mutex_init(&(entry->lock), NULL);
            entry->dst = (FTB_location_id_t *) malloc(sizeof(FTB_location_id_t));
            FTBNI_UTIL_READ(entry, entry->dst, sizeof(FTB_location_id_t));
            FTBNI_UTIL_READ(entry, msg, sizeof(FTBM_msg_t));
            memcpy(incoming_src, entry->dst, sizeof(FTB_location_id_t));

            if (entry->err_flag) {
                close(entry->fd);
                free(entry->dst);
                free(entry);
                if (blocking)
                    continue;
                else
                    break;
            }
            FTBNI_UTIL_WRITE(entry, &FTBN_my_location_id, sizeof(FTB_location_id_t));

			FTBU_list_node_t *node = (FTBU_list_node_t *)malloc(sizeof(FTBU_list_node_t));
			node->data = (void *) entry;
            FTBNI_lock_conn_table();
            FTBU_list_add_front(FTBNI_connection_table, node);
            FTBNI_unlock_conn_table();
            FTBNI_unlock_recv();
            FTB_INFO("FTBN_Recv_msg Out");
            return FTB_SUCCESS;
        }

        FTBNI_lock_conn_table();
        FTBU_list_for_each_readonly(pos, FTBNI_connection_table) {
			FTBU_list_node_t *node = pos;
            FTB_connection_entry_t *conn = (FTB_connection_entry_t *) node->data;
            if (!FD_ISSET(conn->fd, &fds)) {
                continue;
            }

            FTBNI_lock_conn_entry(conn);
            FTBNI_UTIL_READ(conn, msg, sizeof(FTBM_msg_t));
            if (conn->err_flag) {
                FTBNI_unlock_conn_entry(conn);
                FTBNI_unlock_conn_table();
                free(conn->dst);
                FTBU_list_remove_node(node);
                close(conn->fd);
                free(conn);
				free(node);
                FTBNI_unlock_recv();
                FTB_INFO("FTBN_Recv_msg Out");
                return FTB_ERR_NETWORK_GENERAL;
            }
            memcpy(incoming_src, conn->dst, sizeof(FTB_location_id_t));
            FTBNI_unlock_conn_entry(conn);
            FTBNI_unlock_conn_table();
            FTBNI_unlock_recv();
            FTB_INFO("FTBN_Recv_msg Out");
            return FTB_SUCCESS;
        }
        FTBNI_unlock_conn_table();

        if (!blocking)
            break;
    }
    FTBNI_unlock_recv();
    FTB_INFO("FTBN_Recv_msg Out");
    return FTBN_NO_MSG;
}


/* Grab messages from the network and put it in the message queue 
 * - especially for the agent to read. This function will eventually 
 * have to be replaced so that we dont have any special functions 
 * for the agent
 */
int FTBN_Grab_messages(FTBM_msg_node_t **msg_head, FTBM_msg_node_t **msg_tail)
{
	fd_set fds;
    FTBU_list_node_t *pos;

    while ( 1 ) {

		int max_fd = -1;
		FD_ZERO(&fds);
		if (FTBNI_listen_fd != -1) {
			max_fd = FTBNI_listen_fd;
			FD_SET(FTBNI_listen_fd, &fds);
		}

		FTBNI_lock_conn_table();
		FTBU_list_for_each_readonly(pos, FTBNI_connection_table) {
			FTBU_list_node_t *node = pos;
			FTB_connection_entry_t *conn = (FTB_connection_entry_t *) node->data;
			FD_SET(conn->fd, &fds);
			if (max_fd < conn->fd)
				max_fd = conn->fd;
		}
		FTBNI_unlock_conn_table();

		if (select(max_fd + 1, &fds, NULL, NULL, NULL) < 0) {
			if (errno == EINTR || errno == EAGAIN) {
				continue;
			}
			return FTB_ERR_NETWORK_GENERAL;      
		}

		if (FTBNI_listen_fd != -1 && FD_ISSET(FTBNI_listen_fd, &fds)) {
		/*New connection */
			uint32_t system_id;
			FTB_connection_entry_t *entry =
				(FTB_connection_entry_t *) malloc(sizeof(FTB_connection_entry_t));

			entry->err_flag = 0;
			entry->ref_count = 0;
			entry->fd = accept(FTBNI_listen_fd, NULL, NULL);
			if (entry->fd < 0) {
				perror("accept");
				free(entry);
				FTB_ERR_ABORT("accept failed");
			}
      
			FTBNI_UTIL_READ(entry, &system_id, sizeof(uint32_t));
			if (system_id != FTBN_config_location.FTB_system_id) {
				FTB_WARNING("FTB system id doesn't match");
				close(entry->fd);
				free(entry);
			continue;
			}

			pthread_mutex_init(&(entry->lock), NULL);
			entry->dst = (FTB_location_id_t *) malloc(sizeof(FTB_location_id_t));
			FTBM_msg_t *msg = (FTBM_msg_t *) malloc(sizeof(FTBM_msg_t));

			FTBNI_UTIL_READ(entry, entry->dst, sizeof(FTB_location_id_t));
			FTBNI_UTIL_READ(entry, msg, sizeof(FTBM_msg_t));

			if (entry->err_flag) {
				close(entry->fd);
				free(entry->dst);
				free(entry);
				free(msg);
			}
			else {
				FTBNI_UTIL_WRITE(entry, &FTBN_my_location_id, sizeof(FTB_location_id_t));
				FTBU_list_node_t *node = (FTBU_list_node_t *)malloc(sizeof(FTBU_list_node_t));
				node->data = (void *) entry;

				FTBNI_lock_conn_table();
				FTBU_list_add_front(FTBNI_connection_table, node);
				FTBNI_unlock_conn_table();

				FTBM_msg_node_t *msg_node = (FTBM_msg_node_t *)malloc(sizeof(FTBM_msg_node_t));
				msg_node->msg = msg;
				msg_node->incoming_src = (FTB_location_id_t *)malloc(sizeof(FTB_location_id_t));
				memcpy(msg_node->incoming_src, entry->dst, sizeof(FTB_location_id_t));

				if( *msg_head == NULL ) {
					*msg_head = *msg_tail = msg_node;
				}
				else {
					(*msg_tail)->next = msg_node;
				}
				msg_node->next = NULL;
				*msg_tail = msg_node;
			}
		}

		FTBNI_lock_conn_table();      

		FTBU_list_for_each_readonly(pos, FTBNI_connection_table) {
			FTBU_list_node_t *node = pos;
			FTB_connection_entry_t *conn = (FTB_connection_entry_t *) node->data;

			if ( FD_ISSET(conn->fd, &fds) ) {
				FTBM_msg_t *msg = (FTBM_msg_t *) malloc(sizeof(FTBM_msg_t));
				FTBNI_lock_conn_entry(conn);
				FTBNI_UTIL_READ(conn, msg, sizeof(FTBM_msg_t));
				if (conn->err_flag) {
					FTBNI_unlock_conn_entry(conn);
					free(conn->dst);
					FTBU_list_remove_node(node);
					close(conn->fd);
					free(conn);
					free(node);
					free(msg);
					FTBNI_unlock_conn_table();
					return FTB_ERR_NETWORK_GENERAL;      
				}
				else {
					FTBM_msg_node_t *msg_node = (FTBM_msg_node_t *)malloc(sizeof(FTBM_msg_node_t));
					msg_node->msg = msg;
					msg_node->incoming_src = (FTB_location_id_t *)malloc(sizeof(FTB_location_id_t));
					memcpy(msg_node->incoming_src, conn->dst, sizeof(FTB_location_id_t));	    
					FTBNI_unlock_conn_entry(conn);
					if( *msg_head == NULL ) {
						*msg_head = *msg_tail = msg_node;
					}
					else {
						(*msg_tail)->next = msg_node;
					}
					msg_node->next = NULL;
					*msg_tail = msg_node;
				}
			}
		}	
		FTBNI_unlock_conn_table();
		return FTB_SUCCESS;
	}	
}

int FTBN_Get_my_network_address(char *ipaddr)
{
    int i;
    int s = socket(PF_INET, SOCK_STREAM, 0);

    for (i = 1;; i++) {
        struct ifreq ifr;
        struct sockaddr_in *sin = (struct sockaddr_in *) &ifr.ifr_addr;

        ifr.ifr_ifindex = i;

        /*
         * FIXME: Remove references to ioctl since it is not
         * portable! Figure out a different solution to get the ipaddress
         * of the network
         */
        if (ioctl(s, SIOCGIFNAME, &ifr) < 0)
            break;

        /* now ifr.ifr_name is set */
        if (ioctl(s, SIOCGIFADDR, &ifr) < 0)
            continue;

        sprintf(ipaddr, "%s", inet_ntoa(sin->sin_addr));
        if (strcmp(ipaddr, "127.0.0.1") != 0) {
            close(s);
            return 1;
        }
    }
    close(s);
    return -1;
}
