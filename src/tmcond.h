/*
 * $Id: topicmap.h,v 1.9 2002/10/02 22:06:25 jan Exp $
 *
 * Copyright (c) 2002 Jan Algermissen
 * See the file "COPYING" for copying permission.
 *
 */

#ifndef TM_COND_H
#define TM_COND_H

#include <tm.h>
#include <tmerror.h>
#include <tmtopicset.h>
#include <tmassertion.h> 

#ifdef __cplusplus
extern "C" {
#endif

/** \defgroup TMCond
 *
 * @{
 */

enum {
	TM_COND_OP_AND,
	TM_COND_OP_OR,
	TM_COND_OP_NOT,
	TM_COND_OP_IS,
	TM_COND_OP_IN
};
enum {
	TM_ASSERTION_COMPONENT_SELF,
	TM_ASSERTION_COMPONENT_PLAYER,
	TM_ASSERTION_COMPONENT_CASTING
};


/** Structure to hold 'WHERE' clause for assertion cursors.
 *
 */
typedef struct TMCond *TMCond;


/* 'where' clause tree for assertion selection */
struct TMCond
{
	int op;
	union {
		struct {
			TMCond left;
			TMCond right;
		} children;
		struct {
			int comp_flag;
			TMTopic rtopic;
			TMTopic match_topic;
		} is;
		struct {
			int comp_flag;
			TMTopic rtopic;
			TMTopicSet match_set;
		} in;
	} u;
};


TM_API(int) tm_cond_eval(TMCond self, TMTopicAssertion astn);
TM_API(TMCond) tm_cond_make_simple_is_cond(TMCond self, TMTopic rtopic, TMTopic xtopic);
TM_API(TMCond) tm_cond_make_simple_in_cond(TMCond self, TMTopic rtopic, TMTopicSet xtopicset);
TM_API(void) tm_cond_cleanup(TMCond self);

/** @} */

#ifdef __cplusplus
} // extern "C"
#endif

#endif


