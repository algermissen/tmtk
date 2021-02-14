/*
 * $Id: page.c,v 1.3 2002/09/04 20:53:04 jan Exp $
 *
 * Copyright (c) 2002 Jan Algermissen
 * See the file "COPYING" for copying permission.
 *
 */
#include "page.h"
#include <tmtrace.h>

#include <assert.h>

/*
 * "____" is in bytes 0-3 in order to visualize page boundaries when
 * looking at the file with od
 */

#define NUM_OFS		4
#define NUM_SIZE	4


/* the change flag indicates page alteration */

#define CHGFLAG_OFS	8	
#define CHGFLAG_SIZE	1	

/* bytes 9 10 11 are empty to keep 4-byte alignment */
/*
 
Header byte layout:
------------------------------------------------------------------
00 01 02 03   04 05 06 07   08 09 10 11  12 13 14 15   16 17 18 19
 _  _  _  _     page no.    CF -- -- --      
------------------------------------------------------------------
CF:    changed flag
 
(first 12 bytes defined in page.c)
 
*/

void gs_page_init(TMPage page, int n)
{
	tm_page_init(page,n);
}
void tm_page_init(TMPage page, int n)
{
        assert(page);
        assert(n > 0);
	TMTRACE(TM_STORAGE_TRACE,"gs_page_init(): for page %d\n" _ n);
        bzero(page,TM_PAGE_SIZE);
        memcpy(page,"____",4);           	/* will allways be 4 bytes */
        memcpy(page+NUM_OFS,&n,NUM_SIZE);	/* set the page number */
}


int gs_page_get_number(GS_PAGE_T page)
{
	int n;
	assert(page);
	memcpy(&n,page + NUM_OFS,NUM_SIZE);
	assert(n > 0);
	return n;
}
void gs_page_set_changedflag(GS_PAGE_T page, byte f)
{
	assert(page);
	assert(f == 0 || f == 1);
	memcpy(page + CHGFLAG_OFS,&f,CHGFLAG_SIZE);
}
char gs_page_get_changedflag(GS_PAGE_T page)
{
	byte f;	
	assert(page);
	memcpy(&f,page + CHGFLAG_OFS,CHGFLAG_SIZE);
	assert(f == 0 || f == 1);
	return f;
}
void tm_page_set_changedflag(TMPage page)
{
	gs_page_set_changedflag(page,1);
}
void tm_page_clear_changedflag(TMPage page)
{
	gs_page_set_changedflag(page,0);
}
int tm_page_changed(TMPage page)
{
	return (gs_page_get_changedflag(page) == 1);
}


#ifdef TEST

#include "test.h"

int main(int argc, char **args)
{
	GS_PAGE_T page;
	GS_TEST_T test;

	test = gs_test_new(__FILE__);
	page = GS_ALLOC(GS_PAGE_SIZE);
	assert(page);

	gs_page_init(page,1);

	GS_TEST(test, gs_page_get_number(page) == 1);
	GS_TEST(test, gs_page_get_number(page) == 0);


	GS_FREE(page);
	gs_test_delete(&test);

	return 0;
}


#endif

