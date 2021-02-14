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
static TMValueStore _memtopicsetstore_new(void *data,TMProperty prop, void* param);
static TMError _memtopicsetstore_open(TMValueStore self);
static void _memtopicsetstore_delete(TMValueStore self);
static TMError _memtopicsetstore_close(TMValueStore self);


/* TMValueStore API: */
static TMError _memtopicsetstore_lookup_vid(TMValueStore s,const void *v,int flag,TMVid *pvid);
static TMError _memtopicsetstore_get_value(TMValueStore s,TMVid vid, void** vp);
static TMError _memtopicsetstore_delete_value(TMValueStore s,TMVid vid);
static TMError _memtopicsetstore_print(TMValueStore s,FILE *f);
static TMError _memtopicsetstore_scan_open(TMValueStore self, int opcode, const void *arg, TMValueStoreScan *sp);
static TMError _memtopicsetstore_scan_close(TMValueStore s, TMValueStoreScan scan);
static TMError _memtopicsetstore_scan_prepare_next(TMValueStore s, TMValueStoreScan scan);


extern struct TMValueType TopicSet;

struct TMValueStoreType default_mem_topicset_store_type = {
	&TopicSet,
		/* TMValueStore API */
	_memtopicsetstore_new,
	_memtopicsetstore_delete,
	_memtopicsetstore_open,
	_memtopicsetstore_close,

	_memtopicsetstore_lookup_vid,
	_memtopicsetstore_delete_value,
	_memtopicsetstore_get_value,

	_memtopicsetstore_print,
	_memtopicsetstore_scan_open,
	_memtopicsetstore_scan_close,
	_memtopicsetstore_scan_prepare_next,

};
#define INIT_SIZE 100
struct default_mem_topicset_store {
	TMValueStoreType type;
	struct data *data;
	TMValueType vtype;
	int count;
	int size;
	TMTopicSet *topicsets;
	/*
	TMTable tab;
	*/
	
};

typedef struct default_mem_topicset_store *T;

static int _topicsets_find_value(T self,const void* v,TMVid *pvid);

TMValueStore _memtopicsetstore_new(void *data,TMProperty prop, void*param)
{
	T self;
	int i;
	TMTRACE(TM_STORAGE_TRACE,"enter [prop=%s,vtype=%s]\n" _
			prop->name _ prop->value_type->name);
	TM_NEW(self);
	self->type =  &default_mem_topicset_store_type;
	self->data = (struct data *)data;
	self->vtype = prop->value_type;
	self->count = 0;
	self->topicsets = (TMTopicSet*)TM_ALLOC(INIT_SIZE * sizeof(TMTopicSet) );
	self->size = INIT_SIZE;
	for(i=0;i<INIT_SIZE;i++)
		self->topicsets[i] = NULL;
	/*
	self->tab = tm_table_new(10000,tm_strcmp_v,tm_strhash_v);
	*/


	TMTRACE(TM_STORAGE_TRACE,"exit\n");

	return (TMValueStore)self;
}

TMError _memtopicsetstore_open(TMValueStore s)
{
	T self;
	self = (T)s;
	assert(self);
	return TM_OK;
}
TMError _memtopicsetstore_close(TMValueStore s)
{
	T self;
	self = (T)s;
	assert(self);
	return TM_OK;
}
void _memtopicsetstore_delete(TMValueStore s)
{
	T self;
	int i;
	self = (T)s;
	assert(self);
	for(i=1;i<=self->count;i++)
	{
		if(self->topicsets[i])
		{
			tm_topicset_delete(&(self->topicsets[i]));
		}
	}
	/*
	tm_table_delete(&(self->tab));
	*/
	TM_FREE(self->topicsets);
	TM_FREE(self);
}



