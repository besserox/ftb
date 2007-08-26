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
#include <signal.h>
#include <errno.h>

#include "ftb_util.h"
#include "ftb_manager_lib.h"
#include "ftb_network.h"

extern FILE* FTBU_log_file_fp;

static FTBN_addr_sock_t FTBN_bootstrap_my_addr;
static FTBN_config_info_t FTBN_bootstrap_config_location;
static FTBN_config_sock_t FTBN_bootstrap_config;

static int util_send_bootstrap_msg(const FTBN_bootstrap_pkt_t *pkt_send)
{
    struct sockaddr_in server;
    struct hostent *hp;
    int fd;
    if ((fd=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP))==-1)
    {
        FTB_ERR_ABORT("socket failed");
    }
    hp = gethostbyname(FTBN_bootstrap_config.server_name);
    if (hp == NULL) {
        FTB_ERR_ABORT("cannot find database server %s",FTBN_bootstrap_config.server_name);
    }
    memset( (void *)&server, 0, sizeof(server) );
    memcpy( (void *)&server.sin_addr, (void *)hp->h_addr, hp->h_length);
    server.sin_family = AF_INET;
    server.sin_port = htons(FTBN_bootstrap_config.server_port);
    FTB_INFO("sending pkt type %d",pkt_send->bootstrap_msg_type);
    if (sendto(fd, pkt_send, sizeof(FTBN_bootstrap_pkt_t), 0, (struct sockaddr *)&server, sizeof(struct sockaddr_in))!=sizeof(FTBN_bootstrap_pkt_t)) {
        close(fd);
        FTB_ERR_ABORT("sendto failed");
    }
    return FTB_SUCCESS;
}

static int util_exchange_bootstrap_msg(const FTBN_bootstrap_pkt_t *pkt_send, FTBN_bootstrap_pkt_t* pkt_recv)
{
    struct sockaddr_in server;
    struct hostent *hp;
    int fd;
    unsigned int timeout_milli = FTBN_BOOTSTRAP_BACKOFF_INIT_TIMEOUT;
    int i;
    int flag;
    
    if ((fd=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP))==-1)
    {
        FTB_ERR_ABORT("socket failed");
    }
    hp = gethostbyname(FTBN_bootstrap_config.server_name);
    if (hp == NULL) {
        FTB_ERR_ABORT("cannot find database server %s",FTBN_bootstrap_config.server_name);
    }
    memset( (void *)&server, 0, sizeof(server) );
    memcpy( (void *)&server.sin_addr, (void *)hp->h_addr, hp->h_length);
    server.sin_family = AF_INET;
    server.sin_port = htons(FTBN_bootstrap_config.server_port);

    flag = 0;        
    for (i=0;i<FTBN_BOOTSTRAP_BACKOFF_RETRY_COUNT; i++) {
        fd_set fds;
        struct timeval timeout;
        FD_ZERO(&fds);
        FD_SET(fd, &fds);
        
        timeout.tv_sec = timeout_milli/1000;
        timeout.tv_usec = (timeout_milli%1000 * 1000);

        FTB_INFO("sending pkt type %d",pkt_send->bootstrap_msg_type);
        if (sendto(fd, pkt_send, sizeof(FTBN_bootstrap_pkt_t), 0, (struct sockaddr*)&server, sizeof(struct sockaddr_in))!=sizeof(FTBN_bootstrap_pkt_t)) {
            close(fd);
            FTB_ERR_ABORT("sendto failed");
        }

        if (select(fd+1, &fds, NULL, NULL, &timeout)<0) {
            if (errno == EINTR || errno == EAGAIN)
                continue;
            close(fd);
            FTB_ERR_ABORT("select failed");
        }

        if (FD_ISSET(fd,&fds)) {
            if (recvfrom(fd, pkt_recv, sizeof(FTBN_bootstrap_pkt_t), 0, NULL, 0)!=sizeof(FTBN_bootstrap_pkt_t)) {
                close(fd);
                FTB_ERR_ABORT("sendto failed");
            }
            flag = 1;
            break;
        }
        else {
            timeout_milli = timeout_milli * FTBN_BOOTSTRAP_BACKOFF_RATIO;
        }
    }
    if (flag == 0) 
        return FTB_ERR_NETWORK_NO_ROUTE;
    close(fd);
    return FTB_SUCCESS;
}

static void util_get_network_addr(FTBN_addr_sock_t *my_addr)
{
    /*Setup my hostname*/
    my_addr->port = FTBN_bootstrap_config.agent_port;
#ifdef FTB_BGL_ION
    FTBU_get_output_of_cmd("grep ^BGL_IP /proc/personality.sh | cut -f2 -d=", my_addr->name, FTB_MAX_HOST_NAME);
#else
    FTBU_get_output_of_cmd("hostname",my_addr->name,FTB_MAX_HOST_NAME);
#endif
}



