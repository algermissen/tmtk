/*
 * $Id: lptpage.h,v 1.2 2002/07/31 07:59:08 jan Exp $
 *
 * Copyright (c) 2002 Jan Algermissen
 * See the file "COPYING" for copying permission.
 *
 */

#ifndef LPTPAGE_H
#define LPTPAGE_H

#include <tm.h> 
#include "page.h"


typedef struct {
	GS_PAGE_T page;		/* bucket may not exist beyond page access! */
	char      key;		/* character we belong to */
	gs_flag_t has_leaf;	/* indicates if bucket references a leaf */
	int       ref;		/* reference to child page or leaf */
	struct {
		int strlen;	/* len of string */
		const char *str;/* points into page NO \0 !! */
		topic_t ctopic;	/* constituted topic */
		topic_t itopic;	/* indicated topic (optional) */
	} leaf;
} lpt_bucket_t;

/* for convenience and readbility */
#define BUCKET_EMPTY(b) ((b).ref == 0)
#define BUCKET_HAS_LEAF(b) ((b).has_leaf)
#define BUCKET_HAS_CHILD(b) ( (!(b).has_leaf) && ((b).ref != 0) )
#define BUCKET_REF(b) ((b).ref)
#define BUCKET_LEAF_STRLEN(b) ((b).leaf.strlen)
#define BUCKET_LEAF_STR(b) ((b).leaf.str)
#define BUCKET_LEAF_CNODE(b) ((b).leaf.ctopic)
#define BUCKET_LEAF_INODE(b) ((b).leaf.itopic)
 
GS_API(void) gs_lptpage_init(GS_PAGE_T page, int n);
GS_API(void) gs_lptpage_set_parent_number(GS_PAGE_T page, int n);
GS_API(int)  gs_lptpage_get_parent_number(GS_PAGE_T page);
GS_API(void) gs_lptpage_set_freestart(GS_PAGE_T page, int n);
GS_API(int)  gs_lptpage_get_freestart(GS_PAGE_T page);
GS_API(void) gs_lptpage_set_parent_char(GS_PAGE_T page, char c);
GS_API(char) gs_lptpage_get_parent_char(GS_PAGE_T page);
GS_API(int)  gs_lptpage_get_skip(GS_PAGE_T page, char **sp);
GS_API(void) gs_lptpage_set_skip(GS_PAGE_T page, const char *s,int len);

GS_API(void) gs_lptpage_get_skipctopic(GS_PAGE_T page, gs_topic_t *np);
GS_API(void) gs_lptpage_set_skipctopic(GS_PAGE_T page, gs_topic_t n);

GS_API(void) gs_lptpage_set_skipitopic(GS_PAGE_T page, gs_topic_t n);
GS_API(void) gs_lptpage_get_skipitopic(GS_PAGE_T page, gs_topic_t *np);

GS_API(void) gs_lptpage_clear_bucket(GS_PAGE_T page,char key);
GS_API(void) gs_lptpage_set_bucket(GS_PAGE_T page,char key,lpt_bucket_t *bp);
GS_API(void) gs_lptpage_set_bucket_itopic(GS_PAGE_T page,char key,lpt_bucket_t *bp);
GS_API(void) gs_lptpage_set_bucket_ctopic(GS_PAGE_T page,char key,lpt_bucket_t *bp);
GS_API(void) gs_lptpage_get_bucket(GS_PAGE_T page,char key,lpt_bucket_t *bp);
GS_API(void) gs_lptpage_raw_insert(GS_PAGE_T page,const char *suffix, gs_topic_t ctopic, gs_topic_t itopic);



#endif
