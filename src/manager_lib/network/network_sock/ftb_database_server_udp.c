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
/*
 * This file contains the code for the bootstrap server
 * All FTB agents and possibly FTB clients contact the bootstrap server
 * in order to connect to the FTB topology
 */

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
#include <pthread.h>
#include <string.h>
#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>
#include <errno.h>

#include "ftb_util.h"
#include "ftb_manager_lib.h"
#include "ftb_network.h"

FILE *FTBU_log_file_fp;

/* An implementation of database server which returns a registered FTBN_addr_sock_t address. */
static int fd = -1;

typedef struct FTBNI_bootstrap_entry {
    FTBN_addr_sock_t addr;
    uint32_t level;
} FTBNI_bootstrap_entry_t;

static FTBU_map_node_t *FTBNI_bootstrap_addr_map;
static int FTBNI_addr_count = 0;
static FTBNI_config_sock_t FTBNI_bootstrap_config;

void handler(int sig)
{
    if (sig == SIGINT) {
        if (fd != -1)
            close(fd);
    }
}

int FTNI_is_equal_addr_sock(const void *lhs_void, const void *rhs_void)
{
    FTBN_addr_sock_t *lhs = (FTBN_addr_sock_t *) lhs_void;
    FTBN_addr_sock_t *rhs = (FTBN_addr_sock_t *) rhs_void;

    if (lhs->port == rhs->port && strncmp(lhs->name, rhs->name, FTB_MAX_HOST_ADDR) == 0)
        return 1;
    else
        return 0;
}

static inline FTBNI_bootstrap_entry_t *FTBNI_util_find_root_entry()
{
  FTBU_map_node_t *iter;
  FTBNI_bootstrap_entry_t *entry = NULL;
  iter = FTBU_map_begin(FTBNI_bootstrap_addr_map);
  while (iter != FTBU_map_end(FTBNI_bootstrap_addr_map)) {
    entry = (FTBNI_bootstrap_entry_t *) FTBU_map_get_data(iter);
    if (entry->level == 1) {
      return entry;
    }
    iter = FTBU_map_next_node(iter);
  }
  
  return NULL;
}

static inline FTBNI_bootstrap_entry_t *FTBNI_util_cross_out_parent_entry(const FTBNI_bootstrap_pkt_t *pkt_req)
{
  FTBU_map_node_t *iter;
  FTBNI_bootstrap_entry_t *entry = NULL;

  iter = FTBU_map_find_key(FTBNI_bootstrap_addr_map, (FTBU_map_key_t) (void *) &pkt_req->parent_addr);
  if (iter != FTBU_map_end(FTBNI_bootstrap_addr_map)) {
    entry = (FTBNI_bootstrap_entry_t *) FTBU_map_get_data(iter);
    entry->level = -1;
  }

  return entry;
}


static inline FTBNI_bootstrap_entry_t *FTBNI_util_find_parent_addr(const FTBNI_bootstrap_pkt_t * pkt_req)
{
    /*This is to achieve some randomness with certain requirements of the addr */
    FTBU_map_node_t *iter;
    FTBNI_bootstrap_entry_t *entry = NULL;
    FTBNI_bootstrap_entry_t *temp;
    int x, i;

    if (FTBNI_addr_count == 0)
        return NULL;

    x = rand() % FTBNI_addr_count;

    iter = FTBU_map_begin(FTBNI_bootstrap_addr_map);

    /* Return the last suitable one in a random range */
    for (i = 0; i <= x; i++) {
        temp = (FTBNI_bootstrap_entry_t *) FTBU_map_get_data(iter);

        /*
         * The level should be less than to the level in pkt_req, the
         *  addr can't be same as pkt_req
         */
        if ((temp->level < pkt_req->level) && (temp->level != -1) &&
            (pkt_req->addr.port != temp->addr.port ||
             strncmp(pkt_req->addr.name, temp->addr.name, FTB_MAX_HOST_ADDR) != 0)) {
            entry = temp;
        }
        iter = FTBU_map_next_node(iter);
    }

    if (entry == NULL) {
        /* Not found in random part, try to find one in the rest */
        while (iter != FTBU_map_end(FTBNI_bootstrap_addr_map)) {
            temp = (FTBNI_bootstrap_entry_t *) FTBU_map_get_data(iter);

            /*
             *  The level should be less than to the level in pkt_req,
             *  the addr can't be same as pkt_req
             */
            if ((temp->level < pkt_req->level) && (temp->level != -1)
                && (pkt_req->addr.port != temp->addr.port ||
		    strncmp(pkt_req->addr.name, temp->addr.name, FTB_MAX_HOST_ADDR) != 0)) {
                entry = temp;
                break;
            }
            iter = FTBU_map_next_node(iter);
        }
    }

    return entry;
}