int FTBN_Bootstrap_init(const FTBN_config_info_t *config_info, FTBN_config_sock_t *config, FTBN_addr_sock_t *my_addr)
{
    FTBN_util_setup_config_sock(&FTBN_bootstrap_config);
    memcpy(config,&FTBN_bootstrap_config, sizeof(FTBN_config_sock_t));
    
    util_get_network_addr(&FTBN_bootstrap_my_addr);
    memcpy(my_addr, &FTBN_bootstrap_my_addr, sizeof(FTBN_addr_sock_t));
    memcpy(&FTBN_bootstrap_config_location, config_info, sizeof(FTBN_config_info_t));
    
    return FTB_SUCCESS;
}

int FTBN_Bootstrap_get_parent_addr(uint16_t my_level, FTBN_addr_sock_t *parent_addr, uint16_t *parent_level)
{
    /*
    If it is for reconnection, keep my_level and provide the old parent_addr so that the db server will not give the same parent;
    Otherwise, set the port to 0 to indicate an invalid parent addr, also my_level should be set to max value (0-1);
    */
    FTBN_bootstrap_pkt_t pkt_send, pkt_recv;
    int ret;

    pkt_send.bootstrap_msg_type = FTBN_BOOTSTRAP_MSG_TYPE_ADDR_REQ;
    pkt_send.level = my_level;
    
    memcpy(&pkt_send.addr, parent_addr, sizeof(FTBN_addr_sock_t));
    ret = util_exchange_bootstrap_msg(&pkt_send, &pkt_recv);
    if (ret != FTB_SUCCESS)
        return ret;
    
    FTB_INFO("received msg type %d, parent hostname %s, port %d, level %u",
        pkt_recv.bootstrap_msg_type,pkt_recv.addr.name, pkt_recv.addr.port, pkt_recv.level);
    if (pkt_recv.bootstrap_msg_type != FTBN_BOOTSTRAP_MSG_TYPE_ADDR_REP) {
        return FTB_ERR_GENERAL;
    }
    
    memcpy(parent_addr, (void*) &pkt_recv.addr, sizeof(FTBN_addr_sock_t));
    *parent_level = pkt_recv.level;
    return FTB_SUCCESS;
}

int FTBN_Bootstrap_report_conn_failure(const FTBN_addr_sock_t *addr)
{
    FTBN_bootstrap_pkt_t pkt_send;

    pkt_send.bootstrap_msg_type = FTBN_BOOTSTRAP_MSG_TYPE_CONN_FAIL;
    memcpy(&pkt_send.addr, addr, sizeof(FTBN_addr_sock_t));
    util_send_bootstrap_msg(&pkt_send);

    return FTB_SUCCESS;
}

int FTBN_Bootstrap_register_addr(uint16_t my_level)
{
    FTBN_bootstrap_pkt_t pkt_recv, pkt_send;
    int ret;

    if (FTBN_bootstrap_config_location.leaf)
        return FTB_ERR_NOT_SUPPORTED;

    pkt_send.bootstrap_msg_type = FTBN_BOOTSTRAP_MSG_TYPE_REG_REQ;
    pkt_send.level = my_level;
    memcpy(&pkt_send.addr, &FTBN_bootstrap_my_addr, sizeof(FTBN_addr_sock_t));

    FTB_INFO("registering hostname %s, port %d, level %d",
        pkt_send.addr.name, pkt_send.addr.port, pkt_send.level);
    ret = util_exchange_bootstrap_msg(&pkt_send, &pkt_recv);
    if (ret != FTB_SUCCESS)
        return ret;
    
    if (pkt_recv.bootstrap_msg_type != FTBN_BOOTSTRAP_MSG_TYPE_REG_REP) {
        return FTB_ERR_GENERAL;
    }
    
    return FTB_SUCCESS;
}

static int util_bootstrap_deregister_addr(int blocking)
{
    FTBN_bootstrap_pkt_t pkt_recv, pkt_send;
    int ret;

    if (FTBN_bootstrap_config_location.leaf)
        return FTB_ERR_NOT_SUPPORTED;

    pkt_send.bootstrap_msg_type = FTBN_BOOTSTRAP_MSG_TYPE_DEREG_REQ;
    memcpy(&pkt_send.addr, &FTBN_bootstrap_my_addr, sizeof(FTBN_addr_sock_t));

    if (blocking) {
        ret = util_exchange_bootstrap_msg(&pkt_send, &pkt_recv);
        if (ret != FTB_SUCCESS)
            return ret;

        FTB_INFO("received msg type %d, parent hostname %s, port %d",pkt_recv.bootstrap_msg_type,pkt_recv.addr.name, pkt_recv.addr.port);
        if (pkt_recv.bootstrap_msg_type != FTBN_BOOTSTRAP_MSG_TYPE_DEREG_REP) {
            return FTB_ERR_GENERAL;
        }
    }
    else {
        util_send_bootstrap_msg(&pkt_send);
    }

    return FTB_SUCCESS;
}

int FTBN_Bootstrap_finalize()
{
    return util_bootstrap_deregister_addr(1);
}

int FTBN_Bootstrap_abort(void)
{
    return util_bootstrap_deregister_addr(0);
}
