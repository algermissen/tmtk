/*
 * $Id: util.c,v 1.8 2002/10/12 19:05:57 jan Exp $
 *
 * Copyright (c) 2002 Jan Algermissen
 * See the file "COPYING" for copying permission.
 *
 */

#include "tmmem.h"
#include "tmtrace.h"
#include "tmassert.h"

void *gs_alloc(long nbytes, const char *file, int line)
{
        void *ptr;
 
        assert(nbytes > 0);
 
        ptr = malloc(nbytes);
        if(ptr == NULL)
                TMTRACE(TM_ANY_TRACE,"gs_alloc: out of memory in %s line %d" _
                        file _ line);
	if (ptr == NULL)
	{
		assert(ptr);
	}
        return ptr;
}
void *gs_calloc(long count,long nbytes, const char *file, int line)
{
        void *ptr;
 
        assert(nbytes > 0);
        assert(count > 0);
 
        ptr = calloc(count,nbytes);
        if(ptr == NULL)
                TMTRACE(TM_ANY_TRACE,"gs_calloc: out of memory in %s line %d" _
                        file _ line);
        return ptr;
}
void *gs_resize(void *ptr,long nbytes, const char *file, int line)
{
        assert(nbytes > 0);
        assert(ptr);
 
        ptr = realloc(ptr,nbytes);
        if(ptr == NULL)
                TMTRACE(TM_ANY_TRACE,"gs_resize: out of memory in %s line %d" _
                        file _ line);
        return ptr;
}

void gs_free(void *ptr, const char *file, int line)
{
        if(ptr)
                free(ptr);
        else
                assert(!"gs_free called with NULL pointer !");
}
