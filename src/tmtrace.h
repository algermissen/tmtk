/*
 * $Id: util.h,v 1.6 2002/10/12 19:05:57 jan Exp $
 *
 * Copyright (c) 2002 Jan Algermissen
 * See the file "COPYING" for copying permission.
 *
 */
#ifndef TM_TRACE_H
#define TM_TRACE_H

#include "tmtk.h"
 
#ifdef __cplusplus
extern "C" {
#endif

/** 
 *
 *
 * \defgroup Tracing
 * \ingroup LibraryCore
 *
 * @{
 */


#ifndef NDEBUG
TM_API(unsigned int)		TM_TraceFlag; 
#define TM_TRACEFLAG		(TM_TraceFlag)
#else
#define TM_TRACEFLAG		0
#endif

typedef enum _TMTraceFlags {
    TM_SHOW_GRAPH_TRACE		= 0x1,
    TM_SHOW_STORAGE_TRACE	= 0x2,
    TM_SHOW_PROC_TRACE		= 0x4,
    TM_SHOW_VALUETYPE_TRACE	= 0x8,
    TM_SHOW_MODEL_TRACE		= 0x10,
    TM_SHOW_PARSE_TRACE		= 0x20,
    TM_SHOW_QUERY_TRACE		= 0x20,

/*
    TM_SHOW_LOOKUP_TRACE	= 0x40,

    TM_SHOW_ERR_TRACE		= 0x100,
    TM_SHOW_LOG_TRACE		= 0x200,
    TM_SHOW_STACK_TRACE		= 0x400,
    TM_SHOW_STMQL_TRACE		= 0x800,
    TM_SHOW_HTTP_TRACE		= 0x1000,
*/
    TM_SHOW_APP_TRACE		= 0x2000,
/*
    TM_SHOW_PAGE_TRACE		= 0x4000,
    TM_SHOW_X_TRACE		= 0x4000,
*/
    TM_SHOW_OMNIVORE_TRACE      = 0x8000,
/*
    TM_SHOW_MODEL_TRACE		= 0x10000,
*/
 
 
    TM_SHOW_ALL_TRACE      = (int) 0xFFFFFFFF
} TMTraceFlags;


#define TM_ANY_TRACE		(1)
#define TM_PROC_TRACE		(TM_TRACEFLAG & TM_SHOW_PROC_TRACE)
#define TM_GRAPH_TRACE		(TM_TRACEFLAG & TM_SHOW_GRAPH_TRACE)
#define TM_STORAGE_TRACE	(TM_TRACEFLAG & TM_SHOW_STORAGE_TRACE)
#define TM_VALUETYPE_TRACE	(TM_TRACEFLAG & TM_SHOW_VALUETYPE_TRACE)
#define TM_MODEL_TRACE		(TM_TRACEFLAG & TM_SHOW_MODEL_TRACE)
#define TM_PARSE_TRACE		(TM_TRACEFLAG & TM_SHOW_PARSE_TRACE)
#define TM_QUERY_TRACE		(TM_TRACEFLAG & TM_SHOW_QUERY_TRACE)




#define TM_LOOKUP_TRACE		(TM_TRACEFLAG & TM_SHOW_LOOKUP_TRACE)
#define TM_LOG_TRACE		(TM_TRACEFLAG & TM_SHOW_LOG_TRACE)
#define TM_ERR_TRACE		(TM_TRACEFLAG & TM_SHOW_ERR_TRACE)
#define TM_STMQL_TRACE		(TM_TRACEFLAG & TM_SHOW_STMQL_TRACE)
#define TM_STACK_TRACE		(TM_TRACEFLAG & TM_SHOW_STACK_TRACE)
#define TM_HTTP_TRACE		(TM_TRACEFLAG & TM_SHOW_HTTP_TRACE)
#define TM_APP_TRACE		(TM_TRACEFLAG & TM_SHOW_APP_TRACE)
#define TM_PAGE_TRACE		(TM_TRACEFLAG & TM_SHOW_PAGE_TRACE)
#define TM_X_TRACE		(TM_TRACEFLAG & TM_SHOW_X_TRACE)
#define TM_OMNIVORE_TRACE	(TM_TRACEFLAG & TM_SHOW_OMNIVORE_TRACE)


#ifndef NDEBUG
#  undef _
#  define _ ,
#  define TMTRACE(TYPE,FMT) do { if(TYPE) tm_trace(TM_TRACEFUNCTION,FMT); } while (0)
#  define TMTRACE_NOFUNC(TYPE,FMT) do { if(TYPE) tm_trace(NULL,FMT); } while (0)
   TM_API(int) tm_trace(const char *func,const char * fmt, ...);
   TM_API(int) tm_trace_str(const char *str, int len);
#else
#  define TMTRACE(TYPE,FMT)     /* empty */
#  define TMTRACE_NOFUNC(TYPE,FMT)     /* empty */
   /* no prototype for tm_trace() ! */
   /* no prototype for tm_trace_str() ! */
#endif /* !NDEBUG */

TM_API(int) tm_set_trace_mask(const char * shortnames);


/** @} */


#ifdef __cplusplus
} // extern C
#endif

#endif


