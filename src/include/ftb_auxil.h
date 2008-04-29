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
void trim_string( char *, const char *str );

/*Check it the string is composed of alphanumeric & underscore characters only */
int check_alphanumeric_underscore_format(const char *str);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif
