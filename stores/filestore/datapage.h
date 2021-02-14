/*
 * $Id: datapage.h,v 1.2 2002/07/31 07:59:08 jan Exp $
 *
 * Copyright (c) 2002 Jan Algermissen
 * See the file "COPYING" for copying permission.
 *
 */

/******************************************************************************
 *
 * Handles topics pages
 *
 *****************************************************************************/
#ifndef DATAPAGE_H
#define DATAPAGE_H

#include <tm.h>
#include "page.h"


GS_API(void) gs_datapage_init(GS_PAGE_T page, int n);
GS_API(int) gs_dataheadpage_get_pagecount(GS_PAGE_T headpage);
GS_API(void) gs_dataheadpage_set_pagecount(GS_PAGE_T headpage,int pagecount);
GS_API(int) gs_datapage_fits_data(GS_PAGE_T page, int len);
GS_API(int) gs_datapage_append_data(GS_PAGE_T page,const char *data,int len,gs_topic_t topic);
GS_API(int) gs_datapage_get_data(GS_PAGE_T page,int offset,const char **dpp);
 

#endif
