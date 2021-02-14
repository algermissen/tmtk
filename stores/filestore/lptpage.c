/*
 * $Id: lptpage.c,v 1.2 2002/07/31 07:59:08 jan Exp $
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
#include "lptpage.h"
#include "trace.h"


#define PCHAR_OFS	9	
#define PCHAR_SIZE	1	

#define PNUM_OFS	12	
#define PNUM_SIZE	4	

#define SKIPLEN_OFS	16	
#define SKIPLEN_SIZE	4	

#define SKIPCNODE_OFS	20	
#define SKIPCNODE_SIZE	4	

#define SKIPINODE_OFS	24	
#define SKIPINODE_SIZE	4	

#define FREESTART_OFS	28	
#define FREESTART_SIZE	4	

#define HEADER_LEN	32	/* depends on the above */
#define SKIP_MAXLEN	500	/* arbitrary */

/* bucket related things */

#define LPT_LOWKEY	'!'
#define LPT_HIGHKEY	'~'
#define NKEYS		(LPT_HIGHKEY - LPT_LOWKEY + 1 )
#define IS_VALID_KEY(k)	((k) >= LPT_LOWKEY && (k) <= LPT_HIGHKEY )
#define FLAG_SIZE	1	/* a byte indicating if we are a leaf */
#define REF_SIZE	4 	/* bucket ref to next page (pagenr) */
#define BUCKET_SIZE 	(FLAG_SIZE + REF_SIZE) 


#define BUCKETS_OFS	(HEADER_LEN + SKIP_MAXLEN)
#define LEAVES_OFS	(BUCKETS_OFS + (NKEYS * BUCKET_SIZE))

#define BUCKET_OFFSET(key) (BUCKETS_OFS + (((key)-LPT_LOWKEY) * BUCKET_SIZE))

#define CLEAR_SKIP(p) bzero((p)+HEADER_LEN,SKIP_MAXLEN)
#define CLEAR_BUCKET(p,k) bzero((p)+BUCKET_OFFSET( (k) ),BUCKET_SIZE)

/*

Page layout:

 <--- HEADER_SIZE ---> <----------------- SKIP_MAXLEN ---------------------->
+----------------------------------------------------------------------------+
| HEADER              |  SKIP                                                |
+-------------+-------------+-------------+----------------------------------+
| flag | ref  | flag | ref  | flag | ref  |  ....more buckets...             |
+-------------+-------------+-------------+----------------------------------+
|                             ...more buckets...                             |
+-------------+-------------+-------------+----------------------------------+
| flag | ref  | flag | ref  | flag | ref  | space for leaves until end....   |
+-------------+-------------+-------------+----------------------------------+
|                           ...leaves...                                     |
+----------------------------------------------------------------------------+
|                           ...leaves...                                     |
+----------------------------------------------------------------------------+

Note: If leaves cause a page overflow, goose aborts. I'll implement a
      solution once I have some practical experience about this issue.


Header byte layout:
------------------------------------------------------------------
00 01 02 03   04 05 06 07   08 09 10 11  12 13 14 15   16 17 18 19
 _  _  _  _     page no.    CF PC --     parent num     skip len 
------------------------------------------------------------------
20 21 22 23   24 25 26 27   28 29 30 31
 skip ctopic    skip itopic    freestart
------------------------------------------------------------------


PC:    parent char (where we are linked from)
CF:    changed flag


*/

void gs_lptpage_init(GS_PAGE_T page, int n)
{
	gs_page_init(page,n);
	gs_lptpage_set_freestart(page,LEAVES_OFS);
	gs_page_set_changedflag(page,1);
}
/*
 * Get the number of the page.
 */
/*
int gs_lptpage_get_number(GS_PAGE_T page)
{
	int n;
	assert(page);
	memcpy(&n,page + NUM_OFS,NUM_SIZE);
	assert(n > 0);
	return n;
}
*/
/*
 * Set the number of the page's parent.
 */
void gs_lptpage_set_parent_number(GS_PAGE_T page, int n)
{
	assert(page);
	assert(n > 0);
	memcpy(page + PNUM_OFS,&n,PNUM_SIZE);
	GSTRACE(GS_LPTPAGE_TRACE,"set parent number to %d in %d\n" _
		n _ gs_page_get_number(page) );
	gs_page_set_changedflag(page,1);
}
/*
 * Get the number of the page's parent.
 */
