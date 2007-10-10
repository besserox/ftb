/* This file is automatically generated by the xmlparser. It will be 
used by the FTB framework to interpret components and events*/



/*Definition of event severities*/
/*
 * #define FTB_INFO  (1<<0)
#define FTB_FATAL (1<<1)
#define FTB_ERROR (1<<2)
#define FTB_PERF_WARNING (1<<3)
#define FTB_WARNING (1<<4)
*/
#define FTB_INFO  1
#define FTB_FATAL 2
#define FTB_ERROR 3
#define FTB_PERF_WARNING 4
#define FTB_WARNING 5

#define FTB_TOTAL_SEVERITY  5


/*Definition of component categories*/
#define FTB_EXAMPLES  2
#define MPI 3

#define FTB_TOTAL_COMP_CAT  3

#define FTB_WATCHDOG     2
#define SIMPLE           3
#define NOTIFY_LOGGER    4
#define POLLING_LOGGER   5
#define PINGPONG         6
#define MULTICOMP_COMP1  7
#define MULTICOMP_COMP2  8
#define MULTICOMP_COMP3  9
#define MPI_EXAMPLE      10

#define FTB_TOTAL_COMP      10

/*Definition of event name*/
#define WATCH_DOG_EVENT_CODE        1
#define SIMPLE_EVENT_CODE           2
#define PINPONG_EVENT_SRV_CODE      3
#define PINPONG_EVENT_CLI_CODE      4
#define TEST_EVENT_1_CODE           5
#define TEST_EVENT_2_CODE           6
#define TEST_EVENT_3_CODE           7
#define MPI_SIMPLE_EVENT_CODE       8

#define FTB_EVENT_DEF_TOTAL_THROW_EVENTS 8

/*Definition of event categories*/
#define GENERAL             1

FTBM_severity_entry_t FTB_severity_table[FTB_TOTAL_SEVERITY]={
    {"FTB_INFO", FTB_INFO},
    {"FTB_FATAL", FTB_FATAL},
    {"FTB_ERROR", FTB_ERROR},
    {"FTB_PERF_WARNING", FTB_PERF_WARNING},
    {"FTB_WARNING", FTB_WARNING}
};

FTBM_throw_event_entry_t FTB_event_table[FTB_EVENT_DEF_TOTAL_THROW_EVENTS] = {
{"WATCH_DOG_EVENT",WATCH_DOG_EVENT_CODE, FTB_INFO, GENERAL, FTB_EXAMPLES, FTB_WATCHDOG},
{"SIMPLE_EVENT", SIMPLE_EVENT_CODE, FTB_INFO, GENERAL, FTB_EXAMPLES, SIMPLE}, 
{"PINPONG_EVENT_SRV", PINPONG_EVENT_SRV_CODE, FTB_INFO, GENERAL, FTB_EXAMPLES, PINGPONG}, 
{"PINPONG_EVENT_CLI", PINPONG_EVENT_CLI_CODE, FTB_INFO, GENERAL, FTB_EXAMPLES, PINGPONG}, 
{"TEST_EVENT_1", TEST_EVENT_1_CODE, FTB_INFO, GENERAL, FTB_EXAMPLES, MULTICOMP_COMP1}, 
{"TEST_EVENT_2", TEST_EVENT_2_CODE, FTB_INFO, GENERAL, FTB_EXAMPLES, MULTICOMP_COMP2}, 
{"TEST_EVENT_3", TEST_EVENT_3_CODE, FTB_INFO, GENERAL, FTB_EXAMPLES, MULTICOMP_COMP3}, 
{"MPI_SIMPLE_EVENT", MPI_SIMPLE_EVENT_CODE, FTB_INFO, GENERAL, MPI, MPI_EXAMPLE}, 
};

FTBM_comp_cat_entry_t FTB_comp_cat_table[FTB_TOTAL_COMP_CAT]={
    {"FTB_EXAMPLES", FTB_EXAMPLES},
    {"MPI", MPI},
};

FTBM_comp_entry_t FTB_comp_table[FTB_TOTAL_COMP]={
    {"FTB_WATCHDOG",FTB_WATCHDOG},
    {"SIMPLE",SIMPLE},
    {"NOTIFY_LOGGER", NOTIFY_LOGGER},
    {"POLLING_LOGGER", POLLING_LOGGER}, 
    {"PINGPONG", PINGPONG},
    {"MULTICOMP_COMP1", MULTICOMP_COMP1},
    {"MULTICOMP_COMP2", MULTICOMP_COMP2},
    {"MULTICOMP_COMP3", MULTICOMP_COMP3},
    {"MPI_EXAMPLE", MPI_EXAMPLE},
};

char * FTB_severity_table_rev[FTB_TOTAL_SEVERITY+2] = {"", "FTB_INFO", "FTB_FATAL", "FTB_ERROR", "FTB_PERF_WARNING", "FTB_WARNING"};

char * FTB_comp_table_rev[FTB_TOTAL_COMP+2] = {"", "",  "FTB_WATCHDOG", "SIMPLE", "NOTIFY_LOGGER", "POLLING_LOGGER", 
            "PINGPONG", "MULTICOMP_COMP1", "MULTICOMP_COMP2", "MULTICOMP_COMP3", "MPI_EXAMPLE"};

char * FTB_comp_cat_table_rev[FTB_TOTAL_COMP_CAT+1] = {"", "", "FTB_EXAMPLES", "MPI"};

char * FTB_event_table_rev[FTB_EVENT_DEF_TOTAL_THROW_EVENTS+1] = {"","WATCH_DOG_EVENT",  "SIMPLE_EVENT", "PINPONG_EVENT_SRV",
        "PINPONG_EVENT_CLI", "TEST_EVENT_1", "TEST_EVENT_2", "TEST_EVENT_3", "MPI_SIMPLE_EVENT"};

