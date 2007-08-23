#include <stdio.h>
#include <stdlib.h>

#include "ftb_def.h"
#include "ftb_util.h"
#include "ftb_client_lib.h"

#include "zoid_api.h"

FILE* FTBU_log_file_fp;

static int nclients;

#define FTB_DEBUG

void ftb_zoid_client_init(int count) 
{
    nclients = count;
#ifdef FTB_DEBUG
    {
        char filename[128], IP[32], TIME[32];
        FTBU_get_output_of_cmd("grep ^BGL_IP /proc/personality.sh | cut -f2 -d=",IP,32);
        FTBU_get_output_of_cmd("date +%m-%d-%H-%M-%S",TIME,32);
        sprintf(filename,"%s.%s.%s","/bgl/home1/qgao/ftb_bgl/zoid_output",IP,TIME);
        FTBU_log_file_fp = fopen(filename, "w");
        fprintf(FTBU_log_file_fp, "Begin log file\n");
        fflush(FTBU_log_file_fp);
    }
#else
    FTBU_log_file_fp = stderr;
#endif
}

void ftb_zoid_client_fini()
{
#ifdef FTB_DEBUG
    fclose(FTBU_log_file_fp);
#endif
}

int ZOID_FTB_Init( FTB_comp_ctgy_t category /* in:obj */,
                   FTB_comp_t component /* in:obj */,
                   const FTB_component_properties_t *properties /* in:ptr:nullok */,
                   FTB_client_handle_t *client_handle /* out:ptr */ )
{
    int proc_id = __zoid_calling_process_id();
    fprintf(FTBU_log_file_fp,"ZOID_FTB_Init\n");
    fflush(FTBU_log_file_fp);
    return FTBC_Init(category, component, proc_id, properties, client_handle);
}

int ZOID_FTB_Reg_throw( FTB_client_handle_t handle /* in:obj */,
                        FTB_event_name_t event /* in:obj */)
{
    fprintf(FTBU_log_file_fp,"ZOID_FTB_Reg_throw\n");
    fflush(FTBU_log_file_fp);
    return FTBC_Reg_throw(handle, event);
}

int ZOID_FTB_Reg_catch_polling_event( FTB_client_handle_t handle /* in:obj */,
                                      FTB_event_name_t event /* in:obj */)
{
    fprintf(FTBU_log_file_fp,"ZOID_FTB_Reg_catch_polling_event\n");
    fflush(FTBU_log_file_fp);
    return FTBC_Reg_catch_polling_event(handle, event);
}

int ZOID_FTB_Reg_catch_polling_mask( FTB_client_handle_t handle /* in:obj */,
                                     const FTB_event_t *event /* in:ptr */)
{
    fprintf(FTBU_log_file_fp,"ZOID_FTB_Reg_catch_polling_mask\n");
    fflush(FTBU_log_file_fp);
    return FTBC_Reg_catch_polling_mask(handle, event);
}

int ZOID_FTB_Throw ( FTB_client_handle_t handle /* in:obj */,
                     FTB_event_name_t event /* in:obj */)
{
    fprintf(FTBU_log_file_fp,"ZOID_FTB_Throw\n");
    fflush(FTBU_log_file_fp);
    return FTBC_Throw(handle, event);
}

int ZOID_FTB_Catch ( FTB_client_handle_t handle /* in:obj */,
                     FTB_event_t *event /* out:ptr */,
                     FTB_id_t *src /* out:ptr:nullok */)
{
    fprintf(FTBU_log_file_fp,"ZOID_FTB_Catch\n");
    fflush(FTBU_log_file_fp);
    return FTBC_Catch(handle, event, src);
}

int ZOID_FTB_Finalize (FTB_client_handle_t handle /* in:obj */)
{
    fprintf(FTBU_log_file_fp,"ZOID_FTB_Finalize\n");
    fflush(FTBU_log_file_fp);
    return FTBC_Finalize(handle);
}

int ZOID_FTB_Abort (FTB_client_handle_t handle /* in:obj */)
{
    fprintf(FTBU_log_file_fp,"ZOID_FTB_Abort\n");
    fflush(FTBU_log_file_fp);
    return FTBC_Abort(handle);
}