TMError _memtopicsetstore_lookup_vid(TMValueStore s,const void *v,int flag,TMVid *pvid)
{
	T self;
	assert(s);
	self = (T)s;
	assert(v);
	assert(pvid);
	*pvid = 0;

	TMTRACE(TM_STORAGE_TRACE,"enter\n");

	if(flag != TM_VALUE_CREATE)
	{
		if(_topicsets_find_value(self,v,pvid))
		{
			TMTRACE(TM_STORAGE_TRACE,"found vid (%d), returning\n" _ *pvid);
			return TM_OK;
		}
	}
	if(flag == TM_VALUE_LOOKUP)
		return TM_OK;

	TMTRACE(TM_STORAGE_TRACE,"creating new value\n");
	self->count++;
	if(self->count >= self->size)
	{
		int i;
		self->size *= 2;
		TM_RESIZE(self->topicsets,(self->size * sizeof(TMTopicSet)) );

		for(i=self->count;i<self->size;i++)
			self->topicsets[i] = NULL;

		TMTRACE(TM_STORAGE_TRACE,"topicset store: resize to %d\n" _ self->size);
	}

	self->topicsets[self->count] = tm_topicset_copy((TMTopicSet)v);
	/*
	tm_table_put(self->tab,self->topicsets[self->count],(void*)self->count);
	*/
	/*
	TMTRACE(TM_STORAGE_TRACE,"topicset store: added %d -> " _ self->count);
	tm_topicset_print(self->topicsets[self->count],stderr);
	TMTRACE(TM_STORAGE_TRACE,"\n");
	*/

	*pvid = self->count;

	return TM_OK;
}
TMError _memtopicsetstore_get_value(TMValueStore s,TMVid vid, void** vp)
{
	T self;
	assert(s);
	self = (T)s;

	assert(vid > 0);
	assert(vid <= self->count);
	assert(self->topicsets[vid]);	/* caller mustr make sure no deleted values are getted */
	*vp = tm_topicset_copy(self->topicsets[vid]);

	return TM_OK;
}
TMError _memtopicsetstore_delete_value(TMValueStore s,TMVid vid)
{
	T self;
	assert(s);
	self = (T)s;

	assert(vid > 0);
	assert(vid <= self->count);
	assert(self->topicsets[vid]);	/* caller mustr make sure no to-be-deleted values exist */
	/*
	tm_table_remove(self->tab,self->topicsets[vid]);
	*/
	tm_topicset_delete(&(self->topicsets[vid]));
	self->topicsets[vid] = NULL;

	return TM_OK;
}

TMError _memtopicsetstore_print(TMValueStore s,FILE *f)
{
        T self;
        int i;
	/*
	char buf[1024];
	*/
        assert(s);
        self = (T)s;
	
 
        fprintf(f,"TopicSet Store\n\n");
 
        for(i = 1; i<= self->count; i++)
        {
		if(self->topicsets[i])
		{
                	fprintf(f,"%6d -> ", i);
			tm_topicset_print(self->topicsets[i],f);
			fprintf(f,"\n");
		}
		else
		{
                	fprintf(f,"%6d -> [deleted]\n", i);
		}
        }
	return TM_OK;
}


/* -----------------------------------------------------------------------
 *
 *                             STATIC FUNCTIONS
 *
 * ----------------------------------------------------------------------- */

int _topicsets_find_value(T self,const void* v,TMVid *pvid)
{
	int i;
	assert(v);
	TMTRACE(TM_STORAGE_TRACE,"enter\n");
	/*
	*pvid = (TMVid)tm_table_get(self->tab,v);
	return (*pvid != 0);
	*/

	for(i=1;i<=self->count; i++)
	{
		if(!self->topicsets[i])
			continue;

		if( tm_topicset_equal(self->topicsets[i],(TMTopicSet)v) )
		{
			*pvid = i;
			TMTRACE(TM_STORAGE_TRACE,"exit found (vid=%d)\n" _ i);
			return (1);
		}
	}
	TMTRACE(TM_STORAGE_TRACE,"exit not found\n");
	return (0);
}
TMError _memtopicsetstore_scan_open(TMValueStore s, int opcode, const void *arg, TMValueStoreScan *sp)
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
	case TM_OP_TOPICSET_VALUE_INCLUDES_TOPICS:
	for(i=1; i<=self->count; i++)
	{
		if( ! self->topicsets[i] )
			continue;
		if( tm_topicset_contains(self->topicsets[i], (TMTopic)scan->arg) )
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
TMError _memtopicsetstore_scan_close(TMValueStore s, TMValueStoreScan scan)
{
	TMTRACE(TM_STORAGE_TRACE,"enter\n");
	TM_FREE(scan);
	TMTRACE(TM_STORAGE_TRACE,"exit\n");
	return TM_OK;
}
TMError _memtopicsetstore_scan_prepare_next(TMValueStore s, TMValueStoreScan scan)
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
		if( ! self->topicsets[i] )
			continue;
		if( tm_topicset_contains(self->topicsets[i], (TMTopic)scan->arg) )
		{
			scan->next_vid = i;
			break;
		}
			

	}
	scan->data = (void*)i;  /* index */
	return TM_OK;
}

