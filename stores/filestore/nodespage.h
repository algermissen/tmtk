/*
 * $Id: topicspage.h,v 1.3 2002/09/04 20:53:04 jan Exp $
 *
 * Copyright (c) 2002 Jan Algermissen
 * See the file "COPYING" for copying permission.
 *
 */
#ifndef NODESPAGE_H
#define NODESPAGE_H
#include <tm.h>
#include "page.h"

typedef struct {
        GS_PAGE_T page;         /* bucket may not exist beyond page access! */
	gs_topic_t topic;
	int lpt_pnum;
	char lpt_char;
	int data_pnum;
	int data_offset;
} topic_bucket_t;


GS_API(void) gs_topicspage_init(GS_PAGE_T page, int n);

GS_API(int) gs_topicsheadpage_get_topicmax(GS_PAGE_T headpage);
GS_API(int) gs_topicsheadpage_get_pagecount(GS_PAGE_T headpage);
GS_API(void) topicsheadpage_set_pagecount(GS_PAGE_T headpage,int pagecount);
GS_API(void) gs_topicsheadpage_set_topicmax(GS_PAGE_T headpage,int max);

GS_API(int) gs_topicspage_topic_fits(gs_topic_t topic,int pagecount);
GS_API(void) gs_topicspage_get_bucket(GS_PAGE_T page,gs_topic_t topic,
							topic_bucket_t *bp);
GS_API(void) gs_topicspage_set_bucket(GS_PAGE_T page,topic_bucket_t *bp);
GS_API(int)  gs_topicspage_num(gs_topic_t topic); 
 

#endif
