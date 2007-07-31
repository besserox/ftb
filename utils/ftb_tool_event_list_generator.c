#include "ftb.h"

#define FTB_KEY_WORD_VERSION   "VERSION"
#define FTB_OUTPUT_FILE_FORMAT "ftb_%s_event_list"
#define FTB_EVENT_DEFINITION_BUILD_IN "built_in"

static int line_number;
static char line[LINE_MAX];
static char event_definition[LINE_MAX];
static char output_filename[LINE_MAX];
static char output_filename_upper[LINE_MAX];
static FILE *fp_src;
static FILE *fp_list_h;
static FILE *fp_list_c;

static inline int is_built_in()
{
    return !strcmp(event_definition,FTB_EVENT_DEFINITION_BUILD_IN);
}

static void util_write_h_header(char *version)
{
    fprintf(fp_list_h,"/* Automatically generated.  Do not edit!  */\n");
    fprintf(fp_list_h,"#ifndef %s_H\n",output_filename_upper);
    fprintf(fp_list_h,"#define %s_H\n\n",output_filename_upper);
    fprintf(fp_list_h,"#include \"ftb.h\"\n\n");
    if (is_built_in()) {
        fprintf(fp_list_h,"#define FTB_EVENT_VERSION \"%s\"\n",version);
    }
    else {
        fprintf(fp_list_h,"int FTB_get_%s_event_by_id(uint32_t event_id, FTB_event_t *event);\n",event_definition);
    }
}

static void util_write_c_header()
{
    fprintf(fp_list_c,"/* Automatically generated.  Do not edit!  */\n");
    fprintf(fp_list_c,"#include \"%s.h\"\n\n",output_filename);
    if (is_built_in()) {
        fprintf(fp_list_c,"int FTB_get_event_by_id(uint32_t event_id, FTB_event_t *event)\n");
    }
    else {
        fprintf(fp_list_c,"int FTB_get_%s_event_by_id(uint32_t event_id, FTB_event_t *event)\n",event_definition);
    }
    fprintf(fp_list_c,"{\n\tevent->event_id = event_id;\n\tif(0){\n\t}\n");
}

static void util_write_h_tailer()
{
    fprintf(fp_list_h,"\n#endif\n");
}

static void util_write_c_tailer()
{
    fprintf(fp_list_c,"\telse {\n\t\treturn FTB_ERR_EVENT_NOT_FOUND;\n\t}\n\n");
    fprintf(fp_list_c,"\treturn 0;\n}\n");
}    

static void generate_event_entry(char *event_name, char *severity, uint32_t id)
{
    fprintf(fp_list_h,"#define %s 0x%08x\n",event_name, id);
    fprintf(fp_list_c,"\telse if (event_id == %s) {\n",event_name);
    fprintf(fp_list_c,"\t\tevent->severity = FTB_EVENT_SEVERITY_%s;\n",severity);
    fprintf(fp_list_c,"\t\tstrncpy(event->name,\"%s\",FTB_MAX_EVENT_NAME);\n\t}\n",event_name);
}

int parse_error()
{
    fprintf(stderr,"parse error at line %d:\n%s",line_number,line);
    exit(-1);
}

int main(int argc, char *argv[])
{
    uint32_t base_id = 1;
    int i;
    char temp1[LINE_MAX];
    char temp2[LINE_MAX];
    char version[FTB_MAX_EVENT_VERSION_NAME];

    if (argc < 2) {
        fprintf(stderr,"Usage: %s <event_file> [component_name='built_in'] [base_id = 1] \n",argv[0]);
        exit(-1);
    }

    if (argc >= 3) {
        strncpy(event_definition,argv[2], strlen(argv[2])+1);
        for (i=0;i<strlen(event_definition);i++) {
            event_definition[i] = tolower(event_definition[i]);
        }
    }
    else {
        strncpy(event_definition,FTB_EVENT_DEFINITION_BUILD_IN, strlen(FTB_EVENT_DEFINITION_BUILD_IN)+1);
    }

    if (argc >= 4) {
        base_id = atoi(argv[3]);
    }

    if (!is_built_in() && base_id <= FTB_MAX_BUILD_IN_EVENT_ID)  {
        fprintf(stderr,"event_id for additional event must start from at least %d\n",FTB_MAX_BUILD_IN_EVENT_ID+1);
        return -1;
    }

    sprintf(output_filename, FTB_OUTPUT_FILE_FORMAT, event_definition);
    strcpy(output_filename_upper, output_filename);
    for (i=0;i<strlen(output_filename_upper);i++) {
        output_filename_upper[i] = toupper(output_filename_upper[i]);
    }

    fp_src = fopen(argv[1],"r");
    if (fp_src == NULL) {
        fprintf(stderr, "Error open file %s\n",argv[1]);
        return -1;
    }

    if (is_built_in())  {
        while (fgets(line, LINE_MAX, fp_src) != NULL) {
            line_number++;
            if (isalpha(line[0]))
                break;
        }

        if (strncmp(line,FTB_KEY_WORD_VERSION,strlen(FTB_KEY_WORD_VERSION))!=0) {
            parse_error();
        }

        sscanf(line,"VERSION %s",version);

        printf("event list version is %s\n",version);
    }
    
    printf("generating file\n");

    sprintf(temp1,"%s.h",output_filename);
    fp_list_h = fopen(temp1,"w");
    if (fp_list_h == NULL) {
        fprintf(stderr, "Error open file %s\n",temp1);
        return -1;
    }
    
    sprintf(temp1,"%s.c",output_filename);
    fp_list_c = fopen(temp1,"w");
    if (fp_list_c == NULL) {
        fprintf(stderr, "Error open file %s\n",temp1);
        return -1;
    }

    
    util_write_h_header(version);
    util_write_c_header();

    while (fgets(line, LINE_MAX, fp_src) != NULL) {
        line_number++;
        if (!isalpha(line[0]))
            continue;
        sscanf(line, "%s %s", temp1, temp2);
        printf("generating event entry %s\n",temp1);
        for (i=0;i<strlen(temp1);i++) {
            temp1[i] = toupper(temp1[i]);
        }
        for (i=0;i<strlen(temp2);i++) {
            temp2[i] = toupper(temp2[i]);
        }
        if (strcmp(temp2,"INFO") != 0
         && strcmp(temp2,"PERFORMANCE") != 0 
         && strcmp(temp2,"WARNING") != 0
         && strcmp(temp2,"ERROR") != 0
         && strcmp(temp2,"FATAL") != 0
        ) {
            fprintf(stderr,"Unknown event severity: %s for event %s\n",temp2, temp1);
            continue;
        }
        generate_event_entry(temp1, temp2, base_id++);
    }
    
    util_write_h_tailer();
    util_write_c_tailer();
    fclose(fp_list_h);
    fclose(fp_list_c);
    fclose(fp_src);
    
    return 0;
}
