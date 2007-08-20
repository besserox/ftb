/*
 * We need to move this data structure to a more logical position. 
 *
 */

#ifndef FTB_EVENT_DEF_H
#define FTB_EVENT_DEF_H

typedef struct FTB_comp_throw_event{
    char event_name_char[1024];
    FTB_event_name_t event_name;
    FTB_severity_t  severity;
    FTB_event_ctgy_t event_ctgy;
    FTB_comp_ctgy_t comp_ctgy;
    FTB_comp_t comp;
}ftb_comp_throw_event_t;

#endif 
