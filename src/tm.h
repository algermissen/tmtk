/*
 * $Id$
 *
 * Copyright (c) 2002,2003 Jan Algermissen
 * See the file "COPYING" for copying permission.
 *
 */

#ifndef TM_H
#define TM_H

#ifdef __cplusplus
extern "C" {
#endif


#include "tmtk.h"
#include "tmerror.h"

/** The TmTk handle.
 * Per process/thread library handle.
 * If using several threads, use one handle per thread.
 */
typedef struct TM *TM;

#include "tmdisclosureframework.h"
#include "tmvaluetype.h"
#include "tmstorage.h"





/** Log levels.
 * These log levels are available:
 */
typedef enum {
	TM_LOG_NOTHING = 0,	/**< log nothing */
        TM_LOG_CRITICAL,	/**< log critical errors only */
        TM_LOG_ERROR,		/**< log errors and critical */
        TM_LOG_WARNING,		/**< log warnings, errors and critical */
        TM_LOG_INFO,		/**< log info, warnings, errors and critical */
        TM_LOG_DEBUG		/**< log everything */
} TMLogLevel;

/** Initialize a library handle.
 * @return handle or NULL if out of memory.
 */
TM_API(TMError) tm_init(TM *ptm,const char *df_name,void *df_arg);

/** Cleanup a library handle.
 * @param ptm pointer to library handle.
 */
TM_API(void) tm_cleanup(TM *ptm);

/** Set log level.
 * Set the maximum level of messages to be logged.
 */
TM_API(int) tm_set_loglevel(TM tm,int level);

/** Log something.
 * Print a log message to stdout.
 * @todo allow user defined log function.
 * @param tm library handle
 * @param level log level
 * @param fmt printf style format string, followed by arguments
 *
 */
TM_API(void) tm_log(TM tm,int level,const char *fmt,...);

/** Set lib handle's error message.
 *
 */
TM_API(const char*) tm_set_error(TM self, const char *fmt,...);

/** Get libhandle's error message.
 *
 */
TM_API(const char*) tm_get_error(TM self);

/** Lookup a value type implementation by its name.
 *
 */
TM_API(TMValueType) tm_lookup_valuetype(TM self,const char *name);

/** Lookup a storage implementation by its name.
 *
 */
TM_API(TMTopicMapStorageDescriptor) tm_lookup_storage_descriptor(TM self,const char *name);



TM_API(TMDisclosureFramework) tm_get_disclosureframework(TM self);

#ifndef TM_DISABLE_LOGS
#  undef _
#  define _ ,
#  define TM_LOG(TM,LEVEL,ARGS) do { tm_log((TM),(LEVEL),ARGS); } while (0)
#else
#  define LOG(TM,LEVEL,ARGS)     /* empty */
#endif /* TM_DISABLE_LOGS */



/** @} */



#ifdef __cplusplus
} // extern "C"
#endif


#endif /* ifndef TM_H */

