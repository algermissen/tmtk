/*
 * $Id: btreepage.c,v 1.1 2002/09/18 19:35:25 jan Exp $
 *
 * Copyright (c) 2002 Jan Algermissen
 * See the file "COPYING" for copying permission.
 *
 */
#include "btreepage.h"
#include "util.h"
#include "mem.h"
#include "trace.h"

/*------------- HEADER FOR HEADPAGE ------------------------- */

/* first 12 bytes as in page.c */

#define ROOT_OFS	12	
#define ROOT_SIZE	4

#define FREESTART_PNUM_OFS	16	
#define FREESTART_PNUM_SIZE	4

#define FREESTART_OFFSET_OFS	20
#define FREESTART_OFFSET_SIZE	4
/*
Header byte layout for headpage:
--------------------------------------------------------------------------------
00 01 02 03   04 05 06 07   08 09 10 11  12 13 14 15   16 17 18 19   20 21 22 23
 P  A  G  E     page no.    CF -- --      root num      free start    free start
                                                          pnum          offset
---------------------------------------------------------------------------------
CF:    changed flag
*/


/*------------- HEADER FOR NODE AND LEAF PAGES -------------- */

#define TYPE_OFS	12	
#define TYPE_SIZE	4

#define ENTRYNUM_OFS	16
#define ENTRYNUM_SIZE	4


#define NODEPAGE_HEADER_LEN	20	
#define LEAFPAGE_HEADER_LEN	20	
#define DATAPAGE_HEADER_LEN	20	/* arbitrary, but must be > 12 (header in page.c)*/
/*

Header byte layout for page:
------------------------------------------------------------------
00 01 02 03   04 05 06 07   08 09 10 11  12 13 14 15   16 17 18 19
 P  A  G  E     page no.    CF -- --      page type    no. of entries   
------------------------------------------------------------------
CF:    changed flag

(first 12 bytes defined in page.c)

*/

/*

Record layout for player index leaf topics:

4 bytes key, 4 bytes length of chain, 4 bytes pnum, 4 bytes offset in page

*/

#define LEAF_RECORD_LEN	16
#define NODE_RECORD_LEN 8	




/* i-1 because we count from 0 in the page put from 1 in the algorithm !*/
#define NODEPAGE_OFFSET(i)	NODEPAGE_HEADER_LEN + (((i)-1) * (sizeof(gs_topic_t) + sizeof(int)))
#define LEAFPAGE_OFFSET(i)	(LEAFPAGE_HEADER_LEN + ( ((i)-1) * LEAF_RECORD_LEN ))

/* this depends on Prec = 4 and V = 4 ... -10 for safety */
/* #define LEAFPAGE_ORDER (((GS_PAGE_SIZE - LEAFPAGE_HEADER_LEN + 4) / 8) - 10) 
*/


/* -2 entries to allow overflow insert ! */
#define LEAFPAGE_ORDER (((GS_PAGE_SIZE - LEAFPAGE_HEADER_LEN) / LEAF_RECORD_LEN ) - 2)


/* -2 entries to allow overflow insert ! */
#define NODEPAGE_ORDER (((GS_PAGE_SIZE - NODEPAGE_HEADER_LEN) / NODE_RECORD_LEN ) - 5)

void gs_btreeheadpage_init(GS_PAGE_T page, int n)
{
	gs_page_init(page,n);
}


void gs_btreepage_init(GS_PAGE_T page, int n)
{
	gs_page_init(page,n);
}
void gs_btreeheadpage_set_root_num(GS_PAGE_T headpage,int n)
{
	assert(headpage);
	assert(n >= 2);
	memcpy(headpage + ROOT_OFS,&n,ROOT_SIZE);
	gs_page_set_changedflag(headpage,1);
}
	
int gs_btreeheadpage_get_root_num(GS_PAGE_T headpage)
{
	int n;
	assert(headpage);
	memcpy(&n,headpage + ROOT_OFS,ROOT_SIZE);
	return n;
}

