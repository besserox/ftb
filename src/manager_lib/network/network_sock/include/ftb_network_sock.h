/***********************************************************************************/
/* This file is part of FTB (Fault Tolerance Backplance) - the core of CIFTS
 * (Co-ordinated Infrastructure for Fault Tolerant Systems)
 * 
 * See http://www.mcs.anl.gov/research/cifts for more information.
 * 	
 */
/* This software is licensed under BSD. See the file FTB/misc/license.BSD for
 * complete details on your rights to copy, modify, and use this software.
 */
/************************************************************************************/
#ifndef FTB_NETWORK_TCP_H
#define FTB_NETWORK_TCP_H

#include "ftb_util.h"
#include <netdb.h>
#include <arpa/inet.h>

/* *INDENT-OFF* */
#ifdef __cplusplus
extern "C" {
#endif
/* *INDENT-ON* */

extern FILE *FTBU_log_file_fp;

typedef struct FTBN_addr_sock {
    char name[FTB_MAX_HOST_ADDR];
    int port;
} FTBN_addr_sock_t;

typedef struct FTBNI_config_sock {
    int agent_port;
    char server_name[FTB_MAX_HOST_ADDR];
    int server_port;
} FTBNI_config_sock_t;

#define FTBNI_LOCAL_IP  "127.0.0.1"

#define FTBNI_CONFIG_FILE_VAL 128

/* totally try ten times maximum */
#define FTBN_CONNECT_RETRY_COUNT                     1

/* in milliseconds */
#define FTBN_CONNECT_BACKOFF_INIT_TIMEOUT             1

/* every time the timeout times four */
#define FTBN_CONNECT_BACKOFF_RATIO                    4

/*
 * A UDP design of bootstraping module. It is to handle the initial dymanic
 * startup and dynamic changes. It provides mechanisms to connect to a
 *database server and exchange bootstraping information.
 */

/* in milliseconds */
#define FTBNI_BOOTSTRAP_BACKOFF_INIT_TIMEOUT          100

/* every time the timeout times four */
#define FTBNI_BOOTSTRAP_BACKOFF_RATIO                  4

/* totally try ten times maximum */
#define FTBNI_BOOTSTRAP_BACKOFF_RETRY_COUNT            5

#define FTBNI_BOOTSTRAP_MSG_TYPE_ADDR_REQ    1
#define FTBNI_BOOTSTRAP_MSG_TYPE_ADDR_REP    2
#define FTBNI_BOOTSTRAP_MSG_TYPE_REG_REQ     3
#define FTBNI_BOOTSTRAP_MSG_TYPE_REG_REP     4
#define FTBNI_BOOTSTRAP_MSG_TYPE_DEREG_REQ   5
#define FTBNI_BOOTSTRAP_MSG_TYPE_DEREG_REP   6
#define FTBNI_BOOTSTRAP_MSG_TYPE_CONN_FAIL   7
#define FTBNI_BOOTSTRAP_MSG_TYPE_REG_INVALID 10

typedef struct FTBNI_bootstrap_pkt {
    uint8_t bootstrap_msg_type;
    FTBN_addr_sock_t addr;
    uint16_t level;
} FTBNI_bootstrap_pkt_t;

int FTBNI_Bootstrap_init(const FTBN_config_info_t * config_info, FTBNI_config_sock_t * config,
                         FTBN_addr_sock_t * my_addr);

int FTBNI_Bootstrap_get_parent_addr(uint16_t my_level, FTBN_addr_sock_t * parent_addr,
                                    uint16_t * parent_level);

int FTBNI_Bootstrap_register_addr(uint16_t my_level);

int FTBNI_Bootstrap_report_conn_failure(const FTBN_addr_sock_t * addr);

int FTBNI_Bootstrap_finalize(void);

int FTBNI_Bootstrap_abort(void);

static inline void FTBNI_get_data_from_config_file(char *str, char *file, char *output_val, int *retval)
{
    FILE *fp;
    char *pos;
    char line[FTBNI_CONFIG_FILE_VAL];
    int found = 0;

    fp = fopen(file, "r");
    if (!fp) {
        *retval = -1;
        return;
    }
    while (!feof(fp)) {
        fgets(line, FTBNI_CONFIG_FILE_VAL, fp);
        if ((pos = strstr(line, str)) != NULL) {
            while (*pos++ != '=');
            strcpy(output_val, pos);
            found = 1;
            break;
        }
    }
    fclose(fp);
    if (!found)
        *retval = -1;
    return;
}

static inline void FTBNI_util_setup_config_sock(FTBNI_config_sock_t * config)
{
    char *env;

    if ((env = getenv("FTB_AGENT_PORT")) != NULL) {
        config->agent_port = atoi(env);
    }
    else if ((env = getenv("FTB_CONFIG_FILE")) != NULL) {
        char output_val[FTBNI_CONFIG_FILE_VAL];
        int retval = 0;
        FTBNI_get_data_from_config_file("FTB_AGENT_PORT", env, output_val, &retval);
        if (retval == -1) {
            config->agent_port = FTB_AGENT_PORT;
            FTB_INFO ("Error in accessing agent port information from config file %s. Assigning default port to the agent %d\n", env, config->agent_port); 
        }
        else {
            config->agent_port = atoi(output_val);
        }
    }
    else {
        config->agent_port = FTB_AGENT_PORT;
        FTB_INFO("Assigning default port to the agent: %d", config->agent_port);
    }

    if ((env = getenv("FTB_BSTRAP_PORT")) != NULL) {
        config->server_port = atoi(env);
    }
    else if ((env = getenv("FTB_CONFIG_FILE")) != NULL) {
        char output_val[FTBNI_CONFIG_FILE_VAL];
        int retval = 0;

        FTBNI_get_data_from_config_file("FTB_BSTRAP_PORT", env, output_val, &retval);
        if (retval == -1) {
            config->server_port = FTB_BSTRAP_PORT;
            FTB_INFO("Error in accessing server port information from config file %s. Assigning default port for bootstrap server : %d", env, config->server_port);
        }
        else {
            config->server_port = atoi(output_val);
        }
    }
    else {
        config->server_port = FTB_BSTRAP_PORT;
    }

    if ((env = getenv("FTB_BSTRAP_SERVER")) != NULL) {
        strncpy(config->server_name, env, FTB_MAX_HOST_ADDR);
    }
    else if ((env = getenv("FTB_CONFIG_FILE")) != NULL) {
        char output_val[FTBNI_CONFIG_FILE_VAL];
        int retval = 0;

        FTBNI_get_data_from_config_file("FTB_BSTRAP_SERVER", env, output_val, &retval);
        if (retval == -1) {
#ifdef FTB_BSTRAP_SERVER
            strncpy(config->server_name, FTB_BSTRAP_SERVER, FTB_MAX_HOST_ADDR);
            FTB_WARNING
                ("Error in accessing bootstrap server name information from config file %s. Assigning configure time server information: %s",
                 env, config->server_name);
#else
            FTB_ERR_ABORT("Cannot find boot-strap server ip address");
#endif
        }
        else {
            strncpy(config->server_name, output_val, FTB_MAX_HOST_ADDR);
        }
    }
    else {
#ifdef FTB_BSTRAP_SERVER
        strncpy(config->server_name, FTB_BSTRAP_SERVER, FTB_MAX_HOST_ADDR);
#else
        FTB_ERR_ABORT("Cannot find boot-strap server ip address");
#endif
    }
}

/*
 *  Function FTBNI_gethostbyname written by Hoony Park
 */
static inline struct hostent *FTBNI_gethostbyname(const char *name)
{
    static struct hostent *hp = NULL;

    if (hp == NULL) {
        if ((hp = (struct hostent *) malloc(sizeof(struct hostent))) == NULL) {
            exit(1);
        }
        hp->h_length = 4;

        if ((hp->h_addr_list = (char **) malloc(sizeof(char *) * 2)) == NULL) {
            exit(1);
        }

        if ((hp->h_addr_list[0] = (char *) malloc(sizeof(char) * 4)) == NULL) {
            exit(1);
        }

        hp->h_addr_list[1] = 0;
        hp->h_addr = hp->h_addr_list[0];
    }
    /*
     * if (strcmp(name,FTB_BSTRAP_SERVER) == 0) {
     * inet_aton(FTB_BSTRAP_SERVER,((struct in_addr*)hp->h_addr));
     * }
     * else
     */
    if (strcmp(name, "localhost") == 0) {
        inet_aton(FTBNI_LOCAL_IP, ((struct in_addr *) hp->h_addr));
    }
    else {
        /* case when an IP address is passed as
         * a host name */
        inet_aton(name, ((struct in_addr *) hp->h_addr));
    }

    return hp;
}

/* *INDENT-OFF* */
#ifdef __cplusplus
} /* extern "C" */
#endif
/* *INDENT-ON* */

#endif /*FTB_NETWORK_TCP_H */
