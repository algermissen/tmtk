#include <tm.h>
#include <tmmem.h>
#include <tmtrace.h>
#include <tmutil.h>
#include <tmassert.h>

#include <tmstorage.h> 
#include <tmtable.h> 
#include "descriptor.h"                                                         

#define INIT_SIZE 100

#define ARRAY_INIT_SIZE 4
/* 3 is minimum, because we store size and length in first two elems */


/* TMPropertyStore API: */

static TMPropertyStore _mempropertystore_new(void *data,TMProperty prop,TMValueStore vstore);
static TMError _mempropertystore_open(TMPropertyStore self);
static void _mempropertystore_delete(TMPropertyStore self);
static TMError _mempropertystore_close(TMPropertyStore self);

static TMError _mempropertystore_insert(TMPropertyStore self,TMTopic, TMVid);
static TMError _mempropertystore_remove(TMPropertyStore self,TMTopic);
static TMError _mempropertystore_lookup_topic(TMPropertyStore self, TMVid, TMTopic *);
static TMError _mempropertystore_lookup_topics(TMPropertyStore s, TMVid vid, TMTopic **topicsp, int *lenp);
static TMError _mempropertystore_get_vid_of_topic(TMPropertyStore self, TMTopic,TMVid *);
static TMError _mempropertystore_print(TMPropertyStore s,FILE *f);
static TMError _mempropertystore_is_normalized(TMPropertyStore s);
static TMError _mempropertystore_normalize(TMPropertyStore s,TMTopicMap tm);

/* local helper functions, not part of API */
static TMTopic* _new_topics_array(void);
static int _can_append(TMTopic *a);
static void _append(TMTopic *a, TMTopic n);
static TMTopic *_resize(TMTopic *a);


struct TMPropertyStoreType default_mem_property_store_type = {
		/* TMStore API */
	_mempropertystore_new,
	_mempropertystore_delete,
	_mempropertystore_open,
	_mempropertystore_close,

	_mempropertystore_insert,
	_mempropertystore_remove,
	_mempropertystore_lookup_topic,
	_mempropertystore_lookup_topics,
	_mempropertystore_get_vid_of_topic,

	_mempropertystore_print,
	_mempropertystore_is_normalized,
	_mempropertystore_normalize,
};

struct default_mem_property_store {
	TMPropertyStoreType type;
	struct data *data;

	TMProperty property;
	int is_normalized;
	TMTopic **vid_to_topiclist_array;


	TMVid size;
	TMTopic *topics;
	TMTopic **topic_arrays;
	TMValueStore vstore;

	TMTable vid_by_topic_table;

};
/* for convenience we use T in here */
typedef struct default_mem_property_store *T;


TMPropertyStore _mempropertystore_new(void *data, TMProperty prop,TMValueStore vstore)
{
	T self;
	int i;


	TMTRACE(TM_STORAGE_TRACE,"enter\n");
	assert(prop);
	TM_NEW(self);
	self->type =  &default_mem_property_store_type;
	self->data = (struct data *)data;
	self->property = prop;
	self->vstore = vstore;
	self->is_normalized = 1; /* we are empty! */

	TMTRACE(TM_STORAGE_TRACE,"property is %s %s\n" _
		self->property->model _ self->property->name);

	assert(ARRAY_INIT_SIZE >= 3);
	self->vid_to_topiclist_array = (TMTopic**)TM_ALLOC(INIT_SIZE * sizeof(TMTopic*) );
	self->size = INIT_SIZE;
	for(i=0;i<INIT_SIZE;i++)
		self->vid_to_topiclist_array[i] = NULL;
	self->vid_by_topic_table = tm_table_new(10000,NULL,NULL);

	TMTRACE(TM_STORAGE_TRACE,"exit\n");
	return (TMPropertyStore)self;
}

