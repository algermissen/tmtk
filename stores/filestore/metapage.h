/*
 * $Id: metapage.h,v 1.1 2002/08/06 22:08:47 jan Exp $
 *
 * Copyright (c) 2002 Jan Algermissen
 * See the file "COPYING" for copying permission.
 *
 */

#ifndef TM_METAPAGE_H
#define TM_METAPAGE_H

#include <tm.h>
#include "page.h"

/*
#include "assertion.h"
*/

TM_API(void) tm_metapage_add_modelname(TMPage page, const char *name);
TM_API(int) tm_metapage_get_number_of_models(TMPage page);
TM_API(const char *) tm_metapage_get_modelname(TMPage page, int i);

TM_API(TMTopic) tm_metapage_get_new_topic(TMPage page);
TM_API(void) tm_metapage_dispose_topic(TMPage page,TMTopic topic);

TM_API(int) gs_metaheadpage_add_pattern(TMPage headpage,TMTopic ptopic);
TM_API(int) gs_metaheadpage_get_patterns(TMPage page,TMTopic *patterns);
/*
TM_API(void) gs_metaheadpage_init(TM_PAGE_T headpage,struct assertion *ap);
TM_API(int) gs_metaheadpage_get_pattern_count(TM_PAGE_T headpage);
*/
 

#endif

