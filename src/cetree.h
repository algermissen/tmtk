/*
 * $Id: topicmap.h,v 1.9 2002/10/02 22:06:25 jan Exp $
 *
 * Copyright (c) 2002 Jan Algermissen
 * See the file "COPYING" for copying permission.
 *
 */

#ifndef TM_CETREE_H
#define TM_CETREE_H

/*
#include <tm.h>
#include <tmassertion.h> 
#include <tmtopicset.h>
#include <tmerror.h>
*/

#ifdef __cplusplus
extern "C" {
#endif

enum {
	TM_CETREE_OP_SIMPLE_CONDITION = 1,
	TM_CETREE_OP_AND,
	TM_CETREE_OP_OR
};



typedef struct TMCETree *TMCETree;
struct TMCETree {
	int tree_op;
	TMCETree left;
	TMCETree right;
	
	TMProperty prop;
	int op;
	void *arg;


};




#if 0
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
#endif

#ifdef __cplusplus
} // extern "C"
#endif

#endif