TMError _mempropertystore_open(TMPropertyStore s)
{
	T self;

	TMTRACE(TM_STORAGE_TRACE,"enter\n");

	self = (T)s;

	TMTRACE(TM_STORAGE_TRACE,"exit\n");
	return TM_OK;
}
TMError _mempropertystore_close(TMPropertyStore s)
{
	T self;
	self = (T)s;
	assert(self);
	/* do nothing */
	return TM_OK;
}
void _mempropertystore_delete(TMPropertyStore s)
{
	T self;
	int i;
	self = (T)s;
	assert(self);
	for(i = 0; i< self->size; i++)
	{
		if(self->vid_to_topiclist_array[i] != NULL)
			TM_FREE(self->vid_to_topiclist_array[i]);
	}
	TM_FREE(self->vid_to_topiclist_array);
	TM_FREE(self);
	
}



TMError _mempropertystore_insert(TMPropertyStore s,TMTopic topic, TMVid vid)
{
	T self;
	assert(s);
	self = (T)s;
	TMTRACE(TM_STORAGE_TRACE,"topic=%d vid=%d\n" _ topic _ vid);
	assert(tm_table_get(self->vid_by_topic_table,(void*)topic) == NULL);
	
	if(vid >= self->size)
	{
		int i;
		int old_size = self->size;
		TMTRACE(TM_STORAGE_TRACE,"vid=%d > size (%d)\n" _  vid _ self->size );
		do {
			self->size *= 2;
		} while(vid >= self->size);
		TMTRACE(TM_STORAGE_TRACE,"size now %d\n" _  self->size );

		TMTRACE(TM_STORAGE_TRACE,"OP 1\n");
		TM_RESIZE(self->vid_to_topiclist_array, self->size * sizeof(TMTopic*) );
		TMTRACE(TM_STORAGE_TRACE,"OP 2\n");
		for(i = old_size; i < self->size; i++)
		{
			self->vid_to_topiclist_array[i] = NULL;
		}
		TMTRACE(TM_STORAGE_TRACE,"OP 3\n");
	}
	assert(vid < self->size);


	if(self->vid_to_topiclist_array[vid] == NULL)
	{
		self->vid_to_topiclist_array[vid] = _new_topics_array();
		assert(self->vid_to_topiclist_array[vid][0] == ARRAY_INIT_SIZE);
	}
	if( ! _can_append(self->vid_to_topiclist_array[vid]) )
		self->vid_to_topiclist_array[vid] = _resize(self->vid_to_topiclist_array[vid]);

	_append(self->vid_to_topiclist_array[vid],topic);

	tm_table_put(self->vid_by_topic_table,(void*)topic, (void*)vid);
	assert(self->vid_to_topiclist_array[vid][self->vid_to_topiclist_array[vid][1]+1 ] == topic);
	self->is_normalized = 0;
	return TM_OK;
}
TMError _mempropertystore_remove(TMPropertyStore s,TMTopic topic)
{
	T self;
	TMVid vid;
	int n;
	TMTopic *array;
	assert(s);
	self = (T)s;
	TMTRACE(TM_STORAGE_TRACE,"enter topic=%d \n" _ topic);
	assert(tm_table_get(self->vid_by_topic_table,(void*)topic) != NULL);
	vid = (TMVid)tm_table_get(self->vid_by_topic_table,(void*)topic);
	array = self->vid_to_topiclist_array[vid];
	n = array[1];
	if(n == 1)
	{
		TMTRACE(TM_STORAGE_TRACE,"topic %d is last one for value => delete value too\n" _ topic);
		TM_FREE(array);
		self->vid_to_topiclist_array[vid] = NULL;
		tm_value_store_delete_value(self->vstore,vid);
	}
	else
	{
		int i,j,found = 0;
		TMTRACE(TM_STORAGE_TRACE,"more topics on the value of %d => keep value \n" _ topic);
		for(i=0;i<n;i++)
		{
			if( array[i+2] == topic)
			{
				found = 1;
				break;
			}
	
		}
		assert(found); /* just checking code */
		assert(i < n);
		if(i == (n-1))
		{
			assert(array[n-1 + 2] == topic);
			array[n-1 + 2] = 0;
		}
		else
		{
			for(j = i; j<(n-1); j++)
				array[j+2] = array[j+2+1];
		}
		n--;
		array[1] = n;
		TMTRACE(TM_STORAGE_TRACE,"array is now: ");
		for(i=0;i<n;i++)
		{
			TMTRACE(TM_STORAGE_TRACE,"%d " _ array[i+2] );
		}
			TMTRACE(TM_STORAGE_TRACE,"\n");

	}

	
	tm_table_remove(self->vid_by_topic_table,(void*)topic);
	/* FIXME ??
	self->is_normalized = 0;
	*/
	return TM_OK;
}

