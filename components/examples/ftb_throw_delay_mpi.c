#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include "mpi.h"

#include "libftb.h"
#include "ftb_event_def.h"
#include "ftb_throw_events.h"

char err_msg[FTB_MAX_ERRMSG_LEN];

int main (int argc, char *argv[])
{
    FTB_component_properties_t properties;
    FTB_client_handle_t handle;
    int i, count;
    int rank, size;
    double begin, end, delay;
    double min, max, avg;

    if (argc >= 2) {
       count = atoi(argv[1]);
    }
    else {
       count = 500;
    }

    printf("Begin\n");
    properties.catching_type = FTB_EVENT_CATCHING_NONE;
    properties.err_handling = FTB_ERR_HANDLE_NONE;

    printf("FTB_Init\n");
    FTB_Init(MPI, EXAMPLE_MPI, &properties, &handle);
    printf("FTB_Reg_throw\n");
    FTB_Reg_throw(handle, "MPI_SIMPLE_EVENT");

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
    printf("FTB_Finalize\n");
    FTB_Finalize(handle);

    printf("End\n");
    return 0;
}
