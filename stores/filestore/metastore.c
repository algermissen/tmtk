#include <tm.h>
#include <tmmem.h>
#include <tmtrace.h>

#include <tmstorage.h> 
#include "storage.h"
#include "metapage.h"
#include "tmfile.h"
#include "descriptor.h"                                                         

#include <assert.h>

/* TMMetaStore API: */
static TMMetaStore _filemetastore_new(void *handle);
static TMError _filemetastore_open(TMMetaStore self);
static void _filemetastore_delete(TMMetaStore self);
static TMError _filemetastore_close(TMMetaStore self);

static TMError _load_model_list(TMMetaStore self,TMList *);
static TMError _add_model(TMMetaStore self,const char*name);
static TMError _get_new_topic(TMMetaStore self,TMTopic *np);
static TMError _dispose_topic(TMMetaStore self,TMTopic topic);


struct TMMetaStoreType default_file_meta_store_type = {
		/* TMMetaStore API */
	_filemetastore_new,
	_filemetastore_delete,
	_filemetastore_open,
	_filemetastore_close,

	_load_model_list,
	_add_model,
	_get_new_topic,
	_dispose_topic
};

struct default_file_meta_store {
	TMMetaStoreType type;
	struct data *data;
	TMFile file;
};
typedef struct default_file_meta_store *T;


TMMetaStore _filemetastore_new(void *data)
{
	char buf[256];  /* FIXME */
	T self;
	TM_NEW(self);
	self->type =  &default_file_meta_store_type;
	self->data = (struct data *)data;
	snprintf(buf,sizeof(buf),"%s/meta",self->data->path);
	self->file = tm_file_new(buf,0 /* use min cache size */);
	assert(self->file);
	return (TMMetaStore)self;
}

TMError _filemetastore_open(TMMetaStore s)
{
	int cflag;
	int e;
	char *path;
	/*
	double cache_size = 10 * GS_PAGE_SIZE;
	*/
	T self;

	TMTRACE(TM_STORAGE_TRACE,"enter\n");

	self = (T)s;
	assert(self);
	path = self->data->path;

	/* FIXME: check number of open files and what? */
	

        if( (e = tm_file_open(self->file,&cflag)) != TM_OK)
        {
                TMTRACE(TM_STORAGE_TRACE,"can't open metafile: %s\n" _
				tm_file_get_error(self->file));

		tm_topicmap_set_error(self->data->topicmap,
				"can't open metafile: %s",
				tm_file_get_error(self->file));
                return e;
        }
	if(cflag) {
		int pnum;
		/*
		 * Create three pages:
		 * 1) model list
		 * 2) topic freelist
		 * 3) assertion types list
		 */
		tm_file_create_page(self->file,gs_page_init,&pnum);
		assert(pnum == 1);
		tm_file_create_page(self->file,gs_page_init,&pnum);
		assert(pnum == 2);
		tm_file_create_page(self->file,gs_page_init,&pnum);
		assert(pnum == 3);

		/* initialize file */

	}
	TMTRACE(TM_STORAGE_TRACE,"exit\n");
	return TM_OK;
}
TMError _filemetastore_close(TMMetaStore s)
{
	T self;
	TMError e;
	self = (T)s;
	assert(self);
	if( (e = tm_file_flush_cache(self->file)) != TM_OK)
	{
		tm_topicmap_set_error(self->data->topicmap,
				"can't flush metafile: %s",
				tm_file_get_error(self->file));
		return e;
	}
		
	return ( tm_file_close(self->file) );
}
void _filemetastore_delete(TMMetaStore s)
{
	T self;
	self = (T)s;
	assert(self);
	tm_file_delete(&self->file);
	TM_FREE(self);
}



TMError _load_model_list(TMMetaStore s,TMList *listp)
{
	int i,cnt;
	TMPage page;
	TMList list = NULL;
	T self;
	assert(s);
	assert(listp);

	self = (T)s;

	*listp = NULL;

	tm_file_fetch_page(self->file,1,&page);

	cnt = tm_metapage_get_number_of_models(page);
	for(i=0;i<cnt;i++)
	{
		const char *name;
		name = tm_metapage_get_modelname(page,i);
		list = TMList_push(list,(void*)name);
	}
	tm_file_drop_page(self->file,page);
	
	*listp = list;
	
	return TM_OK;
}

TMError _add_model(TMMetaStore s,const char*name)
{
	TMPage page;
	T self;
	assert(s);
	assert(name);

	self = (T)s;

	tm_file_fetch_page(self->file,1,&page);
	tm_metapage_add_modelname(page,name);
	tm_file_drop_page(self->file,page);

	return TM_OK;
}

TMError _get_new_topic(TMMetaStore s,TMTopic *np)
{
	TMPage page;
	T self;
	assert(s);
	/* *np = 0; */	

	self = (T)s;

	tm_file_fetch_page(self->file,2,&page);
	*np = tm_metapage_get_new_topic(page);
	tm_file_drop_page(self->file,page);

	return TM_OK;


}
TMError _dispose_topic(TMMetaStore s,TMTopic topic)
{
	TMPage page;
	T self;
	assert(s);
	assert(topic);

	self = (T)s;

	tm_file_fetch_page(self->file,2,&page);
	tm_metapage_dispose_topic(page,topic);
	/* FIXME retval ?? */
	tm_file_drop_page(self->file,page);

	return TM_OK;


}
