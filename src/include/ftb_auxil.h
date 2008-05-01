/*
 * This file contains some auxillary functions useful for FTB
 * The functions in this file are independent of FTB
 */

#ifndef FTB_AUXIL_H
#define FTB_AUXIL_H


#ifdef __cplusplus
extern "C" {
#endif 

/*Remove the spaces from the start and end of string*/
char * trim_string (const char *str);


void soft_trim(char **str);

//Ensure that the last parameter in the below function call is NULL
//Ensure that str1 is the destination string, large enough to hold the
//concatentation of all strings
int concatenate_strings (char *str1, ...);

/*Check it the string is composed of alphanumeric & underscore characters
 * only 
 */
int check_alphanumeric_underscore_format(const char *str);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif
