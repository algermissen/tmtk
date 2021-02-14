#include <tm.h>
#include <mem.h>
#include <trace.h>

#include <tmstorage.h> 

#include "storage.h"
#include "tmfile.h"
#include "descriptor.h"                                                         

/* TMStore API: */
static void* _new(void *data,va_list args);
static TMError _open(void* self);
static void _delete(void* self);
static TMError _close(void* self);
static const char* _get_error(void* self);


/* TMValueStore API: */
/*
static TMError _load_model_list(TMMetaStore self,TMList *);
static TMError _add_model(TMMetaStore self,const char*name);
static TMError _get_new_topic(TMMetaStore self,TMTopic *np);
static TMError _dispose_topic(TMMetaStore self,TMTopic topic);
*/

struct TMValueStoreType default_file_locator_store_type = {
		/* TMStore API */
	_new,
	_delete,
	_open,
	_close,
	_get_error,
	"locator"

		/* TMMetaStore API */
	/*
	_load_model_list,
	_add_model,
	_get_new_topic,
	_dispose_topic
	*/
};
struct default_file_locator_store {
	TMValueStoreType type;
	struct data *data;
	TMFile file;
};
typedef struct default_file_locator_store *T;


void* _new(void *data,va_list args)
{
	char buf[TM_MAX_FILENAME_LEN];
	T self;
	TM_NEW(self);
	self->type =  &default_file_locator_store_type;
	self->data = (struct data *)data;
	snprintf(buf,sizeof(buf),"%s/locators",self->data->path);
	self->file = tm_file_new(buf,0 /* use min cache size */);
	assert(self->file);
	return (TMMetaStore)self;
}

TMError _open(void* s)
{
	int cflag;
	int e;
	char *path;
	/*
	double cache_size = 10 * GS_PAGE_SIZE;
	*/
	T self;

	GSTRACE(GS_STORAGE_TRACE,"_open(): enter\n");

	self = (T)s;
	assert(self);
	path = self->data->path;

	/* FIXME: check number of open files and what? */
	

        if( (e = tm_file_open(self->file,&cflag)) != TM_OK)
        {
                GSTRACE(GS_GRAPH_TRACE,"_open(): can't open metafile: %s\n" _
				tm_file_get_error(self->file));

		snprintf(self->data->errstr,sizeof(self->data->errstr),
				"can't open metafile: %s",
				tm_file_get_error(self->file));
                return e;
        }
#if 0
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
#endif
	GSTRACE(GS_STORAGE_TRACE,"_open(): exit\n");
	return TM_OK;
}
TMError _close(void* s)
{
	T self;
	TMError e;
	self = (T)s;
	assert(self);
	if( (e = tm_file_flush_cache(self->file)) != TM_OK)
	{
		snprintf(self->data->errstr,sizeof(self->data->errstr),
				"can't flush metafile: %s",
				tm_file_get_error(self->file));
		return e;
	}
		
	return ( tm_file_close(self->file) );
}
void _delete(void* s)
{
	T self;
	self = (T)s;
	assert(self);
	tm_file_delete(&self->file);
	TM_FREE(self);
}

const char *_get_error(void* s)
{
	T self;
	assert(s);
	self = (T)s;
	return self->data->errstr;
}

#if 0
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
		list = TMList_push(list,name);
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
#endif
