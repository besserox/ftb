#ifndef FTB_NETWORK_TCP_H
#define FTB_NETWORK_TCP_H

#include "ftb_conf.h"

#ifdef __cplusplus
extern "C" {
#endif 

typedef struct FTBN_addr_sock {
    char name[FTB_MAX_HOST_NAME];
    int port;
}FTBN_addr_sock_t;

typedef struct FTBN_config_sock {
    int agent_port;
    char server_name[FTB_MAX_HOST_NAME];
    int server_port;
}FTBN_config_sock_t;

#define FTBN_CONNECT_RETRY_COUNT                     10
/*totally try ten times maximum*/
#define FTBN_CONNECT_BACKOFF_INIT_TIMEOUT             1
/*in milliseconds*/
#define FTBN_CONNECT_BACKOFF_RATIO                    4
/*every time the timeout times four*/

/*
A UDP design of bootstraping module. It is to handle the initial dymanic 
startup and dynamic changes. It provides mechanisms to connect to a 
database server and exchange bootstraping information.
*/
#define FTBN_BOOTSTRAP_BACKOFF_INIT_TIMEOUT          100
/*in milliseconds*/
#define FTBN_BOOTSTRAP_BACKOFF_RATIO                   4
/*every time the timeout times four*/
#define FTBN_BOOTSTRAP_BACKOFF_RETRY_COUNT            10
/*totally try ten times maximum*/

#define FTBN_BOOTSTRAP_MSG_TYPE_ADDR_REQ    1
#define FTBN_BOOTSTRAP_MSG_TYPE_ADDR_REP    2
#define FTBN_BOOTSTRAP_MSG_TYPE_REG_REQ     3
#define FTBN_BOOTSTRAP_MSG_TYPE_REG_REP     4
#define FTBN_BOOTSTRAP_MSG_TYPE_DEREG_REQ   5
#define FTBN_BOOTSTRAP_MSG_TYPE_DEREG_REP   6
#define FTBN_BOOTSTRAP_MSG_TYPE_CONN_FAIL   7

typedef struct FTBN_bootstrap_pkt {
    uint8_t bootstrap_msg_type;
    FTBN_addr_sock_t addr;
    uint16_t level;
}FTBN_bootstrap_pkt_t;

int FTBN_Bootstrap_init(const FTBN_config_info_t *config_info, FTBN_config_sock_t *config, FTBN_addr_sock_t *my_addr);

int FTBN_Bootstrap_get_parent_addr(uint16_t my_level, FTBN_addr_sock_t *parent_addr, uint16_t *parent_level);

int FTBN_Bootstrap_register_addr(uint16_t my_level);

int FTBN_Bootstrap_report_conn_failure(const FTBN_addr_sock_t *addr);

int FTBN_Bootstrap_finalize(void);

int FTBN_Bootstrap_abort(void);

static inline void FTBN_util_setup_config_sock(FTBN_config_sock_t *config)
{
    config->agent_port = FTB_CONF_AGENT_PORT;
    config->server_port = FTB_CONF_SERVER_PORT;
    strncpy(config->server_name, FTB_CONF_SERVER_NAME, FTB_MAX_HOST_NAME);
}

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*FTB_NETWORK_TCP_H*/
