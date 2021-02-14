/*
 * $Id: metapage.c,v 1.1 2002/08/06 22:08:47 jan Exp $
 *
 * Copyright (c) 2002 Jan Algermissen
 * See the file "COPYING" for copying permission.
 *
 */
#include "metapage.h"
#include <tmtrace.h>

#include <assert.h>

#define MAX_MODEL_NAME_LENGTH 64 /* FIXME! (what about tm.h?) */

/*------------- HEADER PAGE STUFF -------------- */

#define COUNT_OFS	12	
#define COUNT_SIZE	4

#define HEADER_LEN	16	
/*

Header byte layout:
------------------------------------------------------------------
00 01 02 03   04 05 06 07   08 09 10 11  12 13 14 15   16 17 18 19
 P  A  G  E     page no.    CF -- --      count 
------------------------------------------------------------------
CF:    changed flag

(first 12 bytes defined in page.c)


IMPORTANT: There are three page types, but the header is the same for
           all of them.

The three page types are:

1) list of loaded models
2) topic max and freelist
3) assertion types (to find the files)

*/

static int _get_count(TMPage page)
{
	int n;
	assert(page);
	memcpy(&n,page + COUNT_OFS,COUNT_SIZE);
	assert(n >= 0);
	return n;
}
static void _set_count(TMPage page,int n)
{
	assert(n > 0);
	assert(page);
	memcpy(page + COUNT_OFS,&n,COUNT_SIZE);
	/* canged flag will be set by non static routines */
}

TMTopic tm_metapage_get_new_topic(TMPage page)
{
	int cnt;
	int i=0;
	int ofs;
	int topic;
	assert(page);
	TMTRACE(TM_STORAGE_TRACE,
		"tm_metapage_get_new_topic(): enter\n");


	/* First step: look if there is a free topic in the
	 * freelist.
	 * FIXME: max check!
	 */

	for(;;)
	{
		int n;
		ofs = HEADER_LEN + (i * 4);
		memcpy(&n,page+ofs,4);
		if(n == 0)
			break;
		i++;
	}

	/* if a topic is in freelist */
	if(i > 0)
	{
		int zero = 0;
		i--;
		ofs = HEADER_LEN + (i * 4);
			/* get topic */
		memcpy(&topic,page+ofs,4);
		assert(topic);
			/* store 0 */
		memcpy(page+ofs,&zero,4);
		gs_page_set_changedflag(page,1);

		TMTRACE(TM_STORAGE_TRACE,
			"tm_metapage_get_new_topic(): exit topic=%d\n" _ cnt); 
		return topic;
	}



	/* cnt is the maximum ID that is in use ! */
	cnt = _get_count(page);
	cnt++;
	/* FIXME: check < MAX */

	_set_count(page,cnt);
	gs_page_set_changedflag(page,1);


	TMTRACE(TM_STORAGE_TRACE,
			"tm_metapage_get_new_topic(): exit topic=%d\n" _ cnt); 
	return cnt;
}
void tm_metapage_dispose_topic(TMPage page,TMTopic topic)
{
	int i=0;
	int ofs;
	assert(page);

	TMTRACE(TM_STORAGE_TRACE,
		"tm_metapage_dispose_topic(): enter\n");

	for(;;)
	{
		int n;
		ofs = HEADER_LEN + (i * 4);
		memcpy(&n,page+ofs,4);
		if(n == 0)
			break;
		i++;
	}

	memcpy(page+ofs,&topic,4);

	gs_page_set_changedflag(page,1);

	TMTRACE(TM_STORAGE_TRACE,
			"tm_metapage_dispose_topic(): exit (length of freelist now %d\n" _ i+1); 

}


int gs_metaheadpage_add_pattern(TMPage headpage,TMTopic ptopic)
{
	int cnt;
	int ofs;
	assert(headpage);
	assert(ptopic);

	cnt = _get_count(headpage);
	ofs = HEADER_LEN + 4 + (cnt * 4);

	memcpy(headpage + ofs,&ptopic,4);
	cnt++;
	assert(cnt < 10); /* FIXME!!! */

	_set_count(headpage,cnt);
	gs_page_set_changedflag(headpage,1);
	return cnt;		
}
int gs_metaheadpage_get_patterns(TMPage headpage,TMTopic *patterns)
{
	int cnt;
	int ofs;
	int i;
	assert(headpage);
	assert(patterns);

	cnt = _get_count(headpage);
	for(i=0; i < cnt; i++)
	{
		ofs = HEADER_LEN + 4 + (i * 4);
		memcpy(&(patterns[i]),headpage + ofs,4);
	}
	
	return cnt;
}


void tm_metapage_add_modelname(TMPage page, const char *name)
{
	int cnt;
	int ofs;
	assert(page);
	assert(name);
	TMTRACE(TM_STORAGE_TRACE,
		"tm_metapage_add_modelname(): enter name=%s\n" _
		name);

	assert(strlen(name)+1 < MAX_MODEL_NAME_LENGTH);
	cnt = _get_count(page);
	ofs = HEADER_LEN + (cnt * MAX_MODEL_NAME_LENGTH);

	memcpy(page + ofs,name,strlen(name)+1);
	cnt++;
	/* FIXME: check < MAX */

	_set_count(page,cnt);
	gs_page_set_changedflag(page,1);
	TMTRACE(TM_STORAGE_TRACE,"tm_metapage_add_modelname(): exit\n"); 
}

int tm_metapage_get_number_of_models(TMPage page)
{
	int cnt;
	assert(page);

	cnt = _get_count(page);
	return cnt;		
}

const char * tm_metapage_get_modelname(TMPage page, int i)
{
	int cnt;
	int ofs;
	char *name;
	assert(page);
	TMTRACE(TM_STORAGE_TRACE,"tm_metapage_get_modelname(): enter n=%d\n" _
			i);

	cnt = _get_count(page);
	assert(i < cnt);
	ofs = HEADER_LEN + (i * MAX_MODEL_NAME_LENGTH);

	name = (char*)(page + ofs);
	TMTRACE(TM_STORAGE_TRACE,
		"tm_metapage_get_modelname(): exit name=%s\n" _ name);
	return name;
}


