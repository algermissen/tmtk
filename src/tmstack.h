/*
 * $Id$
 *
 * Copyright (c) 2002 Jan Algermissen
 * See the file "COPYING" for copying permission.
 *
 */
#ifndef TM_STACK_INCLUDED
#define TM_STACK_INCLUDED

#include  "tmtk.h"
 
#ifdef __cplusplus
extern "C" {
#endif

/** \defgroup TMStack
 * \ingroup AbstractDataTypes
 * A stack class.
 *
 * @{
 */


/** The stack type.
 */ 
typedef struct TMStack *TMStack; 

/** Constructor.
 *
 * Create a new stack object.
 *
 * @param hint a hint for the size
 * @return the new stack
 */
TM_API(TMStack) tm_stack_new(int hint);
TM_API(void) tm_stack_delete(TMStack *pself);
TM_API(int) tm_stack_empty(TMStack self);
TM_API(int) tm_stack_size(TMStack self);
TM_API(void) tm_stack_push(TMStack self, void *item);
TM_API(void *) tm_stack_pop(TMStack self);
TM_API(void *) tm_stack_top(TMStack self); 

/** @} */ 

#ifdef __cplusplus
} // extern C
#endif

#endif /* TM_STACK_INCLUDED */
