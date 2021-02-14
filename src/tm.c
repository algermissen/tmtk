#include "tm.h"
#include "tmmem.h"
#include "tmassert.h"

#include <stdio.h> /* for printfs */

/* ------------------------------------------------------------------------
 *
 *      VALUE TYPES
 * 
 * ------------------------------------------------------------------------ */
extern struct TMValueType Topic;
extern struct TMValueType TopicSet;
extern struct TMValueType Text;
extern struct TMValueType Integer;
extern struct TMValueType TextSet;
extern struct TMValueType ASIDP;

static TMValueType vtypes[] = {
	&Topic,
	&TopicSet,
	&Text,
	&Integer,
	&TextSet,
	&ASIDP,
	NULL	
};
/* ------------------------------------------------------------------------
 *
 *      STORAGES
 * 
 * ------------------------------------------------------------------------ */
extern struct TMTopicMapStorageDescriptor default_mem_storage_descriptor;

static TMTopicMapStorageDescriptor storages[] = {
	&default_mem_storage_descriptor,
	NULL	
};

/* ------------------------------------------------------------------------
 *
 *    DISCLOSURE FRAMEWORKS
 * 
 * ------------------------------------------------------------------------ */
extern struct TMDisclosureFramework www_df;

static TMDisclosureFramework dfs[] = {
	&www_df,
	NULL	
};






struct TM
{
        char errstr[4096];  /* FIXME */
        int log_level;
	TMDisclosureFramework df;
};
static TMDisclosureFramework tm_lookup_disclosure_framework(TM self,const char *name);

static const char *log_levels[] = {
	"",
	"CRITICAL",
	"ERROR",
	"WARNING",
	"INFO",
	"DEBUG"
};

TMError tm_init(TM *ptm,const char *df_name,void *df_arg)
{
        TM tm;
        TM_NEW(tm);

	tm->log_level = TM_LOG_INFO;
	tm->log_level = TM_LOG_NOTHING;
	*ptm = tm;
	if( (tm->df = tm_lookup_disclosure_framework(tm,df_name)) == NULL)
		return (999);  /* FIXME what error code? */
	tm_log(tm,TM_LOG_INFO,"library handle initialized, using disclosure framework '%s'",df_name);
	
        return TM_OK;
}
void tm_cleanup(TM *ptm)
{
        assert(ptm && *ptm);
	tm_log(*ptm,TM_LOG_INFO,"library handle cleaned up");
        TM_FREE(*ptm);
}
void tm_log(TM tm,int level,const char *fmt,...)
{
        va_list args;
        assert(tm);
        assert(fmt);
	assert(level >= 0 && level <= TM_LOG_DEBUG);
        if(tm->log_level < level )
                return;
        fprintf(stderr,"[%s] ", log_levels[level]);
        va_start(args,fmt);
        vfprintf(stderr,fmt,args);
        va_end(args);
        fprintf(stderr,"\n");
}
int tm_set_loglevel(TM tm,int level)
{
        int tmp;
        assert(tm);
        tmp = tm->log_level;
        tm->log_level = level;
        return tmp;
}


const char *tm_set_error(TM self, const char *fmt,...)
{
        va_list args;
        assert(self);
        assert(fmt);
        va_start(args,fmt);
        vsnprintf(self->errstr,sizeof(self->errstr),fmt,args);
        va_end(args);
	return self->errstr;
}
const char * tm_get_error(TM self)
{
        assert(self);
        return self->errstr;
}

TMValueType tm_lookup_valuetype(TM self,const char *name)
{
	int i;
	for(i=0;vtypes[i];i++)
	{
        	if( strcmp(name,vtypes[i]->name) == 0) 
			return vtypes[i];
	}
	tm_set_error(self,"cannot find a value type named %s" , name);
        return NULL;
}


TMTopicMapStorageDescriptor tm_lookup_storage_descriptor(TM self,const char *name)
{
	int i;
	for(i=0;storages[i];i++)
	{
        	if( strcmp(name,storages[i]->name) == 0) 
			return storages[i];
	}
	tm_set_error(self,"cannot find a storage descriptor named %s" , name);
        return NULL;
}

TMDisclosureFramework tm_lookup_disclosure_framework(TM self,const char *name)
{
	int i;
	for(i=0;dfs[i];i++)
	{
        	if( strcmp(name,dfs[i]->name) == 0) 
			return dfs[i];
	}
	tm_set_error(self,"cannot find a disclosure framework named %s" , name);
        return NULL;
}

TMDisclosureFramework tm_get_disclosureframework(TM self)
{
	assert(self);
	return self->df;
}
