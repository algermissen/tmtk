#ifndef TM_ERROR_H
#define TM_ERROR_H

#include "tmtk.h"
 
#ifdef __cplusplus
extern "C" {
#endif
 
/** \defgroup Errors
 * \ingroup LibraryCore
 *
 * @{ 
 */


/* FIXME: error should not tell where but WHAT */

/** Error code.
 * General TmTk error code
 */
typedef enum {
	TM_OK,			/* no error */
	TM_FAIL,		/* unspecified error, check error message */
	TM_E_SIDP_EXPECTED,	/* SIDP expected */
	TM_E_PROP_NOT_FOUND,	/* property not found */
	TM_E_TOPIC_NOT_FOUND,	/* topic not found */
	TM_E_SYNTAX,   		 /* syntax error */
	TM_ENODE_NOT_FOUND,     /* required topic not found */
	TM_ETOPIC_NOT_FOUND,     /* required topic not found */
	TM_EMODEL_NOT_LOADED,	/* model not loaded */
	TM_ESYS,	/* error during system call */
	TM_EMODEL_UNKNOWN,	/* unknown TMA name */
	TM_ECONTRADICTING_PROPERTY_VALUES, /* contradicting SIDP values */
	TM_LIKE_LOOKUP_NOT_SUPPORTED, 		/* like lookup not supported */
	TM_E_PSTORE_NOT_NORMALIZED,	/* property store not normalized */
	TM_E_OP_UNDEFINED,	/* operator not defined for value type */
	TM_E_INVALID_ASSERTION, /* invalid assertion */
	TM_E_AMBIGUOUS_LOOKUP, /* ambigous lookup */
	TM_E_VALUE_STORE_NOT_FOUND, /* value store not found for value type */
	TM_E_PARSE, /* parse error */
	TM_E_PARAM /* missing or invalid parameter */
} TMError;



/** Get string representation of error code.
 * @param e the error code
 * @return the string
 */
TM_API(const char*) tm_strerror(TMError e);

/** @} */
#ifdef __cplusplus
} // extern C
#endif

#endif


