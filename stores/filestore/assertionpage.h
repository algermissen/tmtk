/*
 * $Id: assertionpage.h,v 1.3 2002/08/06 22:08:47 jan Exp $
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
#ifndef ASSERTIONPAGE_H
#define ASSERTIONPAGE_H

#include <tm.h>
#include "page.h"
#include "assertion.h"

GS_API(void) gs_assertionheadpage_init(GS_PAGE_T headpage,struct assertion *ap);
GS_API(int) gs_assertionheadpage_get_count(GS_PAGE_T headpage);
GS_API(void) gs_assertionheadpage_set_count(GS_PAGE_T headpage,int count);
GS_API(int) gs_assertionheadpage_get_member_num(GS_PAGE_T headpage);
GS_API(int) gs_assertionheadpage_get_roles(GS_PAGE_T headpage,gs_topic_t *roles);
GS_API(void) gs_assertionheadpage_set_member_num(GS_PAGE_T headpage,int n);

GS_API(int) gs_assertionpage_calc_num(int count,int nmembers);
GS_API(void) gs_assertionpage_set_assertion(GS_PAGE_T page,int n,struct assertion *ap);

GS_API(void) gs_assertionpage_get_assertion(GS_PAGE_T page,GS_PAGE_T headpage,int n,struct assertion *ap);
 

#endif

