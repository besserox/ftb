#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include "ftb_auxil.h"

/*  --------------------------------------------------------------------------------------------  */
/*  Removes white space from the front and rear of oldStr and returns modified value in newStr.   */
/*  Due to use of strncpy, oldStr and newStr must point to different areas of memory.             */
/*  --------------------------------------------------------------------------------------------  */
void trim_string( char *newStr, const char *oldStr ) {

    if( newStr == NULL || oldStr == NULL || newStr == oldStr )
        return;

    int begin = 0,
        end   = strlen( oldStr );

    while( ( begin < end ) && isblank( oldStr[begin] ) ) 
        begin++;

    while( ( end > begin ) && isblank( oldStr[ end - 1 ] ) ) 
        end--;

    strncpy( newStr, &oldStr[begin], end - begin );

    newStr[ end - begin ] = '\0';

}

/*  --------------------------------------------------------------------------------------------  */
/*  Return 1 (true) if each character is alpha-numeric or the underscore, 0 (false) otherwise.    */
/*  --------------------------------------------------------------------------------------------  */
int check_alphanumeric_underscore_format(const char *str)
{
    int i=0; 
    for (i=0; i<strlen(str); i++)
        if( ! isalnum( str[i] ) && ( str[i] != '_' ) )
            return(0);

    return(1);
}

