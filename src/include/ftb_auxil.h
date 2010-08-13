/***********************************************************************************/
/* FTB:ftb-info */
/* This file is part of FTB (Fault Tolerance Backplance) - the core of CIFTS
 * (Co-ordinated Infrastructure for Fault Tolerant Systems)
 *
 * See http://www.mcs.anl.gov/research/cifts for more information.
 * 	
 */
/* FTB:ftb-info */

/* FTB:ftb-fillin */
/* FTB_Version: 0.6.2
 * FTB_API_Version: 0.5
 * FTB_Heredity:FOSS_ORIG
 * FTB_License:BSD
 */
/* FTB:ftb-fillin */

/* FTB:ftb-bsd */
/* This software is licensed under BSD. See the file FTB/misc/license.BSD for
 * complete details on your rights to copy, modify, and use this software.
 */
/* FTB:ftb-bsd */
/***********************************************************************************/

/*
 * This file contains some auxillary functions useful for FTB
 * The functions in this file are independent of FTB
 */

#ifndef FTB_AUXIL_H
#define FTB_AUXIL_H

/* *INDENT-OFF* */
#ifdef __cplusplus
extern "C" {
#endif
/* *INDENT-ON* */


/* Remove the spaces from the start and end of string */
char *trim_string(const char *str);


void soft_trim(char **str);

/* Ensure that the last parameter in the below function call is NULL.
 * Also Ensure that str1 is the destination string, large enough to hold the
 * concatentation of all strings
 */
int concatenate_strings(char *str1, ...);


/*
 * Check it the string is composed of alphanumeric & underscore characters
 * only
 */
int check_alphanumeric_underscore_format(const char *str);

/* *INDENT-OFF* */
#ifdef __cplusplus
} /*extern "C"*/
#endif
/* *INDENT-ON* */

#endif
