/*
 * $Id$
 *
 * Copyright (c) 2002 Jan Algermissen
 * See the file "COPYING" for copying permission.
 *
 */
#ifndef TM_PROPERTY_INCLUDED
#define TM_PROPERTY_INCLUDED

#include "tmtk.h" 


#ifdef __cplusplus
extern "C" {
#endif

/** \defgroup TMProperty
 * The Topic Map Application class.
 * Topic Map Applications are instances of this structure.
 *
 * @{
 */


/** Property Definition.
 *
 */
typedef struct TMProperty *TMProperty;

/** \enum Kalatschi
 * These are values for indicating property type.
 * FIXME
 */
enum {
	TM_SIDP,
	TM_OP
};

#include "tmvaluetype.h"

struct TMProperty {
	const char *model;
	const char *name;
	TMValueType value_type;	
	int type;
	const char *description;
	const char *fullname;
	const char *combination_code;
	const char *equality_code;
};




/** Get the value type of a property.
 *
 */
#define tm_property_get_valuetype(prop) (prop)->value_type




/** @} */


#ifdef __cplusplus
}
#endif


#endif /* TM_PROPERTY_INCLUDED */
