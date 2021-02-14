#include <tm.h>
#include <tmassert.h>
#include <tmmem.h>
#include <tmtrace.h>
#include <tmutil.h>

#include <tmstorage.h> 
#include "descriptor.h"                                                         

/* TMStore API: */
static TMMetaStore _memmetastore_new(void *data);
static TMError _memmetastore_open(TMMetaStore self);
static void _memmetastore_delete(TMMetaStore self);
static TMError _memmetastore_close(TMMetaStore self);


/* TMMetaStore API: */
static TMError _memmetastore_load_model_list(TMMetaStore self,TMList *);
static TMError _memmetastore_add_model(TMMetaStore self,const char*name);
static TMError _memmetastore_get_new_topic(TMMetaStore self,TMTopic *np);
static TMError _memmetastore_dispose_topic(TMMetaStore self,TMTopic topic);
static TMError _memmetastore_get_topicmax(TMMetaStore self,TMTopic *np);

struct TMMetaStoreType default_mem_meta_store_type = {
		/* TMStore API */
	_memmetastore_new,
	_memmetastore_delete,
	_memmetastore_open,
	_memmetastore_close,

		/* TMMetaStore API */
	_memmetastore_load_model_list,
	_memmetastore_add_model,
	_memmetastore_get_new_topic,
	_memmetastore_dispose_topic,
	_memmetastore_get_topicmax
};

struct default_mem_meta_store {
	TMMetaStoreType type;
	struct data *data;

	TMTopic topic_max;
	TMList model_names;
	TMList topic_freelist;
};
typedef struct default_mem_meta_store *T;


TMMetaStore _memmetastore_new(void *data)
{
	T self;
	TM_NEW(self);
	self->type =  &default_mem_meta_store_type;
	self->data = (struct data *)data;
	self->model_names = NULL;
	self->topic_max = 0;
	self->topic_freelist = NULL;

	return (TMMetaStore)self;
}

TMError _memmetastore_open(TMMetaStore s)
{
	T self;

	TMTRACE(TM_STORAGE_TRACE,"enter\n");

	self = (T)s;

	TMTRACE(TM_STORAGE_TRACE,"exit\n");
	return TM_OK;
}
TMError _memmetastore_close(TMMetaStore s)
{
	T self;
	self = (T)s;
	assert(self);
	/* do nothing */
	return TM_OK;
}
void _memmetastore_delete(TMMetaStore s)
{
	T self;
	self = (T)s;
	assert(self);
	/* FIXME: free both lists */
	tm_list_delete(&self->model_names);
	tm_list_delete(&self->topic_freelist);
	TM_FREE(self);
}


TMError _memmetastore_load_model_list(TMMetaStore s,TMList *listp)
{
	T self;

	assert(s);
	assert(listp);

	self = (T)s;

	*listp = NULL;

	return TM_OK;
}

TMError _memmetastore_add_model(TMMetaStore s,const char*name)
{
	T self;
	assert(s);
	assert(name);

	self = (T)s;

	self->model_names = tm_list_push(self->model_names,
				tm_strdup(name));

	return TM_OK;
}

TMError _memmetastore_get_new_topic(TMMetaStore s,TMTopic *np)
{
	T self;
	assert(s);
	/* *np = 0; */	

	self = (T)s;

	if(self->topic_freelist)
	{
		self->topic_freelist = tm_list_pop(self->topic_freelist,(void**)np);
		return TM_OK;
	}

	/* FIXME: INT_MAX check !!! */

	self->topic_max++;
	*np = self->topic_max;

	return TM_OK;


}
TMError _memmetastore_dispose_topic(TMMetaStore s,TMTopic topic)
{
	T self;
	assert(s);
	assert(topic);

	self = (T)s;
	
	/* FIXME */
	self->topic_freelist = tm_list_push(self->topic_freelist,(void*)topic);

	return TM_OK;


}
TMError _memmetastore_get_topicmax(TMMetaStore s,TMTopic *np)
{
	T self;
	assert(s);
	self = (T)s;

	*np = self->topic_max;

	return TM_OK;


}

