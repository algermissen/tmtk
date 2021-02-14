#include "tmassert.h"
#include "tmtrace.h"
#include "tmdisclosureframework.h"


TMError tm_disclosureframework_lookup_model(TMDisclosureFramework self,TM tm, const char *name, TMModel *mp)
{
	TMError e;
	assert(self);
	assert(name);
	assert(mp);
	*mp = NULL;

	if( (e = self->lookup_model(self,tm,name,mp)) != TM_OK)
		return e;

	return TM_OK;
	
}



TMError tm_disclosureframework_lookup_procmodel(TMDisclosureFramework self,TM tm, const char *name, TMProcModel *mp)
{
	TMError e;
	assert(self);
	assert(name);
	assert(mp);
	*mp = NULL;

	if( (e = self->lookup_procmodel(self,tm,name,mp)) != TM_OK)
		return e;

	return TM_OK;
	
}



TMError tm_disclosureframework_lookup_viewhandler(TMDisclosureFramework self,TM tm, const char *name, TMViewHandler *mp)
{
	TMError e;
	assert(self);
	assert(name);
	assert(mp);
	*mp = NULL;

	if( (e = self->lookup_viewhandler(self,tm,name,mp)) != TM_OK)
		return e;

	return TM_OK;
	
}



