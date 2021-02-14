/*
 * $Id$
 *
 * Copyright (c) 2002 Jan Algermissen
 * See the file "COPYING" for copying permission.
 *
 */

#include <tmstack.h>
#include <tmassert.h>
#include <tmmem.h>
#include <tmtrace.h>
#include <tmutil.h>
#include <tmtable.h>
#include <tmtopicset.h>

#include <tmassertion.h>
#include <tmstorage.h>
#include <tmtopicmap.h>


static int _cond_eval_is(TMCond self, TMTopicAssertion astn)
{
	assert(self);
	assert(astn);
	assert(self->op == TM_COND_OP_IS);

	assert(self->u.is.match_topic);

	switch(self->u.is.comp_flag) {
	case TM_ASSERTION_COMPONENT_SELF:
		return ( astn->A == self->u.is.match_topic );
	case TM_ASSERTION_COMPONENT_PLAYER:
		return (    tm_topic_assertion_get_player_of_role(astn,self->u.is.rtopic)
			 == self->u.is.match_topic );
	case TM_ASSERTION_COMPONENT_CASTING:
		return (    tm_topic_assertion_get_casting_of_role(astn,self->u.is.rtopic)
			 == self->u.is.match_topic );
	default:
		assert(0);
	}
	/* not reached */
	assert(0);
	return 0;
}
static int _cond_eval_in(TMCond self, TMTopicAssertion astn)
{
	assert(self);
	assert(astn);
	assert(self->op == TM_COND_OP_IN);

	assert(self->u.in.match_set);

	switch(self->u.in.comp_flag) {
	case TM_ASSERTION_COMPONENT_SELF:
		return ( tm_topicset_contains(self->u.in.match_set,astn->A) );
	case TM_ASSERTION_COMPONENT_PLAYER:
		return (tm_topicset_contains(self->u.in.match_set,
				tm_topic_assertion_get_player_of_role(astn,self->u.in.rtopic)) );
	case TM_ASSERTION_COMPONENT_CASTING:
		return (tm_topicset_contains(self->u.in.match_set,
				tm_topic_assertion_get_casting_of_role(astn,self->u.in.rtopic)) );
	default:
		assert(0);
	}
	/* not reached */
	assert(0);
	return 0;
}



int tm_cond_eval(TMCond self, TMTopicAssertion astn)
{
        if(!self)
                return 1;
        switch(self->op) {
        case TM_COND_OP_AND:
                return (   tm_cond_eval(self->u.children.left,astn)
			&& tm_cond_eval(self->u.children.right,astn) );
        case TM_COND_OP_OR:
                return (   tm_cond_eval(self->u.children.left,astn)
			|| tm_cond_eval(self->u.children.right,astn) );
        case TM_COND_OP_NOT:
                return ( ! tm_cond_eval(self->u.children.left,astn) );
        case TM_COND_OP_IS:
                return ( _cond_eval_is(self,astn) );
        case TM_COND_OP_IN:
                return ( _cond_eval_in(self,astn) );
	default:
		assert( !"unknown operator" );
	} 
	/* not reached */
	assert(0);
	return 0;
}


TMCond tm_cond_make_simple_is_cond(TMCond self, TMTopic rtopic, TMTopic xtopic)
{
	assert(self);
	self->op = TM_COND_OP_IS;
	self->u.is.comp_flag = TM_ASSERTION_COMPONENT_PLAYER;
	self->u.is.rtopic = rtopic;
	self->u.is.match_topic = xtopic;
	
	return self;

}
TMCond tm_cond_make_simple_in_cond(TMCond self, TMTopic rtopic, TMTopicSet xtopicset)
{
	assert(self);
	assert(xtopicset);
	self->op = TM_COND_OP_IN;
	self->u.in.comp_flag = TM_ASSERTION_COMPONENT_PLAYER;
	self->u.in.rtopic = rtopic;
	self->u.in.match_set = xtopicset; /* copy ?? */

	return self;
}

TM_API(void) tm_cond_cleanup(TMCond self)
{
	assert(self);
	/* FIXME */
}

