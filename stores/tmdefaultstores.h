/*
 * $Id$
 *
 * Copyright (c) 2002 Jan Algermissen
 * See the file "COPYING" for copying permission.
 *
 */
#ifndef TM_DEFAULT_STORES_H
#define TM_DEFAULT_STORES_H

#include <tmstorage.h>
 
#ifdef __cplusplus
extern "C" {
#endif
 

extern struct TMTopicMapStorageDescriptor default_file_storage_descriptor;
extern struct TMTopicMapStorageDescriptor default_mem_storage_descriptor;

#ifdef __cplusplus
} // extern C
#endif

#endif
