#include <tm.h>
#include <tmassert.h>
#include <tmmem.h>
#include <tmtrace.h>
#include <tmutil.h> 
#include <tmstorage.h> 
#include <tmtable.h> 
#include <tmbasictypes.h> 

#include "descriptor.h"                                                         


/* TMStore API: */
static TMValueStore _memtextstore_new(void *data,TMProperty prop, void* param);
static TMError _memtextstore_open(TMValueStore self);
static void _memtextstore_delete(TMValueStore self);
static TMError _memtextstore_close(TMValueStore self);


/* TMValueStore API: */
static TMError _memtextstore_lookup_vid(TMValueStore s,const void *v,int flag,TMVid *pvid);
static TMError _memtextstore_get_value(TMValueStore s,TMVid vid, void** vp);
static TMError _memtextstore_delete_value(TMValueStore s,TMVid vid);
static TMError _memtextstore_print(TMValueStore s,FILE *f);
static TMError _memtextstore_scan_open(TMValueStore self, int opcode, const void *arg, TMValueStoreScan *sp);
static TMError _memtextstore_scan_close(TMValueStore s, TMValueStoreScan scan);
static TMError _memtextstore_scan_prepare_next(TMValueStore s, TMValueStoreScan scan);


extern struct TMValueType Text;

struct TMValueStoreType default_mem_text_store_type = {
	&Text,
		/* TMValueStore API */
	_memtextstore_new,
	_memtextstore_delete,
	_memtextstore_open,
	_memtextstore_close,

	_memtextstore_lookup_vid,
	_memtextstore_delete_value,
	_memtextstore_get_value,

	_memtextstore_print,
	_memtextstore_scan_open,
	_memtextstore_scan_close,
	_memtextstore_scan_prepare_next,

};
#define INIT_SIZE 100
struct default_mem_text_store {
	TMValueStoreType type;
	struct data *data;
	TMValueType vtype;
	int count;
	int size;
	char **texts;
	TMTable tab;
	
};

typedef struct default_mem_text_store *T;

static int _texts_find_value(T self,const void* v,TMVid *pvid);

TMValueStore _memtextstore_new(void *data,TMProperty prop, void*param)
{
	T self;
	int i;
	TMTRACE(TM_STORAGE_TRACE,"enter [prop=%s,vtype=%s]\n" _
			prop->name _ prop->value_type->name);
	TM_NEW(self);
	self->type =  &default_mem_text_store_type;
	self->data = (struct data *)data;
	self->vtype = prop->value_type;
	self->count = 0;
	self->texts = (char**)TM_ALLOC(INIT_SIZE * sizeof(char*) );
	self->size = INIT_SIZE;
	for(i=0;i<INIT_SIZE;i++)
		self->texts[i] = NULL;
	self->tab = tm_table_new(10000,tm_strcmp_v,tm_strhash_v);


	TMTRACE(TM_STORAGE_TRACE,"exit\n");

	return (TMValueStore)self;
}

TMError _memtextstore_open(TMValueStore s)
{
	T self;
	self = (T)s;
	assert(self);
	return TM_OK;
}
TMError _memtextstore_close(TMValueStore s)
{
	T self;
	self = (T)s;
	assert(self);
	return TM_OK;
}
void _memtextstore_delete(TMValueStore s)
{
	T self;
	int i;
	self = (T)s;
	assert(self);
	for(i=1;i<=self->count;i++)
	{
		if(self->texts[i])
		{
			TM_FREE(self->texts[i]);
		}
	}
	tm_table_delete(&(self->tab));
	TM_FREE(self->texts);
	TM_FREE(self);
}



TMError _memtextstore_lookup_vid(TMValueStore s,const void *v,int flag,TMVid *pvid)
{
	T self;
	assert(s);
	self = (T)s;
	assert(v);
	assert(pvid);
	*pvid = 0;

	if(flag != TM_VALUE_CREATE)
	{
		if(_texts_find_value(self,v,pvid))
			return TM_OK;
	}
	if(flag == TM_VALUE_LOOKUP)
		return TM_OK;

	self->count++;
	if(self->count >= self->size)
	{
		int i;
		self->size *= 2;
		TM_RESIZE(self->texts,(self->size * sizeof(char*)) );

		for(i=self->count;i<self->size;i++)
			self->texts[i] = NULL;

		TMTRACE(TM_STORAGE_TRACE,"text store: resize to %d\n" _ self->size);
	}

	self->texts[self->count] = tm_strdup((char*)v);
	tm_table_put(self->tab,self->texts[self->count],(void*)self->count);
	TMTRACE(TM_STORAGE_TRACE,"text store: added %d -> %s\n" _
		self->count _ self->texts[self->count]);

	*pvid = self->count;

	return TM_OK;
}
TMError _memtextstore_get_value(TMValueStore s,TMVid vid, void** vp)
{
	T self;
	assert(s);
	self = (T)s;

	assert(vid > 0);
	assert(vid <= self->count);
	assert(self->texts[vid]);	/* caller mustr make sure no deleted values are getted */
	*vp = tm_strdup(self->texts[vid]);

	return TM_OK;
}
TMError _memtextstore_delete_value(TMValueStore s,TMVid vid)
{
	T self;
	assert(s);
	self = (T)s;

	assert(vid > 0);
	assert(vid <= self->count);
	assert(self->texts[vid]);	/* caller mustr make sure no to-be-deleted values exist */
	tm_table_remove(self->tab,self->texts[vid]);
	TM_FREE(self->texts[vid]);
	self->texts[vid] = NULL;

	return TM_OK;
}