int gs_lptpage_get_parent_number(GS_PAGE_T page)
{
	int n;
	assert(page);
	/* should not be called on root page !! */
	assert(gs_page_get_number(page) > 1);
	memcpy(&n,page + PNUM_OFS,PNUM_SIZE);
	assert(n || gs_page_get_number(page) == 1);
	return n;
}
/*
 * Set the character that is the key for the reference from the parent
 * to us. This is important to collect the character when walking up the
 * tree, building the locator.
 */
void gs_lptpage_set_parent_char(GS_PAGE_T page, char c)
{
	assert(page);
	assert(IS_VALID_KEY(c));
	memcpy(page + PCHAR_OFS,&c,PCHAR_SIZE);
	gs_page_set_changedflag(page,1);
}
/*
 * Get the character that is the key for the reference from the parent to
 * us.
 */
char gs_lptpage_get_parent_char(GS_PAGE_T page)
{
	char c;	
	assert(page);
	memcpy(&c,page + PCHAR_OFS,PCHAR_SIZE);
	assert(IS_VALID_KEY(c));
	return c;
}
/*
void gs_lptpage_set_changedflag(GS_PAGE_T page, byte f)
{
	assert(page);
	assert(f == 0 || f == 1);
	memcpy(page + CHGFLAG_OFS,&f,CHGFLAG_SIZE);
}
char gs_lptpage_get_changedflag(GS_PAGE_T page)
{
	byte f;	
	assert(page);
	memcpy(&f,page + CHGFLAG_OFS,CHGFLAG_SIZE);
	assert(f == 0 || f == 1);
	return f;
}
 */
/*
 * Set the position where free space for new leaves starts.
 */
void gs_lptpage_set_freestart(GS_PAGE_T page, int n)
{
	assert(page);
	assert(n >= LEAVES_OFS && n < GS_PAGE_SIZE);
	memcpy(page + FREESTART_OFS,&n,FREESTART_SIZE);
	gs_page_set_changedflag(page,1);
}
/*
 * Get the position where free space for new leaves starts.
 */
int gs_lptpage_get_freestart(GS_PAGE_T page)
{
	int n;
	assert(page);
	memcpy(&n,page + FREESTART_OFS,FREESTART_SIZE);
	assert(n >= LEAVES_OFS && n < GS_PAGE_SIZE);
	return n;
}
	
/*
 * Get the skip string and its length of the page.
 */
int gs_lptpage_get_skip(GS_PAGE_T page, char **sp)
{
	int len;
	GSTRACE(GS_LPTPAGE_TRACE,"lpt_page_get_skip() ENTER\n");
	assert(page);
	assert(sp);
	memcpy(&len,page + SKIPLEN_OFS,4);
	assert(len >= 0 && len < SKIP_MAXLEN);
	GSTRACE(GS_LPTPAGE_TRACE,"skiplen = %d\n" _ len);
	*sp = page + HEADER_LEN;
	return len;	
}

/*
 * Set the skiplen and its length. This involves complete erasing of old
 * skip content.
 */
void gs_lptpage_set_skip(GS_PAGE_T page, const char *s, int len)
{
	GSTRACE(GS_LPTPAGE_TRACE,"lpt_page_set_skip() ENTER\n");
	GSTRACE(GS_LPTPAGE_TRACE,"skiplen = %d\n" _ len);
	assert(page);
	assert(s);

	CLEAR_SKIP(page);
	memcpy(page + SKIPLEN_OFS,&len,4); /* TBD: replace '4' with sizeof */
	memcpy(page + HEADER_LEN,s,len);
	gs_page_set_changedflag(page,1);
}

/*
 * Clear the bucket (and the referenced leaf) that corresponds to the
 * given key.
 */
void gs_lptpage_clear_bucket(GS_PAGE_T page,char key)
{
	lpt_bucket_t b;
	assert(page);
	assert( IS_VALID_KEY(key) );

	/* we need the old bucket to clear the leaf if present */	
	gs_lptpage_get_bucket(page,key,&b);


	if(BUCKET_HAS_LEAF(b))
	{	/* clear leaf */
		bzero(page+b.ref,sizeof(gs_topic_t) /* ctopic */
				+sizeof(gs_topic_t) /* itopic */
				+sizeof(int)	   /* strlen */
				+b.leaf.strlen);   /* leaf string */
	}
	CLEAR_BUCKET(page,key);
	/* TBD: REORGANIZE LEAVES HERE, because of free start  */
	gs_page_set_changedflag(page,1);
}

