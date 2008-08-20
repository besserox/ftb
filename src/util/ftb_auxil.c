#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include "ftb_auxil.h"

void soft_trim(char **str)
{
    int i = 0, j = 0;
    int length = strlen(*str);
    char *e;

    if (**str == '\0')
	return;

    e = *str + length - 1;

    while (*(*str + i) == ' ' || *(*str + i) == '\t')
	i++;
    *str = *str + i;
    j = length - i;

    while (j > 0 && (*e == ' ' || *e == '\t')) {
	j--;
	e--;
    }

    if (j < length - i)
	*(e + 1) = '\0';
}

char *trim_string(const char *str)
{
    int i = 0, j = 0;
    char *new_str = (char *) malloc(strlen(str) + 1);
    strcpy(new_str, str);
    while ((new_str[i] == ' ') || (new_str[i] == '\t'))
	i++;
    if (i > 0) {
	for (j = 0; j < strlen(new_str); j++)
	    new_str[j] = new_str[j + i];
	new_str[j] = '\0';
    }
    i = strlen(new_str) - 1;
    while ((new_str[i] == ' ') || (new_str[i] == '\t'))
	i--;
    if (i < (strlen(new_str) - 1))
	new_str[i + 1] = '\0';
    return new_str;
}

int concatenate_strings(char *str, ...)
{
    char *tempstr;

    va_list arguments;
    va_start(arguments, str);

    tempstr = va_arg(arguments, char *);
    strcpy(str, tempstr);

    while ((tempstr = va_arg(arguments, char *)) != NULL) {
	strcat(str, tempstr);
    }

    va_end(arguments);
    return 1;
}

int check_alphanumeric_underscore_format(const char *str)
{
    int i = 0;
    for (i = 0; i < strlen(str); i++) {
	if ((str[i] >= 'A' && str[i] <= 'Z') 
		|| (str[i] >= 'a' && str[i] <= 'z') 
		|| (str[i] == '_') 
		|| (str[i] >= '0' && str[i] <= '9'))
	    continue;
	else
	    return 0;
    }
    return 1;	/* Return value of '1' indicates that string is alphanumeric & underscore compliant */
}