void gs_btreeheadpage_set_list_freestart(GS_PAGE_T headpage,int pnum, int offset)
{
	assert(headpage);
	memcpy(headpage + FREESTART_PNUM_OFS,&pnum,FREESTART_PNUM_SIZE);
	memcpy(headpage + FREESTART_OFFSET_OFS,&offset,FREESTART_OFFSET_SIZE);
	gs_page_set_changedflag(headpage,1);
}

void gs_btreedatapage_init(GS_PAGE_T page, int n)
{
	gs_btreepage_set_type(page,BTREE_DATA);	
	gs_page_init(page,n);
}
int gs_btreedatapage_fits(int offset, int size)
{
	int remain;

	remain = (GS_PAGE_SIZE - DATAPAGE_HEADER_LEN) - offset;
	return (remain > 4+(size*4) + 20); /* 20 for safety (TBD) */
}

int gs_btreedatapage_put_list(GS_PAGE_T page, int offset,int size, int* array, int len)
{
	int new_freestart;
	assert(page);
	assert(gs_btreepage_get_type(page) == BTREE_DATA);	
	assert(size);
	assert(len >= 3);
	
	assert(size >= len);
	GSTRACE(GS_GRAPH_TRACE,"gs_btreedatapage_put_list(): page: %d offset: %d size: %d len: %d\n" _
		gs_page_get_number(page) _ offset _ size _ len );
	new_freestart = offset + 4 + (size*4);
	GSTRACE(GS_GRAPH_TRACE,"gs_btreedatapage_put_list(): new free start is %d\n" _ new_freestart);
	offset = offset + DATAPAGE_HEADER_LEN;
	GSTRACE(GS_GRAPH_TRACE,"gs_btreedatapage_put_list(): offset now %d\n" _ offset);
	assert(offset + 4 + size*4 <= GS_PAGE_SIZE);

	memcpy(page+offset,&size,4); /* the length of the allocated area in 4byte items */
	memcpy(page+offset+4,array,len*4);
	gs_page_set_changedflag(page,1);
	return new_freestart;
}
void gs_btreedatapage_get(GS_PAGE_T page,int offset ,int *array ,int len)
{
	int size;
	assert(page);
	assert(gs_btreepage_get_type(page) == BTREE_DATA);	

	offset = offset + DATAPAGE_HEADER_LEN;
	memcpy(&size,page+offset,4);
	memcpy(array,page+offset+4,len*4);
	GSTRACE(GS_GRAPH_TRACE,"check get for size: size id %d (len=%d)\n" _ size _ len);
	assert(size >= len);
}
int gs_btreedatapage_get_entry_size(GS_PAGE_T page,int offset)
{
	int size;
	assert(page);
	assert(gs_btreepage_get_type(page) == BTREE_DATA);	
	offset = offset + DATAPAGE_HEADER_LEN;
	memcpy(&size,page+offset,4);
	 GSTRACE(GS_GRAPH_TRACE,"gs_btreedatapage_get_entry_size(): size = %d, offset=%d, page=%d\n" _ size
			_ offset _ gs_page_get_number(page));
	assert(size);
	return size;

}
void gs_btreedatapage_add(GS_PAGE_T page,int offset, int len, int assertion_id )
{
	int size;
	assert(page);
	assert(gs_btreepage_get_type(page) == BTREE_DATA);	
	offset = offset + DATAPAGE_HEADER_LEN;
	memcpy(&size,page+offset,4);
	assert(size >= len);
	memcpy(page+offset+4+(len*4),&assertion_id,4);
	gs_page_set_changedflag(page,1);
	
}

	
void gs_btreeheadpage_get_list_freestart(GS_PAGE_T headpage, int *ppnum, int *poffset)
{
	assert(headpage);
	memcpy(ppnum,headpage + FREESTART_PNUM_OFS,FREESTART_PNUM_SIZE);
	memcpy(poffset,headpage + FREESTART_OFFSET_OFS,FREESTART_OFFSET_SIZE);
}

