/*
 * $Id: storage.h,v 1.4 2002/09/04 20:53:04 jan Exp $
 *
 * Copyright (c) 2002 Jan Algermissen
 * See the file "COPYING" for copying permission.
 *
 */
#ifndef TM_STORAGE_X_H
#define TM_STORAGE_X_H

typedef struct gs_file_t *GS_FILE_T;

#include <topicmaps.h> 
#include "page.h"


TM_API(TMError) gs_storage_create_topicmap(const char *name); 
TM_API(TMError) gs_storage_destroy_topicmap(const char *name); 


TM_API(TMError) gs_storage_open_file(const char *path,
					GS_INITFUNC_T initfunc,
					long cache_size,
					GS_FILE_T *filep,
					int *created_flag);
TM_API(TMError) gs_storage_close_file(GS_FILE_T f);
TM_API(TMError) gs_storage_flush_cache(GS_FILE_T f);
TM_API(TMError) gs_storage_new_page(GS_FILE_T f,
					GS_INITFUNC_T initfunc,
					int *np);
TM_API(TMError) gs_storage_fetch_page(GS_FILE_T f,GS_PAGE_T *pp,int n);
TM_API(TMError) gs_storage_drop_page(GS_FILE_T f, GS_PAGE_T page);


#endif