void gs_lptpage_set_bucket_itopic(GS_PAGE_T page,char key,lpt_bucket_t *bp)
{
	GSTRACE(GS_LPTPAGE_TRACE,"lpt_set_bucket_itopic() %c ENTER\n" _ key);
	assert( IS_VALID_KEY(key) );
	assert(page);
	assert(bp);

	/* just check that bucket belongs to us! */
	assert(bp->page == page);
	assert(bp->key == key);	
	

	assert(bp->has_leaf);

	memcpy(page+(bp->ref)+4,&(bp->leaf.itopic),4);

	gs_page_set_changedflag(page,1);
}
/* FIXME: this function only introduced for word index in near CHANGED3 */
void gs_lptpage_set_bucket_ctopic(GS_PAGE_T page,char key,lpt_bucket_t *bp)
{
	GSTRACE(GS_LPTPAGE_TRACE,"lpt_set_bucket_ctopic() %c ENTER\n" _ key);
	assert( IS_VALID_KEY(key) );
	assert(page);
	assert(bp);

	/* just check that bucket belongs to us! */
	assert(bp->page == page);
	assert(bp->key == key);	
	

	assert(bp->has_leaf);

	/* this line changed from set_.._itopic() */
	memcpy(page+(bp->ref),&(bp->leaf.ctopic),4);

	gs_page_set_changedflag(page,1);
}

/* currently only used for seting page ref ! */
void gs_lptpage_set_bucket(GS_PAGE_T page,char key,lpt_bucket_t *bp)
{
	off_t o;
	GSTRACE(GS_LPTPAGE_TRACE,"lpt_bucket_set() %c ENTER\n" _ key);
	assert( IS_VALID_KEY(key) );
	assert(page);
	assert(bp);

	/* just check that bucket belongs to us! */
	assert(bp->page == page);
	assert(bp->key == key);	

	/* clear before writing new stuff */
	gs_lptpage_clear_bucket(page,key);

	GSTRACE(GS_LPTPAGE_TRACE,"set bucket for %c\n" _ key);
	
	o = BUCKET_OFFSET(key);
	
	memcpy(page+o,&bp->has_leaf,sizeof(gs_flag_t));	
	memcpy(page+o+sizeof(gs_flag_t),&bp->ref,sizeof(int));	


	assert(bp->has_leaf == 0);
	/* TBD: leaf handling ! ??????????????????????????? */

	GSTRACE(GS_LPTPAGE_TRACE,"lpt_bucket_set() EXIT\n");
	gs_page_set_changedflag(page,1);
}

/*
 * Get the bucket that corresponds to a certain key.
 */
void gs_lptpage_get_bucket(GS_PAGE_T page,char key,lpt_bucket_t *bp)
{
	off_t o;
	/*
	GSTRACE(GS_LPTPAGE_TRACE,"lpt_bucket_get() %c ENTER\n" _ key);
	*/
	assert(page);
	if(!IS_VALID_KEY(key) ) {
		fprintf(stderr,"key: %c (%d)\n",key,key);
	}
	assert( IS_VALID_KEY(key) );
	assert(bp);
	bzero(bp,sizeof(lpt_bucket_t));

	
	o = BUCKET_OFFSET(key);

	bp->page = page;
	bp->key = key;
	memcpy(&bp->has_leaf,page+o,sizeof(gs_flag_t));	
	memcpy(&bp->ref,page+o+sizeof(gs_flag_t),sizeof(int));	
	/*
	GSTRACE(GS_LPTPAGE_TRACE,"has_leaf = %d\n" _ bp->has_leaf);
	GSTRACE(GS_LPTPAGE_TRACE,"ref = %d\n" _ bp->ref);
	*/

	if(bp->has_leaf != 0)
	{
		/* TBD: replace '4's */
		memcpy(&bp->leaf.ctopic,page+bp->ref,4);	
		memcpy(&bp->leaf.itopic,page+bp->ref+4,4);	
		memcpy(&bp->leaf.strlen,page+bp->ref+8,4);	/* TBD */
		bp->leaf.str = page+bp->ref+8+4; /* TBD */	
		GSTRACE(GS_LPTPAGE_TRACE,"ctopic=%d\n" _ bp->leaf.ctopic);
		GSTRACE(GS_LPTPAGE_TRACE,"itopic=%d\n" _ bp->leaf.itopic);
		GSTRACE(GS_LPTPAGE_TRACE,"strlen=%d\n" _ bp->leaf.strlen);
		GSTRACE_STR(GS_LPTPAGE_TRACE, bp->leaf.str , bp->leaf.strlen);
	}
	/*
	GSTRACE(GS_LPTPAGE_TRACE,"lpt_bucket_get() EXIT\n");
	*/
}

/*
 * Get the ctopic that corresponds to skip string.
 *
 */
