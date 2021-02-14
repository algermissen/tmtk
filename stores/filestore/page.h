/*
 * $Id: page.h,v 1.3 2002/09/04 20:53:04 jan Exp $
 *
 * Copyright (c) 2002 Jan Algermissen
 * See the file "COPYING" for copying permission.
 *
 */

#ifndef TM_PAGE_H
#define TM_PAGE_H

#include <tm.h> 

#define GS_PAGE_SIZE 4096
#define TM_PAGE_SIZE 4096

/* this is used in the page 'subclasses' */
#define GS_PAGE_HEADER_LEN 12

/* TBD: unsigned char ? or a byte type ?? */
typedef char* GS_PAGE_T;
typedef char* TMPage;

/* functions of this type can be passed to the storage open functions */
typedef void(*GS_INITFUNC_T)(GS_PAGE_T,int);
typedef void(*TMPageInitFunction)(TMPage,int);

/* the page functions that equaly apply to all page 'subclasses' */
TM_API(void) gs_page_init(GS_PAGE_T page, int n);
TM_API(void) tm_page_init(TMPage page, int n);

TM_API(int)  gs_page_get_number(GS_PAGE_T page);
TM_API(void) gs_page_set_changedflag(GS_PAGE_T page, byte f);
TM_API(char) gs_page_get_changedflag(GS_PAGE_T page);

TM_API(void) tm_page_set_changedflag(TMPage page);
TM_API(void) tm_page_clear_changedflag(TMPage page);
TM_API(int) tm_page_changed(TMPage page);


#endif /* not defined PAGE_H */