TMError _mempropertystore_lookup_topic(TMPropertyStore s, TMVid vid, TMTopic *np)
{
	T self;
	TMTopic *array;
	assert(s);
	self = (T)s;
	*np = 0;
	TMTRACE(TM_STORAGE_TRACE,"vid=%d\n" _ vid);

	/* this function may only be called for TM_SIDPs */
	assert(self->property->type == TM_SIDP);
	assert(self->is_normalized);

	assert(vid < self->size);
	/* -- assertion must holed true - or? 
	if( vid >= self->size)
		return TM_OK;
	*/
	assert(self->vid_to_topiclist_array[vid]); /* is my assumption correct??? */
	array = self->vid_to_topiclist_array[vid];
	assert(array[1] > 0);
	assert(array[1] == 1);
	*np = array[2];

	return TM_OK;
}
TMError _mempropertystore_lookup_topics(TMPropertyStore s, TMVid vid, TMTopic **topicsp, int *lenp)
{
	T self;
	TMTopic *array;
	assert(s);
	self = (T)s;
	TMTRACE(TM_STORAGE_TRACE,"enter [vid=%d]\n" _ vid);
	*topicsp = NULL;
	*lenp = 0;

	assert(vid < self->size);
	/* -- assertion must holed true - or? 
	if( vid >= self->size)
		return TM_OK;
	*/
	assert(self->vid_to_topiclist_array[vid]); /* is my assumption correct??? */
	array = self->vid_to_topiclist_array[vid];
	assert(array[1] > 0);
	{/* FIXME: debug code */
		int i,len;
		len = array[1];
		TMTRACE(TM_STORAGE_TRACE,"array is: ");
		for(i=0;i<len;i++)
		{
			TMTRACE(TM_STORAGE_TRACE,"%d " _ array[i+2] );
		}
		TMTRACE(TM_STORAGE_TRACE,"\n");
	}
	*lenp = array[1];
	*topicsp = array+2;
	{/* FIXME: debug code */
		int i,len;
		len = *lenp;
		TMTRACE(TM_STORAGE_TRACE,"array is: ");
		for(i=0;i<len;i++)
		{
			TMTRACE(TM_STORAGE_TRACE,"%d " _ (*topicsp)[i] );
		}
		TMTRACE(TM_STORAGE_TRACE,"\n");
	}
	
	return TM_OK;
}

TMError  _mempropertystore_get_vid_of_topic(TMPropertyStore s, TMTopic topic, TMVid *vidp)
{
	T self;

	assert(s);
	self = (T)s;
	assert(topic);
	*vidp = 0;
	TMTRACE(TM_STORAGE_TRACE,"topic=%d\n" _ topic);

	*vidp = (TMVid)tm_table_get(self->vid_by_topic_table,(void*)topic);
	return TM_OK;
}