/* XX*/
void gs_lptpage_get_skipctopic(GS_PAGE_T page, gs_topic_t *np)
{
	assert(page);
	assert(np);
	*np = 0;
	memcpy(np,page+SKIPCNODE_OFS,4);
}
void gs_lptpage_set_skipctopic(GS_PAGE_T page, gs_topic_t n)
{
	assert(page);
	assert(n);
	memcpy(page+SKIPCNODE_OFS,&n,4);
	gs_page_set_changedflag(page,1);
	assert(n);
}
void gs_lptpage_get_skipitopic(GS_PAGE_T page, gs_topic_t *np)
{
	assert(page);
	assert(np);
	*np = 0;
	memcpy(np,page+SKIPINODE_OFS,4);
}
void gs_lptpage_set_skipitopic(GS_PAGE_T page, gs_topic_t n)
{
	assert(page);
	/* ----- may be 0 as long as we are abusing lptpage for index
	assert(n);
	*/
	memcpy(page+SKIPINODE_OFS,&n,4);
	gs_page_set_changedflag(page,1);
}


void gs_lptpage_raw_insert(GS_PAGE_T page,const char *suffix, gs_topic_t ctopic, gs_topic_t itopic)
{
	int required_size;
	off_t o;
	lpt_bucket_t b;
	off_t free_start;
	assert(page);
	assert(suffix);
	assert(ctopic);
	GSTRACE(GS_LPTPAGE_TRACE,"raw_insert ENTER for _%s_\n" _ suffix);

	if(*suffix == '\0')
	{
		GSTRACE(GS_LPTPAGE_TRACE,"inserting skip ctopic %d itopic %d\n" _ ctopic _ itopic);
		memcpy(page+SKIPCNODE_OFS,&ctopic,4);
		memcpy(page+SKIPINODE_OFS,&itopic,4);
		gs_page_set_changedflag(page,1);
		return;
	}


	/*
	 * It is very important that the bucket is really empty.
	 * This makes sure that no other leaf or child has the same
	 * prefix.
	 */
	gs_lptpage_get_bucket(page,suffix[0],&b);
	assert(BUCKET_EMPTY(b));

	free_start = gs_lptpage_get_freestart(page);
	GSTRACE(GS_LPTPAGE_TRACE,"old freestart: %d\n" _ free_start);

	/*
	 * CHECK FOR SPACE IN PAGE !!
	 * What action to take if space is not enough ??
	 *
	 * - new page just for data and attach to this one ?
	 * - move tree up and search for space ?
	 * ??
	 */
	required_size = strlen(suffix) + 12; /* 4+4+4  TBD */

	if((GS_PAGE_SIZE - free_start) < required_size)
	{
		assert(!"page overflow");
		exit(1);
	}

	/* now we are abusing the bucket. We should better use
	 * some vars for this task */
	b.has_leaf = 1;
	b.ref = free_start;
	b.leaf.ctopic = ctopic;
	b.leaf.itopic = itopic;
	b.leaf.str = suffix+1;
	b.leaf.strlen = strlen(suffix+1);

	/*
	gs_lptpage_set_bucket(page,suffix[0],&b);
	*/

	/* START SET BUCKET CODE */
	o = BUCKET_OFFSET(suffix[0]);
	
	memcpy(page+o,&b.has_leaf,sizeof(gs_flag_t));	
	memcpy(page+o+sizeof(gs_flag_t),&b.ref,sizeof(int));	
	/* END SET BUCKET CODE */



	memcpy(page+free_start,&(b.leaf.ctopic),4);
	memcpy(page+free_start+4,&(b.leaf.itopic),4);
	memcpy(page+free_start+8,&(b.leaf.strlen),4);
	memcpy(page+free_start+12,suffix+1,strlen(suffix+1));
	free_start += 12;
	free_start += strlen(suffix+1);
	GSTRACE(GS_LPTPAGE_TRACE,"new freestart: %d\n" _ free_start);
	gs_lptpage_set_freestart(page,free_start);

	GSTRACE(GS_LPTPAGE_TRACE,"raw_insert EXIT for _%s_\n" _ suffix);
	gs_page_set_changedflag(page,1);
}


#ifdef TEST
int main(int argc, char **args)
{
	int n;
	byte area[GS_PAGE_SIZE];
	GS_PAGE_T page = (GS_PAGE_T)area;
	
	gs_lptpage_init(page,7);	/* any number */	
	gs_lptpage_set_parent_number(page,88819);
	n = gs_lptpage_get_parent_number(page);
	assert(n == 88819);

	return 0;
}
#endif
