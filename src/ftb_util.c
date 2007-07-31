#include "ftb_util.h"

int FTB_Util_match_mask(FTB_event_inst_t *event_inst, FTB_event_mask_t *mask)
{
    if ( (event_inst->event_id & mask->event_id) != 0 
    &&  (event_inst->event_id & ~mask->event_id) == 0 
    &&  (event_inst->severity & mask->severity) != 0
    &&  (event_inst->severity & ~mask->severity) == 0
    && (event_inst->src_namespace  & mask->src_namespace) !=0
    && (event_inst->src_namespace & ~mask->src_namespace) == 0
    && (event_inst->src_id & mask->src_id) !=0
    && (event_inst->src_id & ~mask->src_id) == 0)
        return 1;
    else
        return 0;
}


int FTB_Util_setup_configuration(FTB_config_t* config) 
{
	char *temp;
	FILE *fp;

	if ((temp = getenv("FTB_CONF_FILE"))==NULL) {
		temp = FTB_DEFAULT_CONF_FILE;
	}

	fp = fopen(temp, "r");
	if (fp == NULL) {
		FTB_ERR_ABORT("can not open %s",temp);
	}

	fscanf(fp, "%u %s %u", &(config->FTB_id), config->host_name,
			&(config->port));
	fclose(fp);
	return 0;
}

