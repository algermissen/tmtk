/*
 * $Id$
 *
 * Copyright (c) 2002 Jan Algermissen
 * See the file "COPYING" for copying permission.
 *
 */

#ifndef TM_MODEL_INCLUDED
#define TM_MODEL_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

/** Topic Map Application Definition.
 * "Model" must be renamed to "Application".
 *
 */

typedef struct TMModel *TMModel;

#include "tmtk.h"
#include "tmlist.h"
#include "tmsubject.h"
#include "tmproperty.h"

/** \defgroup TMModel
 * The Topic Map Application class.
 * Topic Map Applications are instances of this structure.
 *
 * @{
 */


/** Assertion Type Definition.
 *
 */
typedef struct TMAssertionType *TMAssertionType;

/** Role Type Definition.
 *
 */
typedef struct TMRoleType *TMRoleType;



/*
#include <tm.h> 
#include <tmassertion.h> 
#include <tmlist.h>
#include <tmvaluetype.h>
#include <tmtopicmap.h>
#include <omnivore.h>
*/

/** The non-opaque struct for an assertion type.
 * FIXME
 */
struct TMAssertionType {
	const char *name;
	const char *description;
	TMSubject subject;
	TMList roles;
	
};

/** The non-opaque struct for a role type.
 * FIXME
 */
struct TMRoleType {
	const char *name;
	const char *description;
	TMSubject subject;
};


struct TMModel {
	const char *name;		/* Name (5.2.1) */		
	const char *description;	/* description */
	TMList properties;
	TMList atypes;
	TMList require;
	TMList subjects;
};




/** Lookup a property by its name.
 * FIXME
 */
TM_API(TMProperty) tm_model_get_property(TMModel self,const char*model, const char *name);





/** @} */


#ifdef __cplusplus
}
#endif


#endif /* TM_MODEL_INCLUDED */
