#ifndef FTB_MANAGER_H
#define FTB_MANAGER_H

#include "ftb.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
FTB_node is a backplane entity which semantically represents a location. 
It is a standalone daemon or a plug-in of another running program, and it 
manages the event throw and catch for the components and it also can throw
& catch events. A FTB_node belongs to backplan namespace, and its id starts
with FTB_SRC_ID_PREFIX_BACKPLANE_NODE.
*/

#define FTB_NODE_TYPE_ROOT              1
#define FTB_NODE_TYPE_NORMAL            2
#define FTB_NODE_TYPE_LEAF              3

/*For authentication and management*/
typedef struct FTB_sys_info{
    uint32_t FTB_system_id;
}FTB_sys_info_t;

typedef struct FTB_node_properities {
    FTB_component_id_t component_id;
    int node_type;
}FTB_node_properites_t;

#define FTB_MSG_TYPE_INIT                       0x01
#define FTB_MSG_TYPE_REG_THROW                  0x11
#define FTB_MSG_TYPE_REG_CATCH                  0x12
#define FTB_MSG_TYPE_REG_THROW_CANCEL           0x13
#define FTB_MSG_TYPE_REG_CATCH_CANCEL           0x14
#define FTB_MSG_TYPE_THROW                      0x21
#define FTB_MSG_TYPE_CATCH                      0x22
#define FTB_MSG_TYPE_NOTIFY                     0x31
#define FTB_MSG_TYPE_LIST_EVENTS                0x60
#define FTB_MSG_TYPE_FIN                        0xFF

typedef struct FTB_msg_init {
    FTB_component_properties_t properties;
}FTB_msg_init_t;

typedef struct FTB_msg_fin {
    FTB_component_id_t id;
}FTB_msg_fin_t;

typedef struct FTB_msg_reg_throw {
    FTB_component_id_t id;
    FTB_event_t event;
}FTB_msg_reg_throw_t;

typedef struct FTB_msg_reg_catch {
    FTB_component_id_t id;
    FTB_event_mask_t event_mask;
    int reg_catch_type;
}FTB_msg_reg_catch_t;

typedef struct FTB_msg_throw {
    FTB_component_id_t id;
    FTB_event_inst_t event_inst;
}FTB_msg_throw_t;

typedef struct FTB_msg_catch {
    FTB_component_id_t id;
}FTB_msg_catch_t;

typedef struct FTB_msg_notify {
    FTB_event_inst_t event_inst;
}FTB_msg_notigy_t;

typedef struct FTB_msg_reg_throw_cancel {
    FTB_component_id_t id;
    FTB_event_t event;
}FTB_msg_reg_throw_cancel_t;

typedef struct FTB_msg_reg_catch_cancel {
    FTB_component_id_t id;
    FTB_event_mask_t event_mask;
    int reg_catch_type;
}FTB_msg_reg_catch_cancel_t;

/*Called when the ftb node get initialized*/
int FTBM_Init(const FTB_node_properites_t *node_properties);

/*Called when the ftb node get finalized*/
int FTBM_Finalize(void);

#define FTB_PROGRESS_ERROR            (-1)
#define FTB_PROGRESS_FINISH            0

int FTBM_Progress(void);

/*site-dependent code*/

/*Make local configurations before calling FTBM_Init, the effect is internal*/
int FTB_Bootstrap_local_config(void);

/*For unique identification and name of FTB core*/
int FTB_Bootstrap_get_component_id(FTB_component_id_t *component_id);

/*Called after the FTB core is made functional*/
int FTB_Bootstrap_done(void);

/*Component management & events handling functions*/
int FTBM_Component_reg(const FTB_component_properties_t *com_properties);
   
int FTBM_Component_dereg(FTB_component_id_t id);

int FTBM_Reg_throw(FTB_component_id_t id, const FTB_event_t *event);

int FTBM_Reg_catch_notify(FTB_component_id_t id, const FTB_event_mask_t *event_mask);

int FTBM_Reg_catch_polling(FTB_component_id_t id, const FTB_event_mask_t *event_mask);

int FTBM_Reg_throw_cancel(FTB_component_id_t id, FTB_event_id_t event_id);

int FTBM_Reg_catch_cancel_notify(FTB_component_id_t id, const FTB_event_mask_t *event_mask);

int FTBM_Reg_catch_cancel_polling(FTB_component_id_t id, const FTB_event_mask_t *event_mask);

int FTBM_Throw(FTB_component_id_t id, const FTB_event_inst_t *event_inst);

int FTBM_Catch(FTB_component_id_t id, FTB_event_inst_t *event_inst);

int FTBM_List_events(FTB_component_id_t id, FTB_event_t *events, int *len);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*FTB_MANAGER_H*/