int gs_btreepage_get_type(GS_PAGE_T page)
{
	int n;
	assert(page);
	memcpy(&n,page + TYPE_OFS,TYPE_SIZE);
	assert(n == BTREE_LEAF || n == BTREE_NODE || n == BTREE_DATA );
	return n;
}
void gs_btreepage_set_type(GS_PAGE_T page,int n)
{
	assert(n == BTREE_LEAF || n == BTREE_NODE || n == BTREE_DATA );
	assert(page);
	memcpy(page + TYPE_OFS,&n,TYPE_SIZE);
	gs_page_set_changedflag(page,1);
}
int gs_btreepage_get_number_of_entries(GS_PAGE_T page)
{
	int n;
	assert(page);
	/*
	assert(gs_btreepage_get_type(page) == BTREE_NODE);
	*/
	memcpy(&n,page + ENTRYNUM_OFS,ENTRYNUM_SIZE);
	return n;
}
void gs_btreepage_set_number_of_entries(GS_PAGE_T page,int n)
{
	assert(page);
	memcpy(page + ENTRYNUM_OFS,&n,ENTRYNUM_SIZE);
	 gs_page_set_changedflag(page,1);
}


void gs_btreetopicpage_init(GS_PAGE_T page, int n)
{
	gs_page_init(page,n);
	gs_btreepage_set_type(page,BTREE_NODE);
	gs_page_set_changedflag(page,1);
}
gs_topic_t gs_btreetopicpage_get_key(GS_PAGE_T page, int i)
{
	int ofs;
	gs_topic_t key;
	assert(page);
	GSTRACE(GS_GRAPH_TRACE," gs_btreetopicpage_get_key(): i=%d, num entries = %d\n" _ i _
		gs_btreepage_get_number_of_entries(page));
	/* note: i < num ! */
	assert(i > 0 && i < gs_btreepage_get_number_of_entries(page) );
	ofs = NODEPAGE_OFFSET(i);
	memcpy(&(key),page+ofs+4,sizeof(gs_topic_t));
	GSTRACE(GS_GRAPH_TRACE,"get_key for pos %d is key %d\n" _ i _ key);
	return key;
}
int gs_btreetopicpage_get_ptr(GS_PAGE_T page, int i)
{
	int ofs,ptr;
	assert(page);
	/* note: i <= num ! */
	assert(i > 0 && i <= gs_btreepage_get_number_of_entries(page) );
	ofs = NODEPAGE_OFFSET(i);
	memcpy(&(ptr),page+ofs,sizeof(ptr));
	GSTRACE(GS_GRAPH_TRACE,"get_ptr for pos %d is pnum %d\n" _ i _ ptr);
	return ptr;
}

