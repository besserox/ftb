#ifndef FTB_MANAGER_H
#define FTB_MANAGER_H

#include "ftb_def.h"

#ifdef __cplusplus
extern "C" {
#endif

/*Event delivery message*/
#define FTBM_MSG_TYPE_NOTIFY                     0x01

/*Component registration/deregistration, only src field is valid*/
#define FTBM_MSG_TYPE_COMP_REG                   0x11
#define FTBM_MSG_TYPE_COMP_DEREG                 0x12

/*Control messages, treat the event as eventmask*/
#define FTBM_MSG_TYPE_REG_THROW                  0x21
#define FTBM_MSG_TYPE_REG_CATCH                  0x22
#define FTBM_MSG_TYPE_REG_THROW_CANCEL           0x23
#define FTBM_MSG_TYPE_REG_CATCH_CANCEL           0x24

typedef struct FTBM_msg {
    int msg_type;
    FTB_id_t src;
    FTB_id_t dst;
    FTB_event_t event;
}FTBM_msg_t;

int FTBM_Init(int leaf);

int FTBM_Finalize(void);

int FTBM_Abort(void);

int FTBM_Get_location_id(FTB_location_id_t *location_id);

int FTBM_Get_parent_location_id(FTB_location_id_t *location_id);

int FTBM_Component_reg(const FTB_id_t *id);
   
int FTBM_Component_dereg(const FTB_id_t *id);

/*
Communication calls
*/
int FTBM_Send(const FTBM_msg_t *msg);

#define FTBM_NO_MSG           0x1
int FTBM_Poll(FTBM_msg_t *msg, FTB_location_id_t *incoming_src);

int FTBM_Wait(FTBM_msg_t *msg, FTB_location_id_t *incoming_src);

/*
Local book-keeping
*/
int FTBM_Reg_throw(const FTB_id_t *id, FTB_event_t *event);

int FTBM_Reg_catch(const FTB_id_t *id, FTB_event_t *event);

int FTBM_Reg_throw_cancel(const FTB_id_t *id, FTB_event_t *event);

int FTBM_Reg_catch_cancel(const FTB_id_t *id, FTB_event_t *event);

int FTBM_Get_catcher_comp_list(const FTB_event_t *event, FTB_id_t **list, int *len);

int FTBM_Release_comp_list(FTB_id_t *list);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*FTB_MANAGER_H*/
