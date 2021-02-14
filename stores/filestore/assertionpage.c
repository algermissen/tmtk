/*
 * $Id: assertionpage.c,v 1.4 2002/09/11 22:03:36 jan Exp $
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
#include <trace.h> 
#include "assertionpage.h"


/*------------- HEADER PAGE STUFF -------------- */

#define COUNT_OFS	12	
#define COUNT_SIZE	4

#define NMEMBER_OFS	16
#define NMEMBER_SIZE	4

#define HEADER_LEN	20	
/*

Header byte layout:
------------------------------------------------------------------
00 01 02 03   04 05 06 07   08 09 10 11  12 13 14 15   16 17 18 19
 P  A  G  E     page no.    CF -- --       count        nmember
------------------------------------------------------------------
CF:    changed flag

(first 12 bytes defined in page.c)

*/

/* Record Layout in data pages:
 * 0        4+(0*4)     4+(0*4)+4        4+((N-1)*4)  4+((N-1)*4)+4
 * +-----------+-----------+-----------+ ... +-----------+-----------+
 * | 4 bytes a | 4 bytes c | 4 bytes x |     | 4 bytes c | 4 bytes a |
 * +-----------+-----------+-----------+ ... +-----------+-----------+
 * 
 */
#define CALC_RECORD_SIZE(nm)	(4 + ((nm)*(4+4)) )

#define ASSERTIONS_PER_PAGE(N) \
  ((GS_PAGE_SIZE - HEADER_LEN)/CALC_RECORD_SIZE((N)) )

#define CALC_PAGE(i,N) ((i) / ASSERTIONS_PER_PAGE( (N) ) + 2 )
#define CALC_OFFSET(i,N) (HEADER_LEN + \
	  (  (((i)-1) % ASSERTIONS_PER_PAGE((N))) * CALC_RECORD_SIZE((N)) ) \
	)

void gs_assertionheadpage_init(GS_PAGE_T headpage,struct assertion *ap)
{
	int i;
	assert(headpage);
	assert(ap);

	assert(gs_assertionheadpage_get_count(headpage) == 0);

	gs_assertionheadpage_set_member_num(headpage,ap->nmember);
	/* roles must be sorted */
        for(i = 0; i < ap->nmember-1; i++)
                assert(ap->members[i].r_topic < ap->members[i+1].r_topic);

		
        for(i = 0; i < ap->nmember; i++)
	{
		int ofs = HEADER_LEN + (i*4);
		memcpy(headpage+ofs,&(ap->members[i].r_topic),4);
	}
	
	gs_page_set_changedflag(headpage,1);
}


int gs_assertionheadpage_get_count(GS_PAGE_T headpage)
{
	int n;
	assert(headpage);
	memcpy(&n,headpage + COUNT_OFS,COUNT_SIZE);
	assert(n >= 0);
	return n;
}
void gs_assertionheadpage_set_count(GS_PAGE_T headpage,int n)
{
	assert(n > 0);
	assert(headpage);
	/* altering noy yet supported, so must be 0 */
	memcpy(headpage + COUNT_OFS,&n,COUNT_SIZE);
	gs_page_set_changedflag(headpage,1);
}
int gs_assertionheadpage_get_member_num(GS_PAGE_T headpage)
{
	int n;
	assert(headpage);
	memcpy(&n,headpage + NMEMBER_OFS,NMEMBER_SIZE);
	assert(n >= 0);
	return n;
}
void gs_assertionheadpage_set_member_num(GS_PAGE_T headpage,int n)
{
	assert(n > 0);
	assert(headpage);
	assert( gs_assertionheadpage_get_member_num(headpage) == 0);
	memcpy(headpage + NMEMBER_OFS,&n,NMEMBER_SIZE);
	gs_page_set_changedflag(headpage,1);
}

int gs_assertionpage_calc_num(int i,int N)
{
	return ( CALC_PAGE(i,N) );
}
/*
for(i = 0; i < ap->nmember; i++)
        {
                int ofs = HEADER_LEN + (i*4);
                memcpy(headpage+ofs,&(ap->members[i].r_topic),4);
        }
*/
void gs_assertionpage_get_assertion(GS_PAGE_T page,GS_PAGE_T headpage,int n,struct assertion *ap)
{
	int ofs;
	int i;
	assert(page);
	assert(headpage);
	assert(gs_assertionheadpage_get_count(headpage) != 0);
	assert(ap);
	assert(n>=0);

	/* TBD: set p_topic form headpage */
	
	GSTRACE(GS_GRAPH_TRACE,"gs_assertionpage_get_assertion for n=%d , N=%d\n" _ n _ ap->nmember);

	ofs = CALC_OFFSET(n,ap->nmember);
	GSTRACE(GS_GRAPH_TRACE,"offset=%d\n" _ ofs);
	memcpy(&(ap->a_topic),page+ofs, 4);
	GSTRACE(GS_GRAPH_TRACE,"set atopic to %d\n" _ ap->a_topic);
	for(i=0; i < ap->nmember; i++)
	{
		int role_ofs = HEADER_LEN + (i*4);
		memcpy(&(ap->members[i].r_topic),headpage+role_ofs,4);
		memcpy(&(ap->members[i].c_topic),page+ofs+4+( i*8 ),4);
		memcpy(&(ap->members[i].x_topic),page+ofs+4+( i*8 )+4,4);
		GSTRACE(GS_GRAPH_TRACE,"set member: %d plays %d (c=%d) \n" _ ap->members[i].x_topic _ ap->members[i].r_topic _ ap->members[i].c_topic);
	}
	GSTRACE(GS_GRAPH_TRACE,"gs_assertionpage_get_assertion exit\n");
}

void gs_assertionpage_set_assertion(GS_PAGE_T page,int n,struct assertion *ap)
{
	int ofs;
	int i,N;
	assert(page);
	assert(ap);
	assert(n>=0);
	N = ap->nmember;

	ofs = CALC_OFFSET(n,N);
	memcpy(page+ofs, &(ap->a_topic), 4);
	GSTRACE(GS_GRAPH_TRACE,"set_assertion, atopic: %d\n" _ ap->a_topic);
	for(i=0; i < N; i++)
	{
		GSTRACE(GS_GRAPH_TRACE,"member %d\n" _ i);
		memcpy(page+ofs+4+( i*8 ),&(ap->members[i].c_topic),4);
		GSTRACE(GS_GRAPH_TRACE,"ctopic %d\n" _ ap->members[i].c_topic);
		assert( ap->members[i].x_topic );
		memcpy(page+ofs+4+( i*8 )+4,&(ap->members[i].x_topic),4);
		GSTRACE(GS_GRAPH_TRACE,"xtopic %d\n" _ ap->members[i].x_topic);
	}

	gs_page_set_changedflag(page,1);
}

int gs_assertionheadpage_get_roles(GS_PAGE_T headpage,gs_topic_t *roles)
{
        int cnt;
        int i;
        assert(headpage);
        assert(roles);
 
        cnt = gs_assertionheadpage_get_member_num(headpage);
        for(i=0; i < cnt; i++)
        {
                int ofs = HEADER_LEN + (i*4);
                memcpy(&(roles[i]),headpage + ofs,4);
        }
 
        return cnt;
}
