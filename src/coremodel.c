/*
 * $Id$
 *
 * Copyright (c) 2002 Jan Algermissen
 * See the file "COPYING" for copying permission.
 *
 */

#include "tmmodel.h"

extern struct TMValueType ASIDP;
extern struct TMValueType TopicSet;

struct TMProperty a_sidp  = {
	"RM-Core",
	"a-sidp",
	&ASIDP,
	TM_SIDP,
	"FIXME",
	"RM-Core::a-sidp",
	NULL, 			/* no combination code */
	NULL,			/* no equality test */
};
struct TMProperty t_sidp  = {
	"RM-Core",
	"t-sidp",
	&TopicSet,
	TM_SIDP,
	"FIXME",
	"RM-Core::t-sidp",
	NULL, 			/* no combination code */
	NULL,			/* no equality test */
};

struct TMList item2 = { NULL, &t_sidp };
struct TMList props = { &item2, &a_sidp };



struct TMModel coremodel = {
	"RM-Core",		/* Name */		
	"The core model implicitly defined by the RM",	
	&props,
	NULL,			/* no assertion types */
	NULL,			/* no required other models */
	NULL			/* no additional subjects */
};

