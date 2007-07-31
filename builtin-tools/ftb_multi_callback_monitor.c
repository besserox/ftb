#include "libftb.h"

#define FTB_MONITOR_ID         0x00001000

int event_handler_job_all(FTB_event_inst_t *event, void *arg)
{
    printf("Job Event: id: %d, name: %s, severity %d, src namespace %d, src id %lu, data: %s\n",
      event->event_id, event->name, event->severity, event->src_namespace, event->src_id, event->immediate_data);
    return 0;
}

int event_handler_backplane_error_fatal(FTB_event_inst_t *event, void *arg)
{
    printf("Backplane Severe Event: id: %d, name: %s, severity %d, src namespace %d, src id %lu, data: %s\n",
      event->event_id, event->name, event->severity, event->src_namespace, event->src_id, event->immediate_data);  
    return 0;
}

int main (int argc, char *argv[])
{
    FTB_component_properties_t properties;
    uint32_t monitor_id = FTB_MONITOR_ID;
    FTB_event_mask_t mask_job, mask_backplane_error_fatal;
    
    if (argc >= 2) {
        monitor_id = atoi(argv[1]);
    }

    properties.com_namespace = FTB_NAMESPACE_BACKPLANE;
    properties.id = FTB_SRC_ID_PREFIX_BACKPLANE_LOGGER | monitor_id;
    strcpy(properties.name,"MONITOR");
    properties.polling_only = 0;
    properties.polling_queue_len = 0;

    FTB_Init(&properties);
    FTB_EVENT_CLR_ALL(mask_job);
    FTB_EVENT_MARK_ALL_EVENT_ID(mask_job);
    FTB_EVENT_MARK_ALL_SEVERITY(mask_job);
    FTB_EVENT_SET_SRC_NAMESPACE(mask_job, FTB_NAMESPACE_JOB);
    FTB_EVENT_MARK_ALL_SRC_ID(mask_job);
    
    FTB_EVENT_CLR_ALL(mask_backplane_error_fatal);
    FTB_EVENT_MARK_ALL_EVENT_ID(mask_backplane_error_fatal);
    FTB_EVENT_SET_SEVERITY(mask_backplane_error_fatal,FTB_EVENT_SEVERITY_ERROR);
    FTB_EVENT_SET_SEVERITY(mask_backplane_error_fatal,FTB_EVENT_SEVERITY_FATAL);
    FTB_EVENT_SET_SRC_NAMESPACE(mask_backplane_error_fatal, FTB_NAMESPACE_BACKPLANE);
    FTB_EVENT_MARK_ALL_SRC_ID(mask_backplane_error_fatal);

    FTB_Reg_catch_notify(&mask_job,event_handler_job_all, NULL);
    FTB_Reg_catch_notify(&mask_backplane_error_fatal,event_handler_backplane_error_fatal, NULL);

    while(1) {
	    sleep(30);
    }
    
    FTB_Finalize();
    
    return 0;
}

