/*
 * $Id$
 *
 * Copyright (c) 2002 Jan Algermissen
 * See the file "COPYING" for copying permission.
 *
 */

#include "tmmodel.h"
#include "tmutil.h"
#include "tmtrace.h"
#include "tmassert.h"

TMProperty tm_model_get_property(TMModel self, const char *model,const char *name)
{
	TMList lp;
	TMList mp;
	assert(self);
	assert(name);
	TMTRACE(TM_MODEL_TRACE,"enter: [model=%s,name=%s]\n" _
			model ? model : "NULL" _ name);

	if(!model || strcmp(model,self->name) == 0)
	{
		for(lp = self->properties; lp; lp=lp->next)
		{
			TMProperty p = (TMProperty)lp->content;

			if(strcmp(p->name,name) == 0)
				return p;
		}
		return NULL;
	}
	TMTRACE(TM_MODEL_TRACE,"not found here, so I am looking in required models\n");
	
	for(mp = self->require; mp; mp = mp->next)
	{
		TMModel m = (TMModel)mp->content;
		if( strcmp(model,m->name) != 0)
			continue;
		TMTRACE(TM_MODEL_TRACE,"now recursive call\n");
		return (tm_model_get_property(m,NULL,name));
	}
	TMTRACE(TM_MODEL_TRACE,"exit (not found)\n");
	return NULL;
}

