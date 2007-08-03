/****************************************************************************/
/* ZEPTOOS:zepto-info */
/*     This file is part of ZeptoOS: The Small Linux for Big Computers.
 *     See www.mcs.anl.gov/zeptoos for more information.
 */
/* ZEPTOOS:zepto-info */
/* */
/* ZEPTOOS:zepto-fillin */
/*     $Id: dummy.h,v 1.0 2007/07/18 14:10:45 qgao Exp $
 *     ZeptoOS_Version: 1.2
 *     ZeptoOS_Heredity: FOSS_ORIG
 *     ZeptoOS_License: GPL
 */
/* ZEPTOOS:zepto-fillin */
/* */
/* ZEPTOOS:zepto-gpl */
/*      Copyright: Argonne National Laboratory, Department of Energy,
 *                 and UChicago Argonne, LLC.  2004, 2005, 2006, 2007
 *      ZeptoOS License: GPL
 * 
 *      This software is free.  See the file ZeptoOS/misc/license.GPL
 *      for complete details on your rights to copy, modify, and use this
 *      software.
 */
/* ZEPTOOS:zepto-gpl */
/****************************************************************************/

#ifndef FTB_AGENT_H
#define FTB_AGENT_H

#include <stdio.h>
#include <stdlib.h>
#include "zoid_api.h"

/* START-ZOID-SCANNER ID=9 INIT=ftb_agent_init FINI=ftb_agent_fini */

int Agent_FTB_Init(const void *properties /* in:arr:size=+1 */,
                   size_t len /* in:obj */);

int Agent_FTB_Reg_throw(const void *event /* in:arr:size=+1 */,
                        size_t len /* in:obj */);

int Agent_FTB_Reg_catch_polling(const void *event_mask /* in:arr:size=+1 */,
                                size_t len /* in:obj */);

int Agent_FTB_Throw(const void *event /* in:arr:size=+1 */,
                    size_t len /* in:obj */,
                    const void *data /* in:arr:size=+1 */, 
                    size_t data_len /* in:obj */);

int Agent_FTB_Catch(void *event /* out:arr:size=+1 */,
                    size_t len /* in:obj */);

int Agent_FTB_Finalize(void);

#endif /* FTB_AGENT_H */