/* FIXME: API handling of allocatioon balance !!!! */
/*
 *
 * GET ALL x WHERE to_lower(x) == to_lower(Vq)
 *
 * GET ALL x WHERE NOT disjoint(x,Vq)
 *
 * -value type definiert operator/method
 * -store implementiert op etc auf VID basis (also im store selber!!) aber auch mit
 *  externem value!!!
 *
 *
 *
 */
#if 0
TMError _memtextstore_get_vids_like(TMValueStore s,const void* v,TMTopicSet set)
{
	/* FIXME */
	T self;
	int i;
	assert(s);
	self = (T)s;
	
	for(i=1;i<=self->count; i++)
	{
		assert(self->texts[i]);
		if( strstr(self->texts[i],(char*)v) != NULL )
		{
			tm_topicset_add(set,(TMTopic)i);	
		}
	}

	return TM_OK;
}
#endif

TMError _memtextstore_print(TMValueStore s,FILE *f)
{
        T self;
        int i;
        assert(s);
        self = (T)s;
 
        fprintf(f,"Text Store\n\n");
 
        for(i = 1; i<= self->count; i++)
        {
                fprintf(f,"%6d -> %s\n", i, self->texts[i] ? self->texts[i] : "[deleted]" );
        }
	return TM_OK;
}


/* -----------------------------------------------------------------------
 *
 *                             STATIC FUNCTIONS
 *
 * ----------------------------------------------------------------------- */

int _texts_find_value(T self,const void* v,TMVid *pvid)
{
	int i;
	assert(v);
	*pvid = (TMVid)tm_table_get(self->tab,v);
	return (*pvid != 0);

	for(i=1;i<=self->count; i++)
	{
		assert(self->texts[i]);
		/* FIXME: we'll have deletes later on, so this assertion will fail then! */
		if(strcmp(self->texts[i],(char*)v) == 0)
		{
			*pvid = i;
			return (1);
		}
	}
	return (0);
}
TMError _memtextstore_scan_open(TMValueStore s, int opcode, const void *arg, TMValueStoreScan *sp)
{
	T self;
	TMValueStoreScan scan;
	int i;
	assert(s);
	self = (T)s;


	TMTRACE(TM_STORAGE_TRACE,"enter\n");
	TM_NEW(scan);
	scan->op = opcode;
	scan->arg = arg;
	scan->next_vid = 0; /* preset so that nothing to be done after loop terminates */

	switch(opcode) {
	case TM_OP_Text_LIKE:
	for(i=1; i<=self->count; i++)
	{
		if( ! self->texts[i] )
			continue;
		if( tm_strstr(self->texts[i], (char*)scan->arg, TM_CASE_INSENSITIVE) != NULL )
		{
			scan->next_vid = i;
			break;
		}
			

	}
	scan->data = (void*)i;  /* index */
		break;
	}
	/*
	--------------------------- go on here -----------------
	*/
	*sp = scan;

	TMTRACE(TM_STORAGE_TRACE,"exit\n");
	return TM_OK;
}
TMError _memtextstore_scan_close(TMValueStore s, TMValueStoreScan scan)
{
	TMTRACE(TM_STORAGE_TRACE,"enter\n");
	TM_FREE(scan);
	TMTRACE(TM_STORAGE_TRACE,"exit\n");
	return TM_OK;
}
TMError _memtextstore_scan_prepare_next(TMValueStore s, TMValueStoreScan scan)
{
	int i;
	T self;
	assert(s);
	self = (T)s;
	if( (int)(scan->data) > self->count)
	{
		scan->next_vid = 0;
	}
	scan->next_vid = 0; /* presetting (see above) */
	for(i=((int)scan->data)+1 ; i<=self->count; i++)
	{
		if( ! self->texts[i] )
			continue;
		if( tm_strstr(self->texts[i], (char*)scan->arg, TM_CASE_INSENSITIVE) != NULL )
		{
			scan->next_vid = i;
			break;
		}
			

	}
	scan->data = (void*)i;  /* index */
	return TM_OK;
}

