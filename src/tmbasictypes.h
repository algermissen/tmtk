/*
 * $Id$
 *
 * Copyright (c) 2002 Jan Algermissen
 * See the file "COPYING" for copying permission.
 *
 */

#ifndef TM_BASICTYPES_H
#define TM_BASICTYPES_H

#include "tmvaluetype.h"

#ifdef __cplusplus
extern "C" {
#endif



extern struct TMValueType Topic;
extern struct TMValueType TopicSet;
enum { TM_OP_TOPICSET_VALUE_INCLUDES_TOPICS = 1 };

extern struct TMValueType Integer;
enum {
	TM_OP_Integer_EQUAL = 1,
	TM_OP_Integer_LT,
	TM_OP_Integer_GT,
	TM_OP_Integer_LT_OR_EQUAL,
	TM_OP_Integer_GT_OR_EQUAL
};


extern struct TMValueType Text;
/* internal representation is char* */
enum { TM_OP_Text_EQUAL = 1, TM_OP_Text_LIKE };



extern struct TMValueType TextSet;
/* internal representation is TMList of char* */
enum { TM_OP_TextSet_NOT_DISJOINT = 1, TM_OP_TextSet_INCLUDES , TM_OP_TextSet_LIKE };

extern struct TMValueType ASIDP;
/* internal representation: */
struct asidp_value {
	TMTopic type;
	TMList memberships; /* of items of the follwoing struct */
};
struct a_membership {
	TMTopic role;
	TMTopic player;
};
enum { TM_OP_ASIDP_HAS_TYPE = 1, TM_OP_ASIDP_VALUE_INCLUDES_TOPICS, TM_OP_ASIDP_HAS_PLAYER_PAIR };



/*
extern struct TMValueType XOP;
*/
/* internal representation is TMList of items of the follwoing struct */
/*
struct x_playing {
	TMTopic assertion;
	TMTopic role;
};
*/
         

#ifdef __cplusplus
} // extern C
#endif

#endif
