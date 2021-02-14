/*
 * $Id: datapage.c,v 1.2 2002/07/31 07:59:07 jan Exp $
 *
 * Copyright (c) 2002 Jan Algermissen
 * See the file "COPYING" for copying permission.
 *
 */

/******************************************************************************
 *  
 *
 *
 * 
 *
 *
 *****************************************************************************/
#include "datapage.h"
#include "trace.h"

 



/*------------- HEADER PAGE STUFF -------------- */


#define FREESTART_OFS	12	
#define FREESTART_SIZE	4

#define HEADER_LEN	16	

#define PAGECOUNT_OFS	FREESTART_OFS	
#define PAGECOUNT_SIZE	FREESTART_SIZE	
/*

Header byte layout:
------------------------------------------------------------------
00 01 02 03   04 05 06 07   08 09 10 11  12 13 14 15   16 17 18 19
 P  A  G  E     page no.    CF -- --      freestart 
------------------------------------------------------------------
CF:    changed flag

(first 12 bytes defined in page.c)

*/

/* Record Layout:
 * +-------------------------+-----------------+-------------------+
 * | 4 bytes lpt-page number | 1 byte lpt-char | 7 bytes data info |
 * +-------------------------+-----------------+-------------------+
 * => 12 bytes record length
 */
#define RECORD_HEAD_LEN (4+4)	/* topic id and data len */	


void gs_datapage_init(GS_PAGE_T page, int n)
{
	int k = HEADER_LEN;
	gs_page_init(page,n);
	memcpy(page + FREESTART_OFS,&k,FREESTART_SIZE);
	gs_page_set_changedflag(page,1);
}
int _get_freestart(GS_PAGE_T page)
{
	int n;
	assert(page);
	memcpy(&n,page + FREESTART_OFS,FREESTART_SIZE);
	return n;
}
void _set_freestart(GS_PAGE_T page, int n)
{
	assert(page);
	memcpy(page + FREESTART_OFS,&n,FREESTART_SIZE);
}

int gs_dataheadpage_get_pagecount(GS_PAGE_T headpage)
{
	int n;
	assert(headpage);
	memcpy(&n,headpage + PAGECOUNT_OFS,PAGECOUNT_SIZE);
	assert(n >= 0);
	return n;
}
void gs_dataheadpage_set_pagecount(GS_PAGE_T headpage,int pagecount)
{
	assert(pagecount >= 0);
	assert(headpage);
	memcpy(headpage + PAGECOUNT_OFS,&pagecount,PAGECOUNT_SIZE);
	gs_page_set_changedflag(headpage,1);
}


int gs_datapage_fits_data(GS_PAGE_T page, int len)
{
	int fs;

	fs = _get_freestart(page);
	return ( (GS_PAGE_SIZE - fs) > (len+RECORD_HEAD_LEN+5)); /* TBD */
}
int gs_datapage_append_data(GS_PAGE_T page,const char *data, int len, gs_topic_t topic)
{
	int fs;
	fs = _get_freestart(page);
	assert( (GS_PAGE_SIZE - fs) > (len+RECORD_HEAD_LEN+5)); /* TBD */

	assert(data);
	assert(*data);
	assert(len > 0);

	memcpy(page+fs,&topic,4);	
	memcpy(page+fs+4,&len,4);	
	memcpy(page+fs+8,data,len);	

	_set_freestart(page,fs+(8+len));
	gs_page_set_changedflag(page,1);
	return fs;
}

int gs_datapage_get_data(GS_PAGE_T page,int offset, const char **dpp)
{
	int len;
	gs_topic_t n;

	memcpy(&n,page+offset,4);	
	memcpy(&len,page+offset+4,4);	
	*dpp = page+offset+8;
	GSTRACE(GS_MAIN_TRACE,"datapage_get_data: got _%s_\n" _ *dpp);
	assert(**dpp);
	assert(len > 0);
	return len;
}