int main(int argc, char *argv[])
{
    FTBNI_bootstrap_pkt_t pkt, pkt_send;
    struct sockaddr_in server, client;
    int slen;
    int optval = 1;

    /* Throw all the log file data to stderr for now */
    FTBU_log_file_fp = stderr;

    FTBNI_bootstrap_addr_map = FTBU_map_init(FTNI_is_equal_addr_sock);
    FTBNI_addr_count = 0;

    /* Get the bootstrap ip address, port and agent information */
    FTBNI_util_setup_config_sock(&FTBNI_bootstrap_config);

    if ((fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
        FTBU_ERR_ABORT("socket failed");
    }

    /* Set the socket to listen to any incoming requests on the bootstrap port */
    memset((char *) &server, sizeof(server), 0);
    server.sin_family = AF_INET;
    server.sin_port = htons(FTBNI_bootstrap_config.server_port);
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char *) &optval, sizeof(optval))) {
        FTBU_WARNING("setsockopt failed");
        close(fd);
        return FTB_ERR_NETWORK_GENERAL;
    }
    if (bind(fd, (struct sockaddr *) &server, sizeof(struct sockaddr_in)) == -1) {
        close(fd);
        FTBU_ERR_ABORT("bind failed");
    }

    /* Wait for user to hit ^C to terminate the bootstrap server */
    signal(SIGINT, handler);

    while (1) {
        int ret;
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(fd, &fds);

        FTBU_INFO("Bootstrap waiting for message");
        if (select(fd + 1, &fds, NULL, NULL, NULL) < 0) {
            if (errno == EINTR || errno == EAGAIN)
                continue;
            close(fd);
            return FTB_ERR_NETWORK_GENERAL;
        }

        if (!FD_ISSET(fd, &fds))
            continue;

        slen = sizeof(struct sockaddr_in);
        ret = recvfrom(fd, &pkt, sizeof(FTBNI_bootstrap_pkt_t), 0,
                       (struct sockaddr *) &client, (socklen_t *) & slen);
        if (ret != sizeof(FTBNI_bootstrap_pkt_t)) {
            perror("recvfrom");
            FTBU_WARNING("Received packet size different from what is expected");
            continue;
        }

        if (pkt.bootstrap_msg_type == FTBNI_BOOTSTRAP_MSG_TYPE_ADDR_REQ) {
            /*
             * Bootstrap server has received a request to supply the parent name of the
             * agent to the caller
             */
            int ret;
            FTBNI_bootstrap_entry_t *entry = NULL;

            FTBU_INFO
                ("Bootstrap received packet type %d (requesting its parent address) from client address %s from client port %d",
                 pkt.bootstrap_msg_type, inet_ntoa(client.sin_addr), ntohs(client.sin_port));

            memset(&pkt_send, 0, sizeof(FTBNI_bootstrap_pkt_t));
            pkt_send.bootstrap_msg_type = FTBNI_BOOTSTRAP_MSG_TYPE_ADDR_REP;

	    if ( FTBNI_util_cross_out_parent_entry(&pkt) != NULL ) {
	      FTBU_INFO("Client %s is asking the address of a new parent", inet_ntoa(client.sin_addr));
	    }

            entry = FTBNI_util_find_parent_addr(&pkt);

            if (entry != NULL) {
	      memcpy(&pkt_send.parent_addr, &entry->addr, sizeof(FTBN_addr_sock_t));
                pkt_send.level = entry->level;
                FTBU_INFO("Found a parent of name %s, port %d and level %d for client = %s",
                          pkt_send.addr.name, pkt_send.addr.port, pkt_send.level,
                          inet_ntoa(client.sin_addr));
            }
            else {
                FTBU_INFO("Found no parent for client address %s from client port %d",
                          inet_ntoa(client.sin_addr), ntohs(client.sin_port));
                pkt_send.parent_addr.name[0] = '\0';
                pkt_send.parent_addr.port = 0;
                pkt_send.level = 0;
            }

            FTBU_INFO
                ("Bootstrap sending client %s response of type %d containing its parent name %s, port = %d and parent level = %d",
                 inet_ntoa(client.sin_addr), pkt_send.bootstrap_msg_type, pkt_send.parent_addr.name,
                 pkt_send.parent_addr.port, pkt_send.level);
            if (sendto
                (fd, &pkt_send, sizeof(FTBNI_bootstrap_pkt_t), 0, (struct sockaddr *) &client,
                 slen) != sizeof(FTBNI_bootstrap_pkt_t)) {
                close(fd);
                FTBU_ERR_ABORT("Sendto failed %d\n", ret);
            }
        }
        else if (pkt.bootstrap_msg_type == FTBNI_BOOTSTRAP_MSG_TYPE_REG_REQ) {
            FTBU_map_node_t *iter;
            FTBNI_bootstrap_entry_t *entry = NULL;

            FTBU_INFO
                ("Bootstrap received packet type %d (register me as parent) from client address %s from client port %d",
                 pkt.bootstrap_msg_type, inet_ntoa(client.sin_addr), ntohs(client.sin_port));

            memset(&pkt_send, 0, sizeof(FTBNI_bootstrap_pkt_t));

	    FTBU_INFO("This is a request to register %s as an agent on port %d, level %u", pkt.addr.name,
                      pkt.addr.port, pkt.level);

            if (pkt.addr.port == 0) {
                FTBU_WARNING("registering an invalid addr");
                continue;
            }

            pkt_send.bootstrap_msg_type = FTBNI_BOOTSTRAP_MSG_TYPE_REG_REP;

            /*
             * Check if the contacting agent is a true root of the tree.
             * If some other agent is the root; then send a
             * FTBNI_BOOTSTRAP_MSG_TYPE_REG_INVALID message
             */
	    if ( pkt.level == 1 && FTBNI_util_find_root_entry() != NULL ) {
		pkt_send.bootstrap_msg_type = FTBNI_BOOTSTRAP_MSG_TYPE_REG_INVALID;
		FTBU_INFO
		  ("Bootstrap preparing to send client %s a response of type %d indicating that its registration as root is invalid",
		   inet_ntoa(client.sin_addr), pkt_send.bootstrap_msg_type);
            }
	    else {
	      if ( (iter = FTBU_map_find_key(FTBNI_bootstrap_addr_map, (FTBU_map_key_t) (void *) &pkt.addr)) 
		   == FTBU_map_end(FTBNI_bootstrap_addr_map) )  {
		entry = (FTBNI_bootstrap_entry_t *) malloc(sizeof(FTBNI_bootstrap_entry_t));
		memcpy(&entry->addr, &pkt.addr, sizeof(FTBN_addr_sock_t));
		entry->level = pkt.level;
		
		FTBU_INFO
		  ("Inserting the client with address %s as a potential parent for other agents",
		   inet_ntoa(client.sin_addr));
		FTBU_map_insert(FTBNI_bootstrap_addr_map, (FTBU_map_key_t) (void *) &entry->addr,
				(void *) entry);
		FTBNI_addr_count++;
	      }
	      else {
		entry = (FTBNI_bootstrap_entry_t *) FTBU_map_get_data(iter);
		entry->level = pkt.level;
		FTBU_WARNING
		  ("In Request register my address section: registering same addr again, update its level to pkt.level");
	      }
	      FTBU_INFO
		("Bootstrap preparing to send client %s a response of type %d, stating that it is now registered to become a parent of future client",
		 inet_ntoa(client.sin_addr), pkt_send.bootstrap_msg_type);
            }

            FTBU_INFO("Sending client %s a response of type %d", inet_ntoa(client.sin_addr),
                      pkt_send.bootstrap_msg_type);
            if (sendto
                (fd, &pkt_send, sizeof(FTBNI_bootstrap_pkt_t), 0, (struct sockaddr *) &client,
                 slen) != sizeof(FTBNI_bootstrap_pkt_t)) {
                close(fd);
                FTBU_ERR_ABORT("Sendto failed");
            }
        }
        else if (pkt.bootstrap_msg_type == FTBNI_BOOTSTRAP_MSG_TYPE_DEREG_REQ) {
            FTBU_map_node_t *iter;
            FTBU_INFO
                ("Bootstrap received packet type %d (deregister me as parent) from client address %s from client port %d",
                 pkt.bootstrap_msg_type, inet_ntoa(client.sin_addr), ntohs(client.sin_port));

            memset(&pkt_send, 0, sizeof(FTBNI_bootstrap_pkt_t));
            if (pkt.addr.port == 0) {
                FTBU_WARNING("deregistering an invalid addr");
                continue;
            }

            iter = FTBU_map_find_key(FTBNI_bootstrap_addr_map, (FTBU_map_key_t) (void *) &pkt.addr);
            if (iter != FTBU_map_end(FTBNI_bootstrap_addr_map)) {
                FTBNI_bootstrap_entry_t *entry = (FTBNI_bootstrap_entry_t *) FTBU_map_get_data(iter);
                free(entry);
                FTBNI_addr_count--;
                FTBU_map_remove_node(iter);
            }
            else {
                FTBU_WARNING("Bootstrap was contacted for deregistering an non-existing addr");
            }

            pkt_send.bootstrap_msg_type = FTBNI_BOOTSTRAP_MSG_TYPE_DEREG_REP;
            FTBU_INFO
                ("Bootstrap sending client %s a response of type %d indicating it has deregistered it as a parent",
                 inet_ntoa(client.sin_addr), pkt_send.bootstrap_msg_type);
            if (sendto
                (fd, &pkt_send, sizeof(FTBNI_bootstrap_pkt_t), 0, (struct sockaddr *) &client,
                 slen) != sizeof(FTBNI_bootstrap_pkt_t)) {
                close(fd);
                FTBU_ERR_ABORT("Sendto failed");
            }
        }
        else if (pkt.bootstrap_msg_type == FTBNI_BOOTSTRAP_MSG_TYPE_CONN_FAIL) {
            FTBU_map_node_t *iter;
            FTBU_INFO
                ("Bootstrap received packet type %d (connection to parent failed) from client address %s from client port %d",
                 pkt.bootstrap_msg_type, inet_ntoa(client.sin_addr), ntohs(client.sin_port));

	    /* ??? What does this block do?
            if (pkt.addr.port == 0) {
                FTBU_WARNING("Bootstrap was contacted by an invalid address");
                continue;
            }
	    */

	    /*            iter = FTBU_map_find_key(FTBNI_bootstrap_addr_map, (FTBU_map_key_t) (void *) &pkt.addr);*/
	    iter = FTBU_map_find_key(FTBNI_bootstrap_addr_map, (FTBU_map_key_t) (void *) &pkt.parent_addr);
            if (iter != FTBU_map_end(FTBNI_bootstrap_addr_map)) {
                FTBNI_bootstrap_entry_t *entry = (FTBNI_bootstrap_entry_t *) FTBU_map_get_data(iter);
                free(entry);
                FTBNI_addr_count--;
                FTBU_map_remove_node(iter);
            }
            else {
                FTBU_WARNING("In Connection failed; Reporting an non-existing addr");
            }
        }
    }
    close(fd);
    return 0;
}