int gs_btreetopicpage_is_full(GS_PAGE_T page)
{
	int q,max;
	assert(page);
	max = NODEPAGE_ORDER;
	q = gs_btreepage_get_number_of_entries(page);

	return (q >= max);	
}
void gs_btreetopicpage_insert_first_triple(GS_PAGE_T page,int left_num,gs_topic_t key,int right_num)
{
	int ofs;
	assert(page);
	
	GSTRACE(GS_GRAPH_TRACE,"first triple: left: %d, key: %d, right: %d\n" _ left_num _ key _ right_num);
	ofs = NODEPAGE_OFFSET(1);
	memcpy(page+ofs,&left_num,4);
	memcpy(page+ofs+4,&key,sizeof(gs_topic_t));
	memcpy(page+ofs+8,&right_num,4);
	gs_btreepage_set_number_of_entries(page,2);
	gs_page_set_changedflag(page,1);
}
void gs_btreetopicpage_print(GS_PAGE_T page)
{
	int i,q,ofs;
	int P;
	assert(page);
	assert(gs_btreepage_get_number_of_entries(page) >= 2);
	assert(gs_btreepage_get_type(page) == BTREE_NODE );
	q = gs_btreepage_get_number_of_entries(page);

	GSTRACE(GS_GRAPH_TRACE,"NODEPAGE: [");

	for(i = 1; i<q; i++)
	{
		gs_topic_t K;
		int ofs = NODEPAGE_OFFSET(i);
		memcpy(&(P),page+ofs,4);
		memcpy(&(K),page+ofs+4,4);

		GSTRACE(GS_GRAPH_TRACE,"(%2d|%4d)" _ P _ K );
	}
	ofs = NODEPAGE_OFFSET(q);
	memcpy(&(P),page+ofs,4);
	GSTRACE(GS_GRAPH_TRACE," (%2d  )]\n" _ P);
	

}
void gs_btreetopicpage_insert_entry(GS_PAGE_T page,int N,gs_topic_t key,int pnum)
{
	int q,ofs,n;
	assert(page);
	assert(key);
	assert(pnum);
	assert(gs_btreepage_get_number_of_entries(page) >= 2);
	assert(gs_btreepage_get_type(page) == BTREE_NODE );
	gs_btreetopicpage_print(page);
	/* there are number_of_entries -1 keys in the page, so we use it or start */
	q = gs_btreepage_get_number_of_entries(page);

	GSTRACE(GS_GRAPH_TRACE,"topicpage_insert_entry(): page %d, q = %d  N=%d key=%d, pnum=%d\n" _
			gs_page_get_number(page) _ q _ N _ key _ pnum);

	ofs = NODEPAGE_OFFSET(q);
	memcpy(&(n),page+ofs,4);
	while(n != N)
	{
		/*
		gs_topic_t k;
		memcpy(&(k),page+ofs,sizeof(gs_topic_t));
		if(k < key)
		{
			int n;
			memcpy(&(n),page+ofs-4,4);
			assert(n == N);
			break;
		}
		*/
	
		memcpy(page+ofs+4,page+ofs-4,8);
		ofs -= 8;
		assert(ofs >= 0);
		memcpy(&(n),page+ofs,4);
	}
	if(ofs != NODEPAGE_OFFSET(q))
	{
		gs_topic_t k;
		memcpy(&(k),page+ofs+4,sizeof(gs_topic_t));
		assert(k > key);
	}
	memcpy(page+ofs+4,&key,sizeof(gs_topic_t));
	memcpy(page+ofs+8,&pnum,4);
	gs_btreepage_set_number_of_entries(page,q+1);
	gs_page_set_changedflag(page,1);
	

}

int gs_btreeleafpage_order(void)
{
	return LEAFPAGE_ORDER;
}

void gs_btreeleafpage_init(GS_PAGE_T page, int n)
{
	gs_page_init(page,n);
	gs_btreepage_set_type(page,BTREE_LEAF);
	gs_page_set_changedflag(page,1);
}


/*
GS_API(void) gs_btreeleafpage_create_entry(GS_PAGE_T page, gs_topic_t key, int ptr);
GS_API(void) gs_btreeleafpage_update_entry(GS_PAGE_T page, gs_btreeleaf_bucket_t *bp);
*/


int gs_btreeleafpage_find_entry(GS_PAGE_T page, gs_topic_t key,gs_btreeleaf_bucket_t *bp)
{
	int i,q;	
	assert(page);
	q = gs_btreepage_get_number_of_entries(page);
	/* hwy < q ??? */


/* TBD: <q because of page ptr....but when to store that, better in head ? ?? */

	for(i = 1; i <= q; i++)
	{
		int ofs;
		ofs = LEAFPAGE_OFFSET(i);	
		if(memcmp(&(key),page+ofs,sizeof(gs_topic_t)) == 0)
		{
			bp->key = key;
			bp->pos = i;
			memcpy(&(bp->len),page+ofs+sizeof(gs_topic_t),4);
			memcpy(&(bp->pnum),page+ofs+8,4);
			memcpy(&(bp->offset),page+ofs+12,4);
			return 1;
		}
	}
	/* not found */
	return 0;
}
int gs_btreeleafpage_is_full(GS_PAGE_T page)
{
	int q,max;
	assert(page);
	max = LEAFPAGE_ORDER;
	q = gs_btreepage_get_number_of_entries(page);

	return (q >= max);	
}

