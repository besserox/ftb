#include <stdio.h>
#include <search.h>
#include <string.h>
#include "ftb_event_search.c"
int main(int argc, char *argv[])
{ 
    FTB_event_t *e = (void *)malloc (sizeof(FTB_event_t));
    FTBM_create_event_hash();
    if (FTBM_get_event_by_name(argv[1], e) != 0) {
        printf("%s", "FTBM_get_event_by_name didnt return success\n");
    }
    else {
        printf("I got severity as %d\n", e->severity); 
    }
        
    return 0;
}
