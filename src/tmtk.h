/*
 * $Id$
 *
 * Copyright (c) 2002,2003 Jan Algermissen
 * See the file "COPYING" for copying permission.
 *
 */

#ifndef TMTK_H
#define TMTK_H

#include "config.h"
#include <stdlib.h>
#include <stdarg.h>
#include <string.h> 


#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned char byte;

#ifdef HAVE_BZERO
void bzero(void *,size_t);
#else
#define bzero(ptr,n) memset((prt),0,(n))
#endif

/* FIXME: we also need to deal with C99 vs other snprintf versions. at least
 * for the to_string functions of the value types.
 */
#ifdef HAVE_SNPRINTF
int snprintf(char*,size_t,const char *,...);
int vsnprintf(char*,size_t,const char *,va_list);
#else
#error snprintf is needed for this library
#endif

#ifndef TM_API
#define TM_API(type) type
#endif

/* works for gcc 2.4 and up */
/* FIXME: make this better! */
/*
#define TM_TRACEFUNCTION ((const char *) 0)
*/
#define TM_TRACEFUNCTION __PRETTY_FUNCTION__

/** \defgroup CoreTypes
 *
 * @{
 */

/** \typedef TMTopic
 * The topic type.
 * This must fit in a pointer.
 */
typedef unsigned int TMTopic;

/** \typedef TMVid
 * The value ID type.
 * This must fit in a pointer.
 */
typedef unsigned int TMVid;

/** @} */


/** \defgroup MagicNumbers
 * \ingroup LibraryCore
 *
 * @{
 */

/** The maximum number of members in an assertion.
 * 
 * To avoid frequent allocations and deallocations, libtm limits the
 * number of members an assertion can have. If your application needs
 * to support more you have to adjust the value here and recompile.
 */
#define TM_MAXMEMBERS 40	/* maximum number of members in an assertion */

/** Maximum number of properties on a topic.
 * \see TM_MAXMEMBERS
 */
#define TM_MAXPROPERTIES 20	/* maximum number of properties on a topic */

#define TM_PROP_SEPERATOR "::"

/** The 'null'-topic value.
 */
#define TM_NO_NODE 0

/** XML namespace delimiter used by expat.
 */
#define NS_DELIM '|'


/**
 * Size of string buffer for value tracing.
 */

#define TM_VALUE_TRACE_BUFSIZE 16000

/** @} */

/* -------------------------------------------------------------------------
 * 
 *
 *
 * ------------------------------------------------------------------------- */

/* a lot of FIXME around here -> clean up */
enum {
	TM_LOOKUP_EQUAL,
	TM_LOOKUP_LIKE
};



/* for omnivore and proc models to keep track of non-existing
 * but referenced  syntactical constructs
 */
enum {
	TM_DEMAND_EXPLICIT,
	TM_DEMAND_IMPLICIT
};


#ifdef __cplusplus
} // extern "C"
#endif


#endif /* ifndef TM_H */

