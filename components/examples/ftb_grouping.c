/**********************************************************************************/
/* This file is part of FTB (Fault Tolerance Backplance) - the core of CIFTS
 * (Co-ordinated Infrastructure for Fault Tolerant Systems)
 *
 * See http://www.mcs.anl.gov/research/cifts for more information.
 * 	
 */
/* This software is licensed under BSD. See the file FTB/misc/license.BSD for
 * complete details on your rights to copy, modify, and use this software.
 */
/*********************************************************************************/

/*
 * A sample MPI application.
 * This creates group based on the user-input group size and sends events to all-nodes in the group in an all-to-all manner.
 */

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include "mpi.h"
#include "libftb.h"

int main(int argc, char *argv[])
{
    FTB_client_handle_t handle;
    FTB_client_t cinfo;
    FTB_event_handle_t ehandle;
    FTB_subscribe_handle_t shandle;
    int i = 0, NUM_EVENTS = 0, GRP_SIZE = 0, k = 0;
    int rank, size, ret = 0, group_id = 0;
    double begin, end, delay;
    double min, max, avg;
    char new_client_name[16], new_sub_str[32] = "client_name=";
    FTB_event_info_t event_info[1] = { {"MPI_SIMPLE_EVENT", "INFO"} };

    if (getenv("NUM_EVENTS"))
        NUM_EVENTS = atoi(getenv("NUM_EVENTS"));
    else
        NUM_EVENTS = 100;

    if (getenv("GRP_SIZE"))
        GRP_SIZE = atoi(getenv("GRP_SIZE"));
    else
        GRP_SIZE = 1;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Barrier(MPI_COMM_WORLD);

    group_id = rank / GRP_SIZE;
    snprintf(new_client_name, 16, "%d", group_id);
    memset(&cinfo, 0, sizeof(cinfo));
    strcpy(cinfo.client_name, new_client_name);
    strcpy(cinfo.event_space, "FTB.MPI.EXAMPLE_MPI");
    strcpy(cinfo.client_schema_ver, "0.5");
    strcpy(cinfo.client_jobid, "43");
    strcpy(cinfo.client_subscription_style, "FTB_SUBSCRIPTION_POLLING");

    ret = FTB_Connect(&cinfo, &handle);
    if (ret != FTB_SUCCESS) {
        printf("FTB_Connect is not successful ret=%d\n", ret);
        exit(-1);
    }

    ret = FTB_Declare_publishable_events(handle, 0, event_info, 1);
    if (ret != FTB_SUCCESS) {
        printf("FTB_Declare_publishable_events failed ret=%d!\n", ret);
        exit(-1);
    }

    MPI_Barrier(MPI_COMM_WORLD);

    /*
     * Catch events thrown by my group
     */
    strcat(new_sub_str, new_client_name);
    ret = FTB_Subscribe(&shandle, handle, new_sub_str, NULL, NULL);
    if (ret != FTB_SUCCESS) {
        printf("FTB_Subscribe failed ret=%d!\n", ret);
        exit(-1);
    }

    sleep(5);

    begin = MPI_Wtime();
    k = 0;

    for (i = 0; i < NUM_EVENTS; i++) {
        FTB_Publish(handle, "MPI_SIMPLE_EVENT", NULL, &ehandle);
    }

    /*
     * Make sure there are equal number of components in each group, else
     * you will wait in a never-ending loop
     */
    while (k < (GRP_SIZE * NUM_EVENTS)) {
        ret = 0;
        FTB_receive_event_t caught_event;

        ret = FTB_Poll_event(shandle, &caught_event);
        if (ret != FTB_SUCCESS) {
            continue;
        }
        else {
            k++;
//       fprintf (stderr, "%d : Received event details: Event space=%s, Severity=%s, Event name=%s, Client name=%s, Hostname=%s, Seqnum=%d\n",rank, caught_event.event_space, caught_event.severity, caught_event.event_name, caught_event.client_name, caught_event.incoming_src.hostname, caught_event.seqnum);
        }
    }
    end = MPI_Wtime();
    delay = end - begin;
    MPI_Reduce(&delay, &max, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
    MPI_Reduce(&delay, &min, 1, MPI_DOUBLE, MPI_MIN, 0, MPI_COMM_WORLD);
    MPI_Reduce(&delay, &avg, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
    avg /= size;

    if (rank == 0) {
//      printf("Average Maximum Minumum Num_Event Grp_Size total_cluster\n");
        printf("%.5f %.5f %.5f %d %d %d\n", avg, max, min, NUM_EVENTS, GRP_SIZE, size);
    }
    FTB_Disconnect(handle);

    MPI_Finalize();


    return 0;
}
