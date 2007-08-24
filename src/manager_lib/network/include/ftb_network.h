#ifndef FTB_NETWORK_H
#define FTB_NETWORK_H

#include "ftb_def.h"
#include "ftb_manager_lib.h"

/*Generic information common for all network, used for authentication and topology management*/
typedef struct FTBN_config_info {
    uint32_t FTB_system_id;
    int leaf;
}FTBN_config_info_t;

#if defined(FTB_NETWORK_SOCK)
#include "ftb_network_sock.h"
#elif defined(FTB_NETWORK_SOME_OTHER_NETWORK)
/*FILLME: some other header file for other networks*/
#endif

#ifdef __cplusplus
extern "C" {
#endif 

#if defined(FTB_NETWORK_SOCK)
typedef FTBN_addr_sock_t FTBN_addr_t;
#elif defined(FTB_NETWORK_SOME_OTHER_NETWORK)
/*FILLME: some other header file for other networks*/
#endif

int FTBN_Init(const FTB_location_id_t *my_id, const FTBN_config_info_t *config_info);

int FTBN_Connect(const FTBM_msg_t *reg_msg, FTB_location_id_t *parent_location_id);

int FTBN_Finalize(void);

int FTBN_Send_msg(const FTBM_msg_t *msg);

/*If blocking == 0, it returns FTBN_NO_MSG indicating no new message, or FTB_SUCCESS indicating new message received*/
#define FTBN_NO_MSG               0x1
int FTBN_Recv_msg(FTBM_msg_t *msg, FTB_location_id_t *incoming_src,  int blocking);

int FTBN_Disconnect_peer(const FTB_location_id_t *peer_location_id);

int FTBN_Abort(void);

#ifdef __cplusplus
} /*extern "C"*/
#endif 

#endif /* FTB_NETWORK_H */