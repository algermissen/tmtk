/*
 * $Id: util.h,v 1.6 2002/10/12 19:05:57 jan Exp $
 *
 * Copyright (c) 2002 Jan Algermissen
 * See the file "COPYING" for copying permission.
 *
 */

#ifndef MEM_H
#define MEM_H

#include "tmtk.h"

 
#ifdef __cplusplus
extern "C" {
#endif

/** \defgroup MemoryManagement
 * \ingroup LibraryCore
 *
 * @{
 */
 


TM_API(void *) gs_alloc(long nbytes, const char *file, int line);
TM_API(void *) gs_calloc(long count, long nbytes, const char *file, int line);
TM_API(void *) gs_resize(void *ptr, long nbytes, const char *file, int line);
TM_API(void) gs_free(void *ptr, const char *file, int line);

#define TM_ALLOC(nbytes) gs_alloc((nbytes), __FILE__, __LINE__)
#define TM_CALLOC(count,nbytes) gs_calloc((count),(nbytes), __FILE__, __LINE__)
#define TM_RESIZE(ptr,nbytes) ((ptr) = gs_resize((ptr),(nbytes),__FILE__,__LINE__))
#define TM_FREE(ptr) ((void)(gs_free((ptr),__FILE__,__LINE__) , (ptr) = 0))
#define TM_NEW(p) ((p) = TM_ALLOC((long)sizeof *(p)))
#define TM_NEW0(p) ((p) = TM_CALLOC(1,(long)sizeof *(p)))

/* depricated */
#define GS_ALLOC(nbytes) gs_alloc((nbytes), __FILE__, __LINE__)
#define GS_CALLOC(count,nbytes) gs_calloc((count),(nbytes), __FILE__, __LINE__)
#define GS_RESIZE(ptr,nbytes) ((ptr) = gs_resize((ptr),(nbytes),__FILE__,__LINE__))
#define GS_FREE(ptr) ((void)(gs_free((ptr),__FILE__,__LINE__) , (ptr) = 0))
#define GS_NEW(p) ((p) = GS_ALLOC((long)sizeof *(p)))
#define GS_NEW0(p) ((p) = GS_CALLOC(1,(long)sizeof *(p)))

/** @} */

#ifdef __cplusplus
} // extern C
#endif

#endif
