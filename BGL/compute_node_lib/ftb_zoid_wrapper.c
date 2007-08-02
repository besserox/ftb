/*Compute node libftb wrapper based on ZOID for BG/L*/

#include "libftb.h"

#include "ftb_agent.h"

int FTB_Init(FTB_component_properties_t *properties)
{
    int ret;
    if (properties == NULL) {
        return -FTB_ERR_NULL_POINTER;
    }
    if (properties->polling_only == 0) {
        return -FTB_ERR_NOT_SUPPORTED;
    }
    ret = Agent_FTB_Init((void*)properties, sizeof(FTB_component_properties_t));
    return ret;
}

int FTB_Finalize()
{
    int ret = Agent_FTB_Finalize();
    return ret;
}

int FTB_Reg_throw(FTB_event_t *event)
{
    int ret;
    if (event == NULL) {
        return -FTB_ERR_NULL_POINTER;
    }
    ret = Agent_FTB_Reg_throw((void*)event, sizeof(FTB_event_t));
    return ret;
}

int FTB_Reg_throw_id(uint32_t event_id)
{
    int ret;
    FTB_event_t event;
    ret = FTB_get_event_by_id(event_id, &event);
    if (ret)
        return ret;
    ret = FTB_Reg_throw(&event);
    return ret;
}

/*Notify scheme not supported on BG/L*/
int FTB_Reg_catch_notify(FTB_event_mask_t *event_mask, int (*callback)(FTB_event_inst_t *, void*), void *arg)
{
    return -FTB_ERR_NOT_SUPPORTED;
}

int FTB_Reg_catch_notify_single(uint32_t event_id, int (*callback)(FTB_event_inst_t *, void*), void *arg)
{
    return -FTB_ERR_NOT_SUPPORTED;
}

int FTB_Reg_catch_notify_all(int (*callback)(FTB_event_inst_t *, void*), void *arg)
{
    return -FTB_ERR_NOT_SUPPORTED;
}


int FTB_Reg_catch_polling(FTB_event_mask_t *event_mask)
{
    int ret;
    if (event_mask == NULL) {
        return -FTB_ERR_NULL_POINTER;
    }
    ret = Agent_FTB_Reg_catch_polling((void*)event_mask, sizeof(FTB_event_mask_t));
    return ret;
}

int FTB_Reg_catch_polling_single(uint32_t event_id)
{
    int ret;
    FTB_event_mask_t mask;
    FTB_EVENT_CLR_ALL(mask);
    FTB_EVENT_MARK_ALL_SRC_NAMESPACE(mask);
    FTB_EVENT_MARK_ALL_SRC_ID(mask);
    FTB_EVENT_MARK_ALL_SEVERITY(mask);
    FTB_EVENT_SET_EVENT_ID(mask,event_id);
    ret = FTB_Reg_catch_polling(&mask);
    return ret;
}

int FTB_Reg_catch_polling_all()
{
    int ret;
    FTB_event_mask_t mask;
    FTB_EVENT_CLR_ALL(mask);
    FTB_EVENT_MARK_ALL_SRC_NAMESPACE(mask);
    FTB_EVENT_MARK_ALL_SRC_ID(mask);
    FTB_EVENT_MARK_ALL_SEVERITY(mask);
    FTB_EVENT_MARK_ALL_EVENT_ID(mask);
    ret = FTB_Reg_catch_polling(&mask);
    return ret;
}

int FTB_Throw(FTB_event_t *event, char *data, uint32_t data_len)
{
    int ret;
    if (event == NULL) {
        return -FTB_ERR_NULL_POINTER;
    }
    if (data_len != 0 && data == NULL) {
        return -FTB_ERR_NULL_POINTER;
    }
    ret = Agent_FTB_Throw((void*)event,sizeof(FTB_event_t),(void*)data,data_len);
    return ret;
}

int FTB_Throw_id(uint32_t event_id, char *data, uint32_t data_len)
{
    int ret;
    FTB_event_t event;
    ret = FTB_get_event_by_id(event_id, &event);
    if (ret)
        return ret;
    ret = FTB_Throw(&event,data,data_len);
    return ret;
}

int FTB_Catch(FTB_event_inst_t *event_inst)
{
    int ret;
    if (event_inst == NULL) {
        return -FTB_ERR_NULL_POINTER;
    }
    ret = Agent_FTB_Catch((void*)event_inst, sizeof(FTB_event_inst_t));
    return ret;
}

int FTB_List_events(FTB_event_t *events, int *len)
{
    *len = 0;
    return -FTB_ERR_NOT_SUPPORTED;
}
