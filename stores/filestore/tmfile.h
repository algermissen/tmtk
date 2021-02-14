/*
 * $Id$
 *
 * Copyright (c) 2002 Jan Algermissen
 * See the file "COPYING" for copying permission.
 *
 */
#ifndef TM_FILE_H
#define TM_FILE_H

typedef struct TMFile *TMFile; 

#include <topicmaps.h> 
#include "page.h"


TM_API(TMFile) tm_file_new(const char *path, long cache_size);
TM_API(void) tm_file_delete(TMFile *pself);
TM_API(TMError) tm_file_open(TMFile self, int *created_flag);
TM_API(TMError) tm_file_close(TMFile self);

TM_API(const char*) tm_file_get_error(TMFile self);
TM_API(TMError) tm_file_flush_cache(TMFile self);

TM_API(TMError) tm_file_create_page(TMFile self,GS_INITFUNC_T initfunc,int *np);

TM_API(TMError) tm_file_fetch_page(TMFile self,int n,TMPage *pp);
TM_API(TMError) tm_file_drop_page(TMFile self, TMPage page);

/* obsolete (I think)
TM_API(TMError) gs_storage_create_topicmap(const char *name); 
TM_API(TMError) gs_storage_destroy_topicmap(const char *name); 
*/


#endif