void gs_btreeleafpage_create_entry(GS_PAGE_T page, gs_topic_t key, int assertion_id)
{
	int i,q;	
	int ofs;
	const int ONE = 1;
	assert(page);
	/* over insertion is ok 
	assert(! gs_btreeleafpage_is_full(page));
	*/
	if(gs_btreeleafpage_is_full(page))
	{
		GSTRACE(GS_GRAPH_TRACE,"|||||||| OVERINSERTION FOR PAGE with key: %d, a_id: %d\n" _
				key _ assertion_id);
	}

	q = gs_btreepage_get_number_of_entries(page);
	
	/* TBD: next ptr ??? */
	if(q == 0)
	{
		ofs = LEAFPAGE_OFFSET(1);
		memcpy(page+ofs,&key,sizeof(gs_topic_t));
		memcpy(page+ofs+4,&ONE,4);
		memcpy(page+ofs+8,&assertion_id,4); /* pnum */
		bzero(page+ofs+12,4); /* offset */
		gs_btreepage_set_number_of_entries(page,1);		
		gs_page_set_changedflag(page,1);
		GSTRACE(GS_GRAPH_TRACE,"(q == 0) created entry at pos 1 for player=%d\n" _ key);
		return;
	}
	
	for(i = q+1; i > 1 ; i--)
	{
		int ofs;
		gs_topic_t k;
		ofs = LEAFPAGE_OFFSET(i-1);	
		memcpy(&(k),page+ofs,sizeof(gs_topic_t));
		if(k < key)
			break;
	
		memcpy(page+ofs+16 /* TBD */,page+ofs,16);
		GSTRACE(GS_GRAPH_TRACE,"moving entry\n");
	}
	
	/* now i is insert position */
	ofs = LEAFPAGE_OFFSET(i);
	memcpy(page+ofs,&key,sizeof(gs_topic_t));
	memcpy(page+ofs+4,&ONE,4);
	memcpy(page+ofs+8,&assertion_id,4);
	bzero(page+ofs+12,4); /* offset */
	gs_btreepage_set_number_of_entries(page,q+1);		
	gs_page_set_changedflag(page,1);
	GSTRACE(GS_GRAPH_TRACE,"(q == %d) created entry at pos %d for player=%d\n" _ q _ i _ key);
}

void gs_btreeleafpage_update_entry(GS_PAGE_T page,gs_btreeleaf_bucket_t *bp)
{
	int k;	
	int ofs;
	assert(page);

	ofs = LEAFPAGE_OFFSET(bp->pos);	
	memcpy(&k,page+ofs,sizeof(gs_topic_t) );
	assert(k == bp->key);

	assert(bp->len > 1);
	memcpy(page+ofs+4,&(bp->len),4);
	memcpy(page+ofs+8,&(bp->pnum),4);
	memcpy(page+ofs+12,&(bp->offset),4);
	gs_page_set_changedflag(page,1);
}

gs_topic_t gs_btreeleafpage_copy_remaining(GS_PAGE_T to, GS_PAGE_T from, int start)
{
	int i,j,ofs;
	gs_topic_t Kj;
	assert(to);
	assert(from);
	assert(start == ceil( (LEAFPAGE_ORDER + 1)/2 ) + 1);
	ofs = LEAFPAGE_OFFSET(start-1);
	memcpy(&(Kj),from+ofs,4);

	for(i = start,j=1; i <= LEAFPAGE_ORDER+1; i++,j++)
	{
		int from_ofs,to_ofs;
		from_ofs = LEAFPAGE_OFFSET(i);
		to_ofs   = LEAFPAGE_OFFSET(j);
		memcpy(to+to_ofs,from+from_ofs,LEAF_RECORD_LEN);
		GSTRACE(GS_GRAPH_TRACE,"copied entry %d\n" _ i );
	}
	GSTRACE(GS_GRAPH_TRACE,"done\n");
	gs_btreepage_set_number_of_entries(to,j);
	gs_page_set_changedflag(to,1);
	GSTRACE(GS_GRAPH_TRACE,"done2\n");
	return Kj;
}
