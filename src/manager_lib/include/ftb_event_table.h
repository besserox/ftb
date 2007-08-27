#ifndef FTB_EVENT_TABLE_H
#define FTB_EVENT_TABLE_H

typedef struct FTB_throw_event_t{
    char event_name_char[1024];
    FTB_event_name_t event_name;
    FTB_severity_t  severity;
    FTB_event_ctgy_t event_ctgy;
    FTB_comp_ctgy_t comp_ctgy;
    FTB_comp_t comp;
}FTB_throw_event_t;

#endif
