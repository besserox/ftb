#ifndef FTB_NETWORK_H
#define FTB_NETWORK_H

#include "ftb.h"
#include "ftb_core.h"

#ifdef __cplusplus
extern "C" {
#endif 

#define FTBN_MAX_MSG_LEN     200;

/*Current definition and implementation is based on TCP socket, can be extended to use other networks later*/
typedef struct FTBN_config_tcp{
    int dummy;
}FTBN_config_tcp_t;

typedef struct FTBN_addr_tcp{
    char name[FTB_MAX_HOST_NAME];
    int port;
}FTBN_addr_tcp_t;

#ifdef SOME_NETWORK

#else
typedef FTBN_addr_tcp_t FTBN_addr_t;
typedef FTBN_config_tcp_t FTBN_config_t;
#endif

typedef struct FTBN_header {
    int msg_type;
    FTB_component_id_t src;
    FTB_component_id_t dst;
    size_t payload_size;
}FTBN_header_t;


/*site-dependent code for bootstraping.*/
/*when it returns, the parent should be ready to accept connection.*/
int FTB_Bootstrap_network_config(FTBN_config_t* config);

int FTB_Bootstrap_connect(const FTBN_addr_t *my_addr, const FTB_node_properites_t* my_node_properties, FTBN_addr_t *parent_addr);

int FTBN_Init(const FTBN_config_t* config, FTBN_addr_t *my_addr);

int FTBN_Finalize(void);

int FTBN_Send_msg(const FTBN_header_t *header, const void *payload);

/*Blocking receive*/
int FTBN_Recv_msg(FTBN_header_t *header, void *payload);

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
