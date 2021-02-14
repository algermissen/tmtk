/*
 * $Id$
 *
 * Copyright (c) 2002 Jan Algermissen
 * See the file "COPYING" for copying permission.
 *
 */
#ifndef TM_TABLE_INCLUDED
#define TM_TABLE_INCLUDED

#include "tmtk.h"
#include "tmlist.h"
 
#ifdef __cplusplus
extern "C" {
#endif

/** \defgroup TMTable
 * \ingroup AbstractDataTypes
 */
/** @{ */

typedef struct TMTable *TMTable;

TM_API(TMTable) tm_table_new(int hint, int cmp(const void *x,const void *y),
		unsigned hash(const void *key));
TM_API(void) tm_table_delete(TMTable *table);
TM_API(int) tm_table_length(TMTable table);
TM_API(void *) tm_table_put(TMTable table, const void *key, void *value);
TM_API(void *) tm_table_get(TMTable table, const void *key);
TM_API(TMList) tm_table_keys(TMTable table, TMList *listp);
TM_API(void *) tm_table_remove(TMTable table, const void *key);
TM_API(void) tm_table_apply(TMTable table, void apply(const void *key, void **value, void *cl), void *cl);

/** @} */
#ifdef __cplusplus
} // extern C
#endif

#endif /* ! TM_TABLE_INCLUDED */

