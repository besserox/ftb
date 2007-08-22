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
#include <netinet/in.h>
#include <signal.h>
#include <errno.h>

#include "ftb_util.h"
#include "ftb_manager_lib.h"
#include "ftb_network.h"

FILE* FTBU_log_file_fp;

/*An implementation of database server which returns a registered FTBN_addr_sock_t address. */
static int fd = -1;

static FTBU_map_node_t *FTBN_bootstrap_addr_set;
static int FTBN_addr_count = 0;
static FTBN_config_sock_t FTBN_bootstrap_config;

void handler(int sig)
{
    if (sig == SIGINT) {
        if (fd != -1)
            close(fd);
    }
}

int FTBN_is_equal_addr_sock(const void* lhs_void, const void* rhs_void)
{
    FTBN_addr_sock_t *lhs = (FTBN_addr_sock_t *)lhs_void;
    FTBN_addr_sock_t *rhs = (FTBN_addr_sock_t *)rhs_void;
    
    if (lhs->port == rhs->port && strncmp(lhs->name,rhs->name,FTB_MAX_HOST_NAME) == 0)
        return 1;
    else
        return 0;
}

int main(int argc, char* argv[])
{
    FTBN_bootstrap_pkt_t pkt, pkt_send;
    struct sockaddr_in server, client;
    int slen;

    FTBU_log_file_fp = stderr;
    FTBN_bootstrap_addr_set = FTBU_map_init(FTBN_is_equal_addr_sock);
    FTBN_addr_count = 0;
    FTBN_util_setup_config_sock(&FTBN_bootstrap_config);
    
    if ((fd=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP))==-1)
    {
        FTB_ERR_ABORT("socket failed");
    }
    memset((char *) &server, sizeof(server), 0);
    server.sin_family = AF_INET;
    server.sin_port = htons(FTBN_bootstrap_config.server_port);
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(fd, (struct sockaddr*) &server, sizeof(struct sockaddr_in))==-1) {
        FTB_ERR_ABORT("bind failed");
    }
    slen = sizeof(struct sockaddr_in);

    signal(SIGINT,handler);
    while (1)
    {
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(fd, &fds);

	FTB_INFO("Waiting for message");
        if (select(fd+1, &fds, NULL, NULL, NULL)<0) {
            if (errno == EINTR || errno == EAGAIN)
                continue;
            return FTB_ERR_NETWORK_GENERAL;
        }

        if (!FD_ISSET(fd,&fds))
            continue;
        
        if (recvfrom(fd, &pkt, sizeof(FTBN_bootstrap_pkt_t), 0, (struct sockaddr *)&client, (socklen_t *)&slen)!=sizeof(FTBN_bootstrap_pkt_t))
        {
            close(fd);
            FTB_ERR_ABORT("recvfrom failed");
        }
        FTB_INFO("received packet type %d client addr %s:%d",pkt.bootstrap_msg_type, inet_ntoa(client.sin_addr), ntohs(client.sin_port));
        if (pkt.bootstrap_msg_type == FTBN_BOOTSTRAP_MSG_TYPE_ADDR_REQ) {
            int ret;
            memset(&pkt_send, 0, sizeof(FTBN_bootstrap_pkt_t));
            pkt_send.bootstrap_msg_type = FTBN_BOOTSTRAP_MSG_TYPE_ADDR_REP;
            if (FTBN_addr_count) {
                FTBN_addr_sock_t addr;
                FTBU_map_iterator_t iter;

                FTB_INFO("try to find agent on same node");
                addr.port = FTBN_bootstrap_config.agent_port;
                strncpy(addr.name, pkt.addr.name, FTB_MAX_HOST_NAME);
                iter = FTBU_map_find(FTBN_bootstrap_addr_set, (FTBU_map_key_t)(void*)&addr);
                if (iter == FTBU_map_end(FTBN_bootstrap_addr_set)) {
                    int i;
                    int x = rand()%FTBN_addr_count;
                    /*Not found the agent on same node*/
                    FTB_INFO("not find agent on same node");
                    iter = FTBU_map_begin(FTBN_bootstrap_addr_set);
                    for (i=0;i<x;i++) {
                        iter = FTBU_map_next_iterator(iter);
                    }
                }
                memcpy((void*)&pkt_send.addr, FTBU_map_get_data(iter), sizeof(FTBN_addr_sock_t));
            }
            else {
                pkt_send.addr.name[0] = '\0';
                pkt_send.addr.port = 0;
            }
            FTB_INFO("sending reply addr of hostname %s, port %d", pkt_send.addr.name, pkt_send.addr.port);
            if(sendto(fd, &pkt_send, sizeof(FTBN_bootstrap_pkt_t), 0, (struct sockaddr *)&client, slen)!=sizeof(FTBN_bootstrap_pkt_t)) {
                close(fd);
                FTB_ERR_ABORT("sendto failed %d\n",ret);
            }
        }
        else if (pkt.bootstrap_msg_type == FTBN_BOOTSTRAP_MSG_TYPE_REG_REQ) {
            memset(&pkt_send, 0, sizeof(FTBN_bootstrap_pkt_t));
            FTBN_addr_sock_t *addr = (FTBN_addr_sock_t*)malloc(sizeof(FTBN_addr_sock_t));
            memcpy(addr, &pkt.addr, sizeof(FTBN_addr_sock_t));
            FTB_INFO("received request to register %s, port %d",pkt.addr.name,pkt.addr.port);
            if (pkt.addr.port == 0) {
                FTB_WARNING("registering an invalid addr");
                continue;
            }
            if (FTBU_map_insert(FTBN_bootstrap_addr_set, (FTBU_map_key_t)(void*)addr, (void*)addr) == FTBU_EXIST)
            {
                free(addr);
                FTB_WARNING("registering same addr again");
            }
            else {
                FTBN_addr_count++;
            }
            
            pkt_send.bootstrap_msg_type = FTBN_BOOTSTRAP_MSG_TYPE_REG_REP;
            FTB_INFO("sending reg reply");
            if (sendto(fd, &pkt_send, sizeof(FTBN_bootstrap_pkt_t), 0, (struct sockaddr *)&client, slen)!=sizeof(FTBN_bootstrap_pkt_t)) {
                close(fd);
                FTB_ERR_ABORT("sendto failed");
            }
        }
        else if (pkt.bootstrap_msg_type == FTBN_BOOTSTRAP_MSG_TYPE_DEREG_REQ) {
            FTBU_map_iterator_t iter;
            
            memset(&pkt_send, 0, sizeof(FTBN_bootstrap_pkt_t));
            if (pkt.addr.port == 0) {
                FTB_WARNING("deregistering an invalid addr");
                continue;
            }

            iter = FTBU_map_find(FTBN_bootstrap_addr_set, (FTBU_map_key_t)(void*)&pkt.addr);
            if (iter != FTBU_map_end(FTBN_bootstrap_addr_set)) {
                FTBN_addr_sock_t *addr = (FTBN_addr_sock_t *)FTBU_map_get_data(iter);
                free(addr);
                FTBN_addr_count--;
                FTBU_map_remove_iter(iter);
            }
            else {
                FTB_WARNING("deregstering an non-existing addr");
            }
            pkt_send.bootstrap_msg_type = FTBN_BOOTSTRAP_MSG_TYPE_DEREG_REP;
            FTB_INFO("sending dereg reply");
            if (sendto(fd, &pkt_send, sizeof(FTBN_bootstrap_pkt_t), 0, (struct sockaddr *)&client, slen) !=sizeof(FTBN_bootstrap_pkt_t)) {
                close(fd);
                FTB_ERR_ABORT("sendto failed");
            }
        }
    }
    return 0;
}
