/*
 * FTB Ring Test
 *
 *     In this FTB example, Rank i catches a message from Rank (i-1) and throws
 * the message to Rank (i+1). It performs this action 1024 times and calculates
 * the average  time taken for one  message to be  thrown from one  Rank to the
 * next.
 *
 *    Note that this example currently works only on the BG/P system. With some
 * minor  modifications, it can be made to work on  other systems  that use MPI.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

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
    int i, ret, niter;
    struct timespec startt, endt;
    double delay = 0;

    char s_event[24];
    char r_event[24];

    FTB_client_t            cinfo;
    FTB_client_handle_t     chandle;
    FTB_event_info_t        einfo[1];
    FTB_subscribe_handle_t  shandle;
    char                    subscription_str[64];
    FTB_event_handle_t      ehandle;
    FTB_receive_event_t     revent;

    evar = getenv("CONTROL_INIT");
    SKIP_COMMA(evar);
    SKIP_COMMA(evar);
    nprocs = strtol(evar, &evar, 10);
    SKIP_COMMA(evar);
    rank = strtol(evar, &evar, 10);


    if (rank == 0) {
        fprintf(stdout, "Running FTB Ringtest\n");
    }
    sleep(3);

    niter = 1024;

    /* Setup Event Names */
    snprintf(s_event, 24, "RANK%d_RANK%d", rank, ((rank+1)%nprocs));
    snprintf(r_event, 24, "RANK%d_RANK%d", ((rank-1+nprocs)%nprocs), rank);

    /* Doing an FTB_Connect() */
    strncpy(cinfo.event_space, "FTB.MPI.EXAMPLE", FTB_MAX_EVENTSPACE);
    snprintf(cinfo.client_name, FTB_MAX_CLIENT_NAME, "PROC_%d", rank);
    strcpy(cinfo.client_subscription_style, "FTB_SUBSCRIPTION_POLLING");
    ret = FTB_Connect(&cinfo, &chandle);
    if (ret != FTB_SUCCESS) goto err1;

    /* Declaring Publishable Events */
    strncpy(einfo[0].event_name, s_event, FTB_MAX_EVENT_NAME);
    strcpy(einfo[0].severity, "info");
    ret = FTB_Declare_publishable_events(chandle, NULL, einfo, 1);
    if (ret != FTB_SUCCESS) goto err2;

    /* Subscribing to necessary Events */
    sprintf(subscription_str, "event_name=%s", r_event);
    ret = FTB_Subscribe(&shandle, chandle, subscription_str, NULL, NULL);
    if (ret != FTB_SUCCESS) goto err3;

    fprintf(stdout, "Rank %d alive\n", rank);

    for (i=0; i<niter; i++) {
        usleep(100000);
        if (rank == 0) {
            clock_gettime(CLOCK_MONOTONIC, &startt);

            fprintf(stdout, "Rank %d: Publishing\n", rank);
            ret = FTB_Publish(chandle, s_event, NULL, &ehandle);
            if (ret != FTB_SUCCESS) goto perror;

            fprintf(stdout, "Rank %d: Polling\n", rank);
            while ((ret = FTB_Poll_event(shandle, &revent)) == FTB_GOT_NO_EVENT);
            /* Assume that the message you receive is the right one */
            goto pexit;

perror:
            fprintf(stdout, "Rank %d: FTB_Publish() error for Iteration %d\n", rank, i);

pexit:
            clock_gettime(CLOCK_MONOTONIC, &endt);
            delay += ( (endt.tv_sec - startt.tv_sec) * 1000 +       \
                       (endt.tv_nsec - startt.tv_nsec) / 1000000 );
        } else {

            fprintf(stdout, "Rank %d: Polling\n", rank);
            while ((ret = FTB_Poll_event(shandle, &revent)) == FTB_GOT_NO_EVENT);
            /* Assume that the message you receive is the right one */

            fprintf(stdout, "Rank %d: Publishing\n", rank);
            ret = FTB_Publish(chandle, s_event, NULL, &ehandle);
            if (ret != FTB_SUCCESS)
                fprintf(stdout, "Rank %d: FTB_Publish() error for Iteration %d\n", rank, i);
        }
    }

    FTB_Unsubscribe(&shandle);

    if (rank == 0) {
        delay = delay * 1000 / (niter * nprocs);
        fprintf(stdout, "NPROCS=%d,ITERATIONS=%d,DELAY= %.3f us\n", nprocs, niter, delay);
    }

    ret = 0;
    goto exit;

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

exit:
exit2:
    FTB_Disconnect(chandle);

exit1:
    return(ret);
}
