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

#define FTBM_EVENT_CLR_ALL(evt_mask) {\
    evt_mask.severity = 0x0; \
    evt_mask.comp_cat = 0x0; \
    evt_mask.comp = 0x0; \
    evt_mask.event_cat = 0x0; \
    evt_mask.event_name = 0x0; \
}

#define FTBM_EVENT_CLR_SEVERITY(evt_mask) {\
    evt_mask.severity = 0x0; \
}
#define FTBM_EVENT_CLR_COMP_CAT(evt_mask) {\
    evt_mask.comp_cat = 0x0; \
}
#define FTBM_EVENT_CLR_COMP(evt_mask) {\
    evt_mask.comp = 0x0; \
}
#define FTBM_EVENT_CLR_EVENT_CAT(evt_mask) {\
    evt_mask.event_cat = 0x0; \
}
#define FTBM_EVENT_CLR_EVENT_NAME(evt_mask) {\
    evt_mask.event_name = 0x0; \
}


/*use the underflow trick of unsigned int create all mask*/
#define FTBM_EVENT_SET_ALL(evt_mask) {\
    evt_mask.severity = 0-1; \
    evt_mask.comp_cat = 0-1; \
    evt_mask.comp = 0-1; \
    evt_mask.event_cat = 0-1; \
    evt_mask.event_name = 0-1; \
}

#define FTBM_EVENT_SET_ALL_SEVERITY(evt_mask) {\
    evt_mask.severity = 0-1; \
}
#define FTBM_EVENT_SET_ALL_COMP_CAT(evt_mask) {\
    evt_mask.comp_cat = 0-1; \
}
#define FTBM_EVENT_SET_ALL_COMP(evt_mask) {\
    evt_mask.comp = 0-1; \
}
#define FTBM_EVENT_SET_ALL_EVENT_CAT(evt_mask) {\
    evt_mask.event_cat = 0-1; \
}
#define FTBM_EVENT_SET_ALL_EVENT_NAME(evt_mask) {\
    evt_mask.event_name = 0-1; \
}

#define FTBM_EVENT_SET_SEVERITY(evt_mask, val) {\
    evt_mask.severity = evt_mask.severity | val; \
}
#define FTBM_EVENT_SET_COMP_CAT(evt_mask, val) {\
    evt_mask.comp_cat = evt_mask.comp_cat | val; \
}
#define FTBM_EVENT_SET_COMP(evt_mask, val) {\
    evt_mask.comp = evt_mask.comp | val; \
}
#define FTBM_EVENT_SET_EVENT_CAT(evt_mask, val) {\
    evt_mask.event_cat = evt_mask.event_cat | val; \
}

/*different from others because event_name field can only have one valid value*/
#define FTBM_EVENT_SET_EVENT_NAME(evt_mask, val) {\
    evt_mask.event_name = val; \
}

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
