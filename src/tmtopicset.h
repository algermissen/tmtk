/*
 * $Id: topicmap.h,v 1.9 2002/10/02 22:06:25 jan Exp $
 *
 * Copyright (c) 2002 Jan Algermissen
 * See the file "COPYING" for copying permission.
 *
 */
#ifndef TM_NODESET_INCLUDED
#define TM_NODESET_INCLUDED

#include "tmtk.h"

#include <stdio.h> /* for FILE */
 
#ifdef __cplusplus
extern "C" {
#endif
/** \defgroup TMTopicSet
 * \ingroup AbstractDataTypes
 */
/** @{ */
 
     
typedef struct TMTopicSet *TMTopicSet;

TM_API(TMTopicSet) tm_topicset_new(int size_hint);
TM_API(void) tm_topicset_delete(TMTopicSet *pself);
TM_API(int) tm_topicset_empty(TMTopicSet self);
TM_API(void) tm_topicset_clear(TMTopicSet self);
TM_API(int) tm_topicset_size(TMTopicSet self);
TM_API(void) tm_topicset_add(TMTopicSet self, TMTopic topic);
TM_API(int) tm_topicset_remove(TMTopicSet self, TMTopic topic);
TM_API(TMTopic) tm_topicset_get(TMTopicSet self, int i);
TM_API(int) tm_topicset_contains(TMTopicSet self, TMTopic topic);
TM_API(int) tm_topicset_disjoint(TMTopicSet self, TMTopicSet other);
TM_API(void) tm_topicset_print(TMTopicSet self, FILE* f);
TM_API(int) tm_topicset_equal(TMTopicSet self, TMTopicSet other);
TM_API(TMTopicSet) tm_topicset_copy(TMTopicSet self);

/** @} */
#ifdef __cplusplus
} // extern C
#endif

#endif /* TM_NODESET_INCLUDED */
