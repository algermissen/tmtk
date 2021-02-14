/*
 * $Id: topicspage.c,v 1.3 2002/09/04 20:53:04 jan Exp $
 *
 * Copyright (c) 2002 Jan Algermissen
 * See the file "COPYING" for copying permission.
 *
 */
#include "topicspage.h"



/*------------- HEADER PAGE STUFF -------------- */

#define MAXNODE_OFS	12	
#define MAXNODE_SIZE	4

#define PAGECOUNT_OFS	16
#define PAGECOUNT_SIZE	4

#define HEADER_LEN	20	
/*

Header byte layout:
------------------------------------------------------------------
00 01 02 03   04 05 06 07   08 09 10 11  12 13 14 15   16 17 18 19
 P  A  G  E     page no.    CF -- --       maxtopic     page count    
------------------------------------------------------------------
CF:    changed flag

(first 12 bytes defined in page.c)

*/

/* Record Layout:
 * 0                         4                 8        12         16
 * +-------------------------+-----------------+-------------------+
 * | 4 bytes lpt-page number | 1 byte lpt-char | 8 bytes data info |
 * +-------------------------+-----------------+-------------------+
 * => 16 bytes record length
 */
#define RECORD_SIZE	16	


#define NODES_PER_PAGE ( (GS_PAGE_SIZE - HEADER_LEN) / RECORD_SIZE )
#define GET_PAGE(topic)	( ((topic) / NODES_PER_PAGE) + 2 )
#define GET_OFFSET(topic) \
	(HEADER_LEN + (  (((topic)-1) % NODES_PER_PAGE)   * RECORD_SIZE)   )

void gs_topicspage_init(GS_PAGE_T page, int n)
{
	gs_page_init(page,n);
}

int gs_topicsheadpage_get_topicmax(GS_PAGE_T headpage)
{
	int n;
	assert(headpage);
	memcpy(&n,headpage + MAXNODE_OFS,MAXNODE_SIZE);
	assert(n >= 0);
	return n;
}
void gs_topicsheadpage_set_topicmax(GS_PAGE_T headpage,int max)
{
	assert(max > 0);
	assert(headpage);
	memcpy(headpage + MAXNODE_OFS,&max,MAXNODE_SIZE);
	gs_page_set_changedflag(headpage,1);
}

int gs_topicsheadpage_get_pagecount(GS_PAGE_T headpage)
{
	int n;
	assert(headpage);
	memcpy(&n,headpage + PAGECOUNT_OFS,PAGECOUNT_SIZE);
	assert(n >= 0);
	return n;
}
void topicsheadpage_set_pagecount(GS_PAGE_T headpage,int pagecount)
{
	assert(pagecount >= 0);
	assert(headpage);
	memcpy(headpage + PAGECOUNT_OFS,&pagecount,PAGECOUNT_SIZE);
	gs_page_set_changedflag(headpage,1);
}
int gs_topicspage_topic_fits(gs_topic_t topic,int pagecount)
{
	return (pagecount >= GET_PAGE(topic) );
}
int gs_topicspage_num(gs_topic_t topic)
{
	return ( GET_PAGE(topic) );
}
void gs_topicspage_get_bucket(GS_PAGE_T page,gs_topic_t topic,topic_bucket_t *bp)
{
	int ofs;
	assert(page);
	assert(topic);
	assert(bp);

	ofs = GET_OFFSET(topic);
	bp->page = page;
	bp->topic = topic;
	memcpy(&(bp->lpt_pnum),page+ofs,4);
	memcpy(&(bp->lpt_char),page+ofs+4,1);
	memcpy(&(bp->data_pnum),page+ofs+8,4);
	memcpy(&(bp->data_offset),page+ofs+12,4);
}
void gs_topicspage_set_bucket(GS_PAGE_T page,topic_bucket_t *bp)
{
	int ofs;
	assert(page);
	assert(bp);
	assert(page == bp->page);

	ofs = GET_OFFSET(bp->topic);
	memcpy(page+ofs,  &(bp->lpt_pnum), 4);
	memcpy(page+ofs+4,&(bp->lpt_char),1);
	memcpy(page+ofs+8,&(bp->data_pnum),4);
	memcpy(page+ofs+12,&(bp->data_offset),4);

	gs_page_set_changedflag(page,1);
}

