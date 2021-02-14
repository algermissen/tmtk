/*
 * $Id$
 *
 * Copyright (c) 2002 Jan Algermissen
 * See the file "COPYING" for copying permission.
 *
 */

#ifndef TM_DISCLOSUREFRAMEWORK_INCLUDED
#define TM_DISCLOSUREFRAMEWORK_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

typedef struct TMDisclosureFramework *TMDisclosureFramework;

#include "tmtk.h"
#include "tmerror.h"
#include "tmmodel.h"
#include "tmprocmodel.h"
#include "tm.h"
#include "tmtopicmap.h"


typedef TMError (*TMDisclosureFrameworkLookupModelFunction)(TMDisclosureFramework,TM tm,const char *,TMModel*);
typedef TMError (*TMDisclosureFrameworkLookupProcModelFunction)(TMDisclosureFramework,TM tm,const char *,TMProcModel*);
typedef TMError (*TMDisclosureFrameworkLookupViewHandlerFunction)(TMDisclosureFramework,TM tm,const char *,TMViewHandler*);

#include <stdarg.h> 

/** \defgroup TMDisclosureFramework
 * @{
 */

struct TMDisclosureFramework {
	const char *name;
	TMDisclosureFrameworkLookupModelFunction lookup_model;
	TMDisclosureFrameworkLookupProcModelFunction lookup_procmodel;
	TMDisclosureFrameworkLookupViewHandlerFunction lookup_viewhandler;
};


TM_API(TMError) tm_disclosureframework_lookup_model(TMDisclosureFramework self,TM tm, const char *name, TMModel *);
TM_API(TMError) tm_disclosureframework_lookup_procmodel(TMDisclosureFramework self,TM tm, const char *name, TMProcModel *);
TM_API(TMError) tm_disclosureframework_lookup_viewhandler(TMDisclosureFramework self,TM tm, const char *name, TMViewHandler *);

/**
 * @}
 */

#endif
