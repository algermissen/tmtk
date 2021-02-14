/*
 * $Id$
 *
 * Copyright (c) 2002 Jan Algermissen
 * See the file "COPYING" for copying permission.
 *
 */
#ifndef TM_PARAMS_INCLUDED
#define TM_PARAMS_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

#include "tmtk.h"

/** \defgroup TMParams
 * \ingroup AbstractDataTypes
 */
/** @{ */

typedef struct TMParams *TMParams; 

enum { TM_PARAMS_SIMPLE = 1, TM_PARAMS_CGI };

/** Create a new parameters object.
 * Parses s accoring to type and returns new parameter object.
 * @param s the parameter string
 * @param type how s is encoded
 * @return new parameter object (FIXME: NULL on error? )
 */
TM_API(TMParams) tm_params_new(const char *s,int type);

/** Delete parameters object.
 * @param pself pointer to object (will be set to NULL)
 */
TM_API(void) tm_params_delete(TMParams *pself);

/** Get value of parameter named 'name'.
 * @param self the parameters object
 * @param name the name of the parameter
 * @return value or NULL if not found
 */
TM_API(const char *) tm_params_get(TMParams self, const char *name);


/** @} */

#ifdef __cplusplus
} // extern "C"
#endif

#endif