TMError _mempropertystore_print(TMPropertyStore s,FILE *f)
{
	T self;
	int i;
	assert(s);
	self = (T)s;
#if 0 

	fprintf(f,"\n\nProperty Store (%s::%s) View A\n", self->property->model,self->property->name);
	fprintf(f,"-------------------------------------------------\n");
	fprintf(f,"  (The vid corresponds to the vid of the property's value store\n");
	fprintf(f,"  vid ->    topics\n");
	for(i=1;i<self->size;i++)
	{
		int j;
		TMTopic *a;
		if(! self->vid_to_topiclist_array[i])
			continue;
		a = self->vid_to_topiclist_array[i];	
		fprintf(f,"%6d -> ", i);
		for(j=0;j<a[1];j++)
		{
			fprintf(f,"%6d ", a[j+2] );
		}
		fprintf(f,"\n");
	}

	fprintf(f,"\n\n");
#endif
#if 1 
	fprintf(f,"\n\n[Property Store (%s::%s) View B]\n", self->property->model,self->property->name);
	fprintf(f,";-------------------------------------------------\n");
	fprintf(f,"; topic -> value [%s]\n", self->property->value_type->name);
	for(i=1;i<self->size;i++)
	{
		int j,n;
		void *value;
		TMTopic *a;
		char vbuf[4096];
		if(! self->vid_to_topiclist_array[i])
			continue;

		tm_value_store_get_value(self->vstore,i,&value);
		n = tm_value_to_string(self->property->value_type,value,vbuf,sizeof(vbuf));
		/* FIXME
		assert(n < sizeof(vbuf));
		*/
		
		a = self->vid_to_topiclist_array[i];	
		for(j=0;j<a[1];j++)
		{
			fprintf(f,"%6d=%s\n", a[j+2] , vbuf );
		}
		tm_value_delete(self->property->value_type,&value);

	}

	fprintf(f,";-------------------------------------------------\n");
	fprintf(f,"\n\n");
#endif

	return TM_OK;
}
TMError _mempropertystore_is_normalized(TMPropertyStore s)
{
	assert(s);
	
	return ( ( ((T)s)->property->type == TM_OP) || ( ((T)s)->is_normalized ) );
}
TMError _mempropertystore_normalize(TMPropertyStore s,TMTopicMap topicmap)
{
	T self;
	int i;
	TMTopic stays;
	assert(s);
	self = (T)s;

	if(self->property->type != TM_SIDP)
		return TM_OK;
	if(self->is_normalized)
		return TM_OK;


	for(i=1;i<self->size;i++)
	{
		TMTopic *a;
		int j;
		if(! self->vid_to_topiclist_array[i])
			continue;

		assert( self->vid_to_topiclist_array[i][1] > 0 ); /* just a check of my thinking */
		/* if there is no list, we needn't process this one */
		if( self->vid_to_topiclist_array[i][1] == 1)
			continue;


		a = self->vid_to_topiclist_array[i];	
		stays = a[2];
		tm_topicmap_add_mergelist(topicmap,a+2,a[1]);
		/*----
		for(j=0;j<a[1];j++)
		{
			fprintf(f,"%6d -> %s\n", a[j+2] , vbuf );
		}
		*/
		/* start from one because we don't remove first one */
		for(j=1;j<a[1];j++)
		{
			tm_table_remove(self->vid_by_topic_table,(void*)a[j+2]);
			/*
			fprintf(f,"%6d -> %s\n", a[j+2] , vbuf );
			*/
		}
		TM_RESIZE(a,3 * sizeof(TMTopic) );
		a[0] = 3;
		a[1] = 1;
		assert(a[2] == stays);

	}
	self->is_normalized = 1;

	return TM_OK;
}



TMTopic* _new_topics_array(void)
{
	TMTopic *a;
	a = (TMTopic*)TM_ALLOC(ARRAY_INIT_SIZE * sizeof(TMTopic));
	a[0] = ARRAY_INIT_SIZE;	/* current size */
	a[1] = 0;		/* current length */
	return a;
}
int _can_append(TMTopic *a)
{
	assert(a);
	return ( (a[0] - 2) > a[1] );
}
TMTopic *_resize(TMTopic *a)
{
	int size;
	assert(a);
	size = a[0];
	size *= 2;
	TM_RESIZE(a,size * sizeof(TMTopic) );
	a[0] = size;
	return a;
}
void _append(TMTopic *a, TMTopic n)
{
	assert(a);
	assert(n);
	assert( _can_append(a) );

	a[ 2 + a[1] ] = n;
	a[1]++;
}

