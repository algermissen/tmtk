/*
 * $Id: stmql.h,v 1.5 2002/09/15 22:41:47 jan Exp $
 *
 * Copyright (c) 2002 Jan Algermissen
 * See the file "COPYING" for copying permission.
 *
 */

#ifndef STMQL_INCLUDED
#define STMQL_INCLUDED 

#include <tm.h>
#include "topicset.h"
#include "topicmap.h"

enum {
	SUBJECT_REPR_TYPE_LOCATOR,
	SUBJECT_REPR_TYPE_NODE,
	SUBJECT_REPR_TYPE_DATA,
	SUBJECT_REPR_TYPE_SIR_LOCATOR,
	SUBJECT_REPR_TYPE_SIR_DATA,
	SUBJECT_REPR_TYPE_ANY_BASENAME
};

#define MAXSELECTIONS 10
 


struct select_block {
	int distinct;
	int nselections;
	gs_topic_t p_topic;
	struct {
		gs_topic_t r_topic;
		int subject_repr_type;
	} selections[MAXSELECTIONS];
	gs_topic_t rr_topic;
	gs_topic_t rx_topic;
	struct condition condition;
};

struct gs_parse_state {
	GS_GRAPH_T topicmap;
	char errstr[1024];
	const char *query;
	const char *ptr;
	int len;
	int current_token;
	
	char text[GS_MAXLINE];	/* last parsed text */
	gs_topic_t topic;		/* last parsed topic */
	GS_NODESET_T topicset;	/* last parsed topicset */	
	int intval;		/* last parsed int value */
	/*
	GWStack_T queries;	
	struct query *currentQuery;
	*/
	/*
	GWNodeSet_T lastSet;
	*/

	/* function table */
	/* user args ? */

	struct select_block *current_select_block;
};


void gs_parse_state_init(struct gs_parse_state *ps,GS_GRAPH_T g, const char *query);

/* return 1 on success, 0 on error */
int gs_parse_query(struct gs_parse_state *ps);
void gs_parse_state_cleanup(struct gs_parse_state *ps);
const char* gs_parse_state_error(struct gs_parse_state *ps);


#endif

