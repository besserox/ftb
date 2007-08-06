#ifndef FTB_NETWORK_H
#define FTB_NETWORK_H

#include "ftb.h"
#include "ftb_manager.h"

#ifdef __cplusplus
extern "C" {
#endif 

#define FTBN_MAX_MSG_LEN     200

#define FTBN_NO_MSG               0x1

/*Current definition and implementation is based on sockets, can be extended to use other networks later*/

typedef struct FTBN_config_sock{
    int dummy;
}FTBN_config_sock_t;

typedef struct FTBN_addr_sock{
    char name[FTB_MAX_HOST_NAME];
    int port;
}FTBN_addr_sock_t;
#define FTBN_IS_NULL_ADDR_SOCK(x)    ((x).port == 0)


#ifdef SOME_NETWORK

#else
typedef FTBN_addr_sock_t FTBN_addr_t;
typedef FTBN_config_sock_t FTBN_config_t;
#define FTBN_IS_NULL_ADDR(x)       FTBN_IS_NULL_ADDR_SOCK(x)
#endif

typedef struct FTBN_header {
    int msg_type;
    FTB_unique_id_t src;
    FTB_unique_id_t dst;
    size_t payload_size;
}FTBN_header_t;


/*site-dependent code for bootstraping.*/
int FTB_Bootstrap_network_config(FTBN_config_t* config);

/*when it returns, the parent should be ready to accept connection.*/
int FTB_Bootstrap_connect(const FTBN_addr_t *my_addr, const FTB_node_properties_t* my_node_properties, FTBN_addr_t *parent_addr);

int FTBN_Init(const FTBN_config_t* config, FTBN_addr_t *my_addr);

int FTBN_Finalize(void);

int FTBN_Send_msg(const FTBN_header_t *header, const void *payload);

/*Blocking receive, which blocks until any message coming*/
int FTBN_Recv_msg_blocking(FTBN_header_t *header, void *payload);

/*Non-blocking receive, which returns immediately*/
int FTBN_Recv_msg_nonblocking(FTBN_header_t *header, void *payload);

/*
Create a thread to listen for new connections, handler function will be called upon new connection request.
If handler function does not return FTB_SUCCESS, reject connection.
*/
int FTBN_Listen(int (*conn_handler)(FTBN_header_t *, void *));

int FTBN_Connect(const FTBN_addr_t *addr, FTB_component_id_t *id);

#ifdef __cplusplus
} /*extern "C"*/
#endif 

#endif /* FTB_NETWORK_H */
