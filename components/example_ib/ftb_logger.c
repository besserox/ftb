#include "libftb.h"

#include <time.h>

#define FTB_DEFAULT_LOGGER_ID         0x00000001
#define DEFAULT_LOG_FILE "/tmp/ftb_log"

static FILE* logFile;

int event_handler(FTB_event_inst_t *event, void *arg)
{
	time_t current = time(NULL);
	fprintf(logFile,"%s\t",asctime(localtime(&current)));
	fprintf(logFile,"Log Event: id: %d, name: %s, severity %d, src namespace %d, src id %lu, data: %s\n",
        event->event_id, event->name, event->severity, event->src_namespace, event->src_id, event->immediate_data);
	fflush(logFile);
	return 0;
}

int main (int argc, char *argv[])
{
    FTB_component_properties_t properties;
    uint32_t logger_id = FTB_DEFAULT_LOGGER_ID;
    
    if (argc >= 2) {
        logger_id = atoi(argv[1]);
    }

    properties.com_namespace = FTB_NAMESPACE_BACKPLANE;
    properties.id = FTB_SRC_ID_PREFIX_BACKPLANE_LOGGER | logger_id;
    strcpy(properties.name,"LOGGER");
    properties.polling_only = 0;
    properties.polling_queue_len = 0;

    {
    	char *temp;
    	if ((temp = getenv("FTB_LOG_FILE")) == NULL) {
    		temp = DEFAULT_LOG_FILE;
    	}
    	logFile = fopen(temp, "w");
    }

    FTB_Init(&properties);
    FTB_Reg_catch_notify_all(event_handler, NULL);

    while(1) {
	    sleep(30);
    }
    
    fclose(logFile);
    FTB_Finalize();
    
    return 0;
}

