#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <string.h>

#include "ftb_def.h"
#include "ftb_manager_lib.h"
#include "ftb_util.h"

static volatile int done = 0;    
static pthread_t progress_thread;
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

FILE* FTBU_log_file_fp;

void *progress_loop(void *arg)
{
    FTBM_msg_t msg;
    FTBM_msg_t msg_send;
    int ret;
    FTB_id_t *ids;
    int ids_len;
    int i;
    FTB_location_id_t incoming_src;
    
    pthread_mutex_lock(&lock);
    while (1) {
        pthread_mutex_unlock(&lock);
        ret = FTBM_Wait(&msg, &incoming_src);
        if (ret != FTB_SUCCESS) {
            FTB_WARNING("FTBM_Wait failed %d",ret);
            continue; 
        }
        pthread_mutex_lock(&lock);
        if (msg.msg_type == FTBM_MSG_TYPE_NOTIFY) {
            msg_send.msg_type = FTBM_MSG_TYPE_NOTIFY;
            memcpy(&msg_send.src, &msg.src, sizeof(FTB_id_t));
            memcpy(&msg_send.event, &msg.event, sizeof(FTB_event_t));
            ret = FTBM_Get_catcher_comp_list(&msg.event, &ids, &ids_len);
            if (ret != FTB_SUCCESS) {
                FTB_WARNING("FTBM_Get_catcher_comp_list failed");
                continue;
            }
            for (i=0;i<ids_len;i++) {
                if (ids[i].client_id.comp == FTB_COMP_MANAGER 
               && ids[i].client_id.comp_ctgy == FTB_COMP_CTGY_BACKPLANE
               && FTBU_is_equal_location_id(&ids[i].location_id,&incoming_src))  {
                /*not send same msg back in case incoming source is another agent*/
                    continue;
                }
                memcpy(&msg_send.dst, &ids[i], sizeof(FTB_id_t));
                ret = FTBM_Send(&msg_send);
                if (ret != FTB_SUCCESS) {
                    FTB_WARNING("FTBM_Send failed %d",ret);
                }
            }
            ret = FTBM_Release_comp_list(ids);
        }
        else if (msg.msg_type == FTBM_MSG_TYPE_COMP_REG) {
            ret = FTBM_Component_reg(&msg.src);
            if (ret != FTB_SUCCESS) {
                FTB_WARNING("FTBM_Component_reg failed");
            }
        }
        else if (msg.msg_type == FTBM_MSG_TYPE_COMP_DEREG) {
            ret = FTBM_Component_dereg(&msg.src);
            if (ret != FTB_SUCCESS) {
                FTB_WARNING("FTBM_Component_reg failed");
            }
        }
        else if (msg.msg_type == FTBM_MSG_TYPE_REG_CATCH) {
            ret = FTBM_Reg_catch(&msg.src, &msg.event);
            if (ret != FTB_SUCCESS) {
                FTB_WARNING("FTBM_Reg_catch failed");
            }
        }
        else if (msg.msg_type == FTBM_MSG_TYPE_REG_THROW) {
            ret = FTBM_Reg_throw(&msg.src, &msg.event);
            if (ret != FTB_SUCCESS) {
                FTB_WARNING("FTBM_Reg_throw failed");
            }
        }
        else if (msg.msg_type == FTBM_MSG_TYPE_REG_CATCH_CANCEL) {
            ret = FTBM_Reg_catch_cancel(&msg.src, &msg.event);
            if (ret != FTB_SUCCESS) {
                FTB_WARNING("FTBM_Reg_catch_cancel failed");
            }
        }
        else if (msg.msg_type == FTBM_MSG_TYPE_REG_THROW_CANCEL) {
            ret = FTBM_Reg_throw_cancel(&msg.src, &msg.event);
            if (ret != FTB_SUCCESS) {
                FTB_WARNING("FTBM_Reg_throw_cancel failed");
            }
        }
        else {
            FTB_WARNING("Unknown message type %d",msg.msg_type);
        }
    }
    return NULL;
}

void handler(int sig)
{
    if (sig == SIGINT || sig == SIGTERM) {
        done = 1;
    }
}

#define FTB_DEBUG

int main(int argc, char *argv[]) 
{
    int ret;

#ifdef FTB_DEBUG
    if (argc >= 2 && strcmp("ION_AGENT",argv[1])==0)
    {
        char filename[128], hostname[32], TIME[32];
        FTBU_get_output_of_cmd("hostname",hostname,32);
        FTBU_get_output_of_cmd("date +%m-%d-%H-%M-%S",TIME,32);
        sprintf(filename,"%s.%s.%s","/bgl/home1/qgao/ftb_bgl/IO_agent_output",hostname,TIME);
        FTBU_log_file_fp = fopen(filename, "w");
        fprintf(FTBU_log_file_fp, "Begin log file\n");
        fflush(FTBU_log_file_fp);
    }
    else
#endif
    FTBU_log_file_fp = stderr;

    ret = FTBM_Init(0);
    if (ret != FTB_SUCCESS) {
        FTB_ERR_ABORT("FTBM_Init failed %d",ret);
    }
    pthread_create(&progress_thread, NULL, progress_loop, NULL);
    
    signal(SIGINT, handler);
    signal(SIGTERM, handler);
    while (!done) {
        sleep(1);
    }
    pthread_mutex_lock(&lock);
    pthread_cancel(progress_thread);
    pthread_join(progress_thread, NULL);

    FTBM_Finalize();

    if(FTBU_log_file_fp != stderr) {
        fclose(FTBU_log_file_fp);
    }

    return 0;
}
