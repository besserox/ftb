#ifndef FTB_BOOTSTRAP_H 
#define FTB_BOOTSTRAP_H

#include "ftb.h"
#include "ftb_network.h"
#include "ftb_manager.h"
#include "ftb_conf.h"

/*
A UDP design of bootstraping module. It is to handle the initial dymanic 
startup (required by BG) and dynamic changes (to support failure 
recovery). It provides mechanisms to connect to a database server and 
exchange bootstraping information.
*/

#define FTB_BOOTSTRAP_BACKOFF_INIT_TIMEOUT          100
/*in milliseconds*/
#define FTB_BOOTSTRAP_BACKOFF_RATIO                          4
/*every time the timeout times four*/
#define FTB_BOOTSTRAP_BACKOFF_RETRY_COUNT            10
/*totally try ten times maximum*/

#define FTB_BOOTSTRAP_MSG_TYPE_ADDR_REQ   1
#define FTB_BOOTSTRAP_MSG_TYPE_ADDR_REP   2
#define FTB_BOOTSTRAP_MSG_TYPE_REG_REQ   3
#define FTB_BOOTSTRAP_MSG_TYPE_REG_REP   4
#define FTB_BOOTSTRAP_MSG_TYPE_DEREG_REQ   5
#define FTB_BOOTSTRAP_MSG_TYPE_DEREG_REP   6

typedef struct FTB_Bootstrap_pkt {
    int msg_type;
    FTB_unique_id_t id;
    int node_type;
    FTBN_addr_t addr;
}FTB_Bootstrap_pkt_t;

#endif /*FTB_BOOTSTRAP_H*/
