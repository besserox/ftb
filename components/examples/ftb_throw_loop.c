/*
 * FTB Throw Loop
 *
 *    In this example, each of the N ranks performs the following operations in
 * a tight loop.
 *     - Throws  one event.
 *     - Catches N/2 events.
 *
 *     A suitable script can be run on the IO Node to sample the CPU Utilization
 * of the FTB Agent and measure its scalibility.
 *
 *    Note that this example currently works only on the BG/P system. With some
 * minor  modifications, it can be made to work on  other systems  that use MPI.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libftb.h>

#define SKIP_COMMA(_p)          \
    do {                        \
        if (!_p) break;         \
        while (*_p) {           \
            if (*_p == '\0') {  \
                _p = NULL;      \
                break;          \
            }                   \
            if (*_p == ',') {   \
                ++_p;           \
                break;          \
            }                   \
            ++_p;               \
        }                       \
    } while(0)

int main(int argc, char **argv, char **env)
{
    char *evar;
    int rank, nprocs;
    int i, ret;

    FTB_client_t            cinfo;
    FTB_client_handle_t     chandle;
    FTB_event_info_t        einfo[1];
    FTB_subscribe_handle_t  shandle;
    FTB_event_handle_t      ehandle;
    char                    subscription_str[64];
    FTB_receive_event_t     revent;

    evar = getenv("CONTROL_INIT");
    SKIP_COMMA(evar);
    SKIP_COMMA(evar);
    nprocs = strtol(evar, &evar, 10);
    SKIP_COMMA(evar);
    rank = strtol(evar, &evar, 10);


    if (rank == 0) {
        fprintf(stdout, "Running FTB Throw Loop\n");
    }
    sleep(3);

    /* Doing an FTB_Connect() */
    strncpy(cinfo.event_space, "FTB.MPI.EXAMPLE", FTB_MAX_EVENTSPACE);
    snprintf(cinfo.client_name, FTB_MAX_CLIENT_NAME, "PROC_%d", rank);
    strcpy(cinfo.client_subscription_style, "FTB_SUBSCRIPTION_POLLING");
    ret = FTB_Connect(&cinfo, &chandle);
    if (ret != FTB_SUCCESS) goto err1;

    /* Declaring Publishable Events */
    snprintf(einfo[0].event_name, FTB_MAX_EVENT_NAME, "SR_EVENT%d", rank);
    strcpy(einfo[0].severity, "info");
    ret = FTB_Declare_publishable_events(chandle, NULL, einfo, 1);
    if (ret != FTB_SUCCESS) goto err2;

    /* Subscribing to necessary Events */
    strcpy(subscription_str, "event_name=ALL");
    ret = FTB_Subscribe(&shandle, chandle, subscription_str, NULL, NULL);
    if (ret != FTB_SUCCESS) goto err3;

    fprintf(stdout, "Rank %d alive\n", rank);
    sleep(3);

    int j = 0;
    while (1) {
        if (j<1000) {
            fprintf(stdout, "[Rank %d] Iteration %d\n", rank, j);
            j++;
        }

        /* Publish Event */
        ret = FTB_Publish(chandle, einfo[0].event_name, NULL, &ehandle);
        if (ret != FTB_SUCCESS) {
            fprintf(stdout, "[Rank %d] FTB_Publish() returned %d\n", rank, ret);
        }

        /* Poll for certain number of events */
        for (i=0; i<rank; i++) {
            /* FIXME: May need better error handling */
            while ((ret = FTB_Poll_event(shandle, &revent)) == FTB_GOT_NO_EVENT);
            /* Assume that the message you receive is the right one */
        }

        usleep(10000);
    }

    /* Control never gets here */

err1:
    fprintf(stdout, "[Rank %d] FTB_Connect() failed with %d\n", rank, ret);
    ret = -1;
    goto exit1;

err2:
    fprintf(stdout, "[Rank %d] FTB_Declare_publishable_events() failed with %d\n", rank, ret);
    ret = -2;
    goto exit2;

err3:
    fprintf(stdout, "[Rank %d] FTB_Subscribe() failed with %d\nSubscription String: %s\n",
                     rank, ret, subscription_str);
    ret = -3;
    goto exit3;

exit3:
exit2:
    FTB_Disconnect(chandle);

exit1:
    return(ret);
}
