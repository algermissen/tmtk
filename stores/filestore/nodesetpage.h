/*
 * $Id: btreepage.h,v 1.1 2002/09/18 19:35:25 jan Exp $
 *
 * Copyright (c) 2002 Jan Algermissen
 * See the file "COPYING" for copying permission.
 *
 */
#ifndef NODESETPAGE_H
#define NODESETPAGE_H

#include <tm.h>
 
#include "page.h"

GS_API(void) gs_topicsetpage_init(GS_PAGE_T page, int n);
GS_API(int) gs_topicsetpage_get_number_of_lastpage(GS_PAGE_T page);
GS_API(void) gs_topicsetpage_set_number_of_lastpage(GS_PAGE_T page,int n);
GS_API(int) gs_topicsetpage_fits(GS_PAGE_T page, int size);
GS_API(int) gs_topicsetpage_get_freestart(GS_PAGE_T page);
GS_API(void) gs_topicsetpage_set_freestart(GS_PAGE_T page,int n);
GS_API(int) gs_topicsetpage_insert(GS_PAGE_T page,int record_size,int len,gs_topic_t *topics);

GS_API(void) gs_topicsetpage_get_topics(GS_PAGE_T page,int offset,gs_topic_t **topics, int *len);
GS_API(int) gs_topicsetpage_add_topic(GS_PAGE_T page,int offset, gs_topic_t topic);
GS_API(int) gs_topicsetpage_would_fit_in_whole_page(int nbytes);
GS_API(void) gs_topicsetpage_MARK(GS_PAGE_T page,int offset);



GS_API(void) gs_topicsetpage_get(GS_PAGE_T datapage,int offset ,int *array ,int len);
GS_API(int) gs_topicsetpage_put_list(GS_PAGE_T page, int offset,int size, int* array, int len);
GS_API(int) gs_topicsetpage_get_entry_size(GS_PAGE_T page,int offset);
GS_API(void) gs_topicsetdatapage_add(GS_PAGE_T page,int offset, int len, int assertion_id );
GS_API(int) gs_topicsetpage_is_full(GS_PAGE_T page);



/*
GS_API(int) gs_btreepage_get_type(GS_PAGE_T page);
GS_API(void) gs_btreepage_set_type(GS_PAGE_T page,int n);
GS_API(int) gs_btreepage_get_number_of_entries(GS_PAGE_T page);
GS_API(void) gs_btreepage_set_number_of_entries(GS_PAGE_T page,int n);


GS_API(void) gs_btreeheadpage_init(GS_PAGE_T page, int n);
GS_API(void) gs_btreeheadpage_set_root_num(GS_PAGE_T headpage,int n);
GS_API(int) gs_btreeheadpage_get_root_num(GS_PAGE_T headpage);
GS_API(void) gs_btreeheadpage_set_list_freestart(GS_PAGE_T headpage,int pnum, int offset);
GS_API(void) gs_btreeheadpage_get_list_freestart(GS_PAGE_T headpage, int *ppnum, int *poffset);

GS_API(void) gs_btreedatapage_init(GS_PAGE_T page, int n);
GS_API(int) gs_btreedatapage_fits(int offset, int size);
GS_API(void) gs_btreedatapage_get(GS_PAGE_T datapage,int offset ,int *array ,int len);
GS_API(int) gs_btreedatapage_put_list(GS_PAGE_T page, int offset,int size, int* array, int len);
GS_API(int) gs_btreedatapage_get_entry_size(GS_PAGE_T page,int offset);
GS_API(void) gs_btreedatapage_add(GS_PAGE_T page,int offset, int len, int assertion_id );
GS_API(int) gs_btreetopicpage_is_full(GS_PAGE_T page);



GS_API(void) gs_btreetopicpage_init(GS_PAGE_T page, int n);
GS_API(gs_topic_t) gs_btreetopicpage_get_key(GS_PAGE_T page, int i);
GS_API(int) gs_btreetopicpage_get_ptr(GS_PAGE_T page, int i);

GS_API(void) gs_btreetopicpage_insert_first_triple(GS_PAGE_T page,int left_num,gs_topic_t key,int right_num);
GS_API(void) gs_btreetopicpage_insert_entry(GS_PAGE_T p,int N,gs_topic_t key,int pnum);


GS_API(int) gs_btreeleafpage_order(void);
GS_API(void) gs_btreeleafpage_init(GS_PAGE_T page, int n);
GS_API(int) gs_btreeleafpage_find_entry(GS_PAGE_T page, gs_topic_t key,gs_btreeleaf_bucket_t *bp);
GS_API(int) gs_btreeleafpage_is_full(GS_PAGE_T page);
GS_API(void) gs_btreeleafpage_create_entry(GS_PAGE_T page, gs_topic_t key, int ptr);
GS_API(void) gs_btreeleafpage_update_entry(GS_PAGE_T page, gs_btreeleaf_bucket_t *bp);
GS_API(gs_topic_t) gs_btreeleafpage_copy_remaining(GS_PAGE_T to, GS_PAGE_T from, int start);
*/
#endif

