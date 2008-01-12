#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include "mpi.h"

#include "libftb.h"
#include "ftb_mpi_mpi_example_events.h"

char err_msg[FTB_MAX_ERRMSG_LEN];

int main (int argc, char *argv[])
{
    FTB_client_handle_t handle;
    FTB_client_t cinfo;
    int i, count;
    int rank, size;
    double begin, end, delay;
    double min, max, avg;
    char err_msg[128];

    if (argc >= 2) {
       count = atoi(argv[1]);
    }
    else {
       count = 500;
    }

    printf("Begin\n");

    /* Create namespace and other attributes before calling FTB_Connect */
    strcpy(cinfo.event_space, "FTB.MPI.EXAMPLE_MPI");
    strcpy(cinfo.client_schema_ver, "0.5"); 
    strcpy(cinfo.client_name, "");
    strcpy(cinfo.client_jobid,"");
    strcpy(cinfo.client_subscription_style, "FTB_SUBSCRIPTION_NONE");
    
    printf("FTB_Connect\n");
    ret = FTB_Connect(&cinfo, &handle);
    if (ret != FTB_SUCCESS) {
        printf("FTB_Connect is not successful ret=%d\n", ret);
        exit(-1);
    }
    
    printf("FTB_Register_publishbale_events\n");
    FTB_Register_publishable_events(handle, ftb_mpi_mpi_example_events, FTB_MPI_MPI_EXAMPLE_TOTAL_EVENTS, err_msg);

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Barrier(MPI_COMM_WORLD);
    begin = MPI_Wtime();
    for (i=0;i<count;i++) {
        FTB_Publish_event(handle, "MPI_SIMPLE_EVENT", NULL, err_msg);
    }
    end = MPI_Wtime();
    delay = end-begin;
    MPI_Reduce(&delay, &max, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
    MPI_Reduce(&delay, &min, 1, MPI_DOUBLE, MPI_MIN, 0, MPI_COMM_WORLD);
    MPI_Reduce(&delay, &avg, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
    avg /= size;

    printf("%d: delay %.5f\n", rank, delay);
    if (rank == 0) {
        printf("***** AVG delay: %.5f for %d throws *****\n",avg,count);
        printf("***** MAX delay: %.5f for %d throws *****\n",max,count);
        printf("***** MIN delay: %.5f for %d throws *****\n",min,count);
    }

    MPI_Finalize();
    printf("FTB_Disconnect\n");
    FTB_Disconnect(handle);

    printf("End\n");
    return 0;
}
