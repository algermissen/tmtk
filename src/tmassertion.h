/*
 * $Id: assertion.h,v 1.4 2002/09/04 20:53:04 jan Exp $
 *
 * Copyright (c) 2002 Jan Algermissen
 * See the file "COPYING" for copying permission.
 *
 */

#ifndef TM_ASSERTION_H
#define TM_ASSERTION_H

 
#ifdef __cplusplus
extern "C" {
#endif

/** \defgroup TMAssertion
 * \ingroup AbstractDataTypes
 *
 * @{
 */
 
/** A structure connecting several subjects to an assertion.
 */	
typedef struct TMAssertion *TMAssertion;

/** A structure connecting several topics to an assertion.
 */	
typedef struct TMTopicAssertion *TMTopicAssertion;

#include "tmtk.h"
#include "tmsubject.h"
#include <stdio.h>   /* for FILE */


struct TMAssertion {
	TMSubject type;
	TMSubject self;
	int N;
	struct {
		TMSubject role;
		TMSubject casting;
		TMSubject player;
	} memberships[TM_MAXMEMBERS];
};

struct TMTopicAssertion {
        TMTopic T;
        TMTopic A;
        int N;
        struct {
                TMTopic R;
                TMTopic C;
                TMTopic x;
        } memberships[TM_MAXMEMBERS];
};

/** \def TM_SET_ASSERTION_N2(A,t,a,r1,c1,x1,r2,c2,x2)
 * Initialize a TMAssertion structure with a certain type and
 * two memberships.
 */
#define TM_SET_ASSERTION_N2(A,t,a,r1,c1,x1,r2,c2,x2) \
	do { \
		(A)->type = (t); \
                (A)->self = (a); \
                (A)->N = 2;      \
                (A)->memberships[0].role    = (r1); \
                (A)->memberships[0].casting = (c1); \
                (A)->memberships[0].player  = (x1); \
                (A)->memberships[1].role    = (r2); \
                (A)->memberships[1].casting = (c2); \
                (A)->memberships[1].player  = (x2); \
	} while(0) 

/** Initialize a topic assertion.
 * This just sets the whole structure to zeros.
 */
TM_API(void) tm_topic_assertion_init(TMTopicAssertion self);

/** Sort the memberships in the assertion by the topic ID of the
 * roles.
 * This is a preparation for storage and also provides a way
 * for fast comparison of nodal assertions.
 */
TM_API(void) tm_topic_assertion_sort(TMTopicAssertion self);

/** Print an assertion.
 *
 */
TM_API(void) tm_topic_assertion_print(TMTopicAssertion self,FILE* f);

/** Get the player of a certain role.
 *
 */
TM_API(TMTopic) tm_topic_assertion_get_player_of_role(TMTopicAssertion self,TMTopic rtopic);

/** Get the casting topic of a certain role.
 *
 */
TM_API(TMTopic) tm_topic_assertion_get_casting_of_role(TMTopicAssertion self,TMTopic rtopic);

#if 0
TM_API(const char **) TMSubject_getProperty(TMSubject self, int n);
TM_API(int) TMAssertion_getNumberOfMemberships(TMAssertion self);
TM_API(void) TMAssertion_setMembershipRoleSubject(TMAssertion self, int n, const char *model, const char *propname, void *propval);
TM_API(void) TMAssertion_setMembershipCastingSubject(TMAssertion self, int n, const char *model, const char *propname, void *propval);
TM_API(void) TMAssertion_setMembershipPlayerSubject(TMAssertion self, int n, const char *model, const char *propname, void *propval);
TM_API(TMSubject) TMAssertion_getMembershipRoleSubject(TMAssertion self, int n);
TM_API(TMSubject) TMAssertion_getMembershipCastingSubject(TMAssertion self, int n);
TM_API(TMSubject) TMAssertion_getMembershipPlayerSubject(TMAssertion self, int n);
TM_API(void) TMAssertion_setTypeSubject(TMAssertion self, const char *model, const char *propname, void *propval);
TM_API(TMSubject) TMAssertion_getTypeSubject(TMAssertion self);
TM_API(TMSubject) TMAssertion_getAssertionSubject(TMAssertion self);
TM_API(void) TMAssertion_setAssertionSubject(TMAssertion self, const char *model, const char *propname, void *propval);
TM_API(void) TMAssertion_init(TMAssertion self);
#endif

/** @} */

#ifdef __cplusplus
} // extern C
#endif

#endif
