#include <tm.h>
#include <tmmem.h>
#include <tmtrace.h>
#include <tmutil.h> 
#include <tmstorage.h> 
#include <tmtable.h> 
#include <tmassert.h>
#include <tmbasictypes.h>

#include "descriptor.h"                                                         


static TMValueStore _memlocatorsetstore_new(void *data,TMProperty prop, void* param);
static TMError _memlocatorsetstore_open(TMValueStore self);
static void _memlocatorsetstore_delete(TMValueStore self);
static TMError _memlocatorsetstore_close(TMValueStore self);


static TMError _memlocatorsetstore_lookup_vid(TMValueStore s,const void *v,int flag,TMVid *pvid);
static TMError _memlocatorsetstore_get_value(TMValueStore s,TMVid vid, void** vp);
static TMError _memlocatorsetstore_delete_value(TMValueStore s,TMVid vid);
static TMError _memlocatorsetstore_print(TMValueStore s,FILE *f);

static TMError _memlocatorsetstore_scan_open(TMValueStore self, int opcode, const void *arg, TMValueStoreScan *sp);
static TMError _memlocatorsetstore_scan_close(TMValueStore s, TMValueStoreScan scan);
static TMError _memlocatorsetstore_scan_prepare_next(TMValueStore s, TMValueStoreScan scan);

struct bucket {
	char *locator;
	TMVid set_vid;
};


extern struct TMValueType TextSet;

struct TMValueStoreType default_mem_locatorset_store_type = {
	&TextSet,
		/* TMValueStore API */
	_memlocatorsetstore_new,
	_memlocatorsetstore_delete,
	_memlocatorsetstore_open,
	_memlocatorsetstore_close,

	_memlocatorsetstore_lookup_vid,
	_memlocatorsetstore_delete_value,
	_memlocatorsetstore_get_value,
	_memlocatorsetstore_print,

	_memlocatorsetstore_scan_open,
	_memlocatorsetstore_scan_close,
	_memlocatorsetstore_scan_prepare_next,


};


#define INIT_SIZE 100
struct default_mem_locatorset_store {
	TMValueStoreType type;
	struct data *data;
	TMValueType vtype;

	TMTable loctab;

	int count;
	int size;
	char ***sets;

	/*
	TMTable vid_by_locator_table;
	*/
	
};
typedef struct default_mem_locatorset_store *T;
/*
static int _locs_find_value(T self,const void* v,TMVid *pvid);
*/

static TMError _find_set_vid(T self,TMList list,TMVid *pvid);

TMValueStore _memlocatorsetstore_new(void *data,TMProperty prop, void*param)
{
	T self;
	int i;
	TMTRACE(TM_STORAGE_TRACE,"enter [prop=%s,vtype=%s]\n" _
			prop->name _ prop->value_type->name);
	TM_NEW(self);
	self->type =  &default_mem_locatorset_store_type;
	self->data = (struct data *)data;
	self->vtype = prop->value_type;

	self->loctab = tm_table_new(100,tm_strcmp_v,tm_strhash_v);


	self->count = 0;
	self->sets = (char***)TM_ALLOC(INIT_SIZE * sizeof(char*) );
	self->size = INIT_SIZE;
	for(i=0;i<INIT_SIZE;i++)
		self->sets[i] = NULL;
	TMTRACE(TM_STORAGE_TRACE,"exit\n");

	return (TMValueStore)self;
}

TMError _memlocatorsetstore_open(TMValueStore s)
{
	T self;
	self = (T)s;
	assert(self);
	return TM_OK;
}
TMError _memlocatorsetstore_close(TMValueStore s)
{
	T self;
	self = (T)s;
	assert(self);
	return TM_OK;
}
void _memlocatorsetstore_delete(TMValueStore s)
{
	T self;
	int i;
	TMList k,keys = NULL;
	self = (T)s;
	assert(self);

	keys = tm_table_keys(self->loctab, &keys);
	for(k=keys ;k;k=k->next)
	{
		struct bucket *bp;
		char *p = (char*)k->content;
		bp = (struct bucket*)tm_table_get(self->loctab,p);

		tm_table_remove(self->loctab,p);
		TM_FREE(bp->locator);
		TM_FREE(bp);
		k->content = NULL;
	}

	tm_table_delete( &(self->loctab) );
	tm_list_delete(&keys);

		
	for(i=1;i<=self->count;i++)
	{
		if(self->sets[i])
		{
			TM_FREE(self->sets[i]);
		}
	}
	TM_FREE(self->sets);
	TM_FREE(self);
}



TMError _memlocatorsetstore_lookup_vid(TMValueStore s,const void *v,int flag,TMVid *pvid)
{
	T self;
	TMList locset,lp;
	char **set;
	int i,n;
	TMVid vid;


	assert(s);
	self = (T)s;
	assert(v);
	assert(pvid);
	*pvid = 0;
	TMTRACE(TM_STORAGE_TRACE,"enter\n");

	if(flag == TM_VALUE_LOOKUP)
	{
		return ( _find_set_vid(self,(TMList)v,pvid) );	
	}


	assert(flag == TM_VALUE_CREATE); /* for the use with SAM::SubjectIndicators, this should hold true (we get only called in branch with prop equality code!! (no lookup by set-equality, only be non-disjointness.) (we never get called in the no-prop-equality-branch of _pstore_vput() */

	/* FIXME: a lookup_create branch is now not 'allowed' */
	/* this is now only debug code */
#ifndef NDEBUG
	_find_set_vid(self,(TMList)v,&vid);	
	assert(vid == 0);
#endif

	locset = (TMList)v;
	n = tm_list_size(locset);
	set = (char**)TM_ALLOC( (n + 1 ) * sizeof(char*) );
	set[0] = (char*)n;
	TMTRACE(TM_STORAGE_TRACE,"length of set is %d\n" _ n);

	self->count++;
	if(self->count >= self->size)
	{
		int i;
		self->size *= 2;
		TM_RESIZE(self->sets,(self->size * sizeof(char*)) );

		for(i=self->count;i<self->size;i++)
			self->sets[i] = NULL;

		TMTRACE(TM_STORAGE_TRACE,"text store: resize to %d\n" _ self->size);
	}
	vid = self->count;


	TMTRACE(TM_STORAGE_TRACE,"new vid is %d\n" _ vid);
	for(lp=locset,i=1;lp;lp=lp->next,i++)
	{
		struct bucket *bp;


		TMTRACE(TM_STORAGE_TRACE,"handling set item '%s'\n" _ (char*)lp->content );

		bp = (struct bucket*)tm_table_get( self->loctab , (char*)lp->content );
		if(bp == NULL)
		{
			TMTRACE(TM_STORAGE_TRACE,"%s not found in loctab\n" _ (char*)lp->content  );
			TM_NEW(bp);
			bp->locator = tm_strdup( (char*)lp->content);
			bp->set_vid = vid;
			tm_table_put( self->loctab , bp->locator , bp);
		} else {
			TMTRACE(TM_STORAGE_TRACE,"%s found in loctab\n" _ bp->locator );
			TMTRACE(TM_STORAGE_TRACE,"set_vid=%d vid=%d\n" _ bp->set_vid _ vid);
			assert(0);
			assert( bp->set_vid == vid);
		}
		set[i] =bp->locator ;
		TMTRACE(TM_STORAGE_TRACE,"storing locator: %s (i=%d)\n" _
				bp->locator _ i);
	}


	self->sets[vid] = set;
	/*
	TMTRACE(TM_STORAGE_TRACE,"text store: added %d -> %s\n" _
		self->count _ self->texts[self->count]);
	*/

	*pvid = vid;
	return TM_OK;
}
TMError _memlocatorsetstore_get_value(TMValueStore s,TMVid vid, void** vp)
{
	T self;
	TMList locset = NULL;
	int i,n;
	char **set;
	assert(s);
	self = (T)s;


	assert(vid > 0);
	assert(vid <= self->count);
	assert(self->sets[vid]);	/* caller mustr make sure no deleted values are getted */
	set = self->sets[vid];
	n = (int)set[0];
	for(i=1;i<=n;i++)
	{
		char *p = set[i];
		locset = tm_list_push(locset,tm_strdup(p));
	}

	*vp = locset;

	return TM_OK;
}
TMError _memlocatorsetstore_delete_value(TMValueStore s,TMVid vid)
{
	T self;
	int i,len;
	char **set;
	assert(s);
	self = (T)s;
	TMTRACE(TM_STORAGE_TRACE,"enter (deleting value with vid %d)\n" _ vid);
	assert(vid > 0);
	assert(vid <= self->count);
	assert(self->sets[vid]);	/* caller mustr make sure to-be-deleted values exist */
	set = self->sets[vid];
	len = (int)set[0];
	TMTRACE(TM_STORAGE_TRACE,"len is %d\n" _ len);
	for(i=1;i<=len;i++)
	{
		struct bucket *bp;
		char *loc = set[i];
		bp = (struct bucket*)tm_table_get( self->loctab , loc );
		assert(bp);
		tm_table_remove( self->loctab , loc );
		TMTRACE(TM_STORAGE_TRACE,"removed loctab entry for %s \n" _ loc);
		TMTRACE(TM_STORAGE_TRACE,"set_vid=%d vid=%d\n" _ bp->set_vid _ vid);
		assert(bp->set_vid == vid);
		assert(bp->locator == loc);
		TM_FREE(bp->locator);
		TM_FREE(bp);
	}

	TM_FREE(self->sets[vid]);
	self->sets[vid] = NULL;
	return TM_OK;
}


TMError _memlocatorsetstore_print(TMValueStore s,FILE *f)
{
        T self;
        int i,n;
        assert(s);
        self = (T)s;
 
        fprintf(f,"TextSet Store\n\n");
        for(i = 1; i<= self->count; i++)
        {
		int j;
		char **set;
		if(!self->sets[i])
		{
                	fprintf(f,"%6d -> [deleted]\n", i );
			continue;
		}
		set = self->sets[i];
		n = (int)set[0];
                fprintf(f,"%6d -> [", i );
		for(j=1;j<=n;j++)
		{

                	fprintf(f,"\"%s\",", set[j]);
		}
                fprintf(f,"]\n");

        }
	return TM_OK;
}

struct _FIXME {
	char *query;
	TMTopicSet set;
	int pos;
	int size;
};

void _FIXME(const void *key, void **value, void *cl)
{
	struct _FIXME *p;
	struct bucket **bpp;
	struct bucket *bp;
	p = (struct _FIXME*)cl;
	TMTRACE(TM_STORAGE_TRACE,"enter\n");
	bpp = (struct bucket**)value;
	bp = (struct bucket*)*bpp;
	TMTRACE(TM_STORAGE_TRACE,"key=%s bp->locator=%s, set_vid=%d\n" 
			_ (char*)key _ bp->locator _ bp->set_vid);
	assert(strcmp(key,bp->locator) == 0);
	assert(key == bp->locator);

	if( tm_strstr((char*)key,p->query,TM_CASE_INSENSITIVE) != NULL)
	{
		TMTRACE(TM_STORAGE_TRACE,"%s contains %s, storing set vid %d\n" 
			_ (char*)key _ p->query _ bp->set_vid);
		tm_topicset_add(p->set,bp->set_vid);	
	}	
	TMTRACE(TM_STORAGE_TRACE,"exit\n");
}

TMError _memlocatorsetstore_scan_open(TMValueStore s, int opcode, const void *arg, TMValueStoreScan *sp)
{
	T self;
	TMValueStoreScan scan;
	TMList list;

	assert(s);
	self = (T)s;


	TMTRACE(TM_STORAGE_TRACE,"enter\n");
	TM_NEW(scan);
	scan->op = opcode;
	scan->arg = arg;
	scan->next_vid = 0; /* preset so that nothing to be done after loop terminates */

	switch(scan->op) {
	case TM_OP_TextSet_NOT_DISJOINT:
		TMTRACE(TM_STORAGE_TRACE,"op=NOT_DISJOINT\n");
		for(list=(TMList)arg;list;list=list->next)
		{
			struct bucket *bp;
			char *locator = (char*)list->content;
			TMTRACE(TM_STORAGE_TRACE,"checking for locator %s \n" _ locator );
			bp = (struct bucket*)tm_table_get( self->loctab , locator );
			if(bp)
			{
				TMTRACE(TM_STORAGE_TRACE,"locator %s is in store with locatorset vid=%d\n" _ bp->locator _ bp->set_vid);
				scan->next_vid = bp->set_vid;
				break;
			}
		}
		scan->data = list;  /* rest of list (including current item!) */
		break;
	case TM_OP_TextSet_INCLUDES: {
		struct bucket *bp;
		TMTRACE(TM_STORAGE_TRACE,"op=INCLUDES\n");
		bp = (struct bucket*)tm_table_get( self->loctab , (char*)scan->arg );
		if(bp)
		{
				scan->next_vid = bp->set_vid;
		}
		break; }
	case TM_OP_TextSet_LIKE: {
		struct _FIXME *p;
		TMTRACE(TM_STORAGE_TRACE,"op=LIKE\n");
		TM_NEW(p);
		p->query = (char*)scan->arg;
		p->set = tm_topicset_new(0);
		tm_table_apply(self->loctab, _FIXME, p);
		p->pos = 0;
		p->size = tm_topicset_size(p->set);
		if(p->size > 0)
		{
				scan->next_vid = tm_topicset_get(p->set,p->pos);
		}
		scan->data = p;

		break; }

	}
	*sp = scan;

	TMTRACE(TM_STORAGE_TRACE,"exit\n");
	return TM_OK;
}
TMError _memlocatorsetstore_scan_close(TMValueStore s, TMValueStoreScan scan)
{
	switch(scan->op) {
	case TM_OP_TextSet_LIKE: {
		struct _FIXME *p;
		p = (struct _FIXME *)scan->data;
		tm_topicset_delete(&(p->set));
		TM_FREE(p);
		break; }
	}
	TMTRACE(TM_STORAGE_TRACE,"enter\n");
	TM_FREE(scan);
	TMTRACE(TM_STORAGE_TRACE,"exit\n");
	return TM_OK;
}
TMError _memlocatorsetstore_scan_prepare_next(TMValueStore s, TMValueStoreScan scan)
{

	T self;
	assert(s);
	self = (T)s;

	TMTRACE(TM_STORAGE_TRACE,"enter\n");
	switch(scan->op) {
	case TM_OP_TextSet_NOT_DISJOINT: {
		TMList list;
		TMTRACE(TM_STORAGE_TRACE,"op=NOT_DISJOINT\n");
		scan->next_vid = 0;
		list = (TMList)scan->data;
		if(list)
			list = list->next; /* list was current, so move to next item */
		if( ! list )
		{
			scan->next_vid = 0;
			break;
		}
		for( ;list;list=list->next)
		{
			struct bucket *bp;
			char *locator = (char*)list->content;
			TMTRACE(TM_STORAGE_TRACE,"checking for locator %s \n" _ locator );
			bp = (struct bucket*)tm_table_get( self->loctab , locator );
			if(bp)
			{
				TMTRACE(TM_STORAGE_TRACE,"locator %s is in store with locatorset vid=%d\n" _ bp->locator _ bp->set_vid);
				scan->next_vid = bp->set_vid;
				break;
			}
		}
		scan->data = list;  /* rest of list (including current item!) */
		break; }
	case TM_OP_TextSet_INCLUDES: 
		TMTRACE(TM_STORAGE_TRACE,"op=INCLUDES\n");
		if(scan->next_vid)
			scan->next_vid = 0;
		break;
	case TM_OP_TextSet_LIKE: {
		struct _FIXME *p;
		TMTRACE(TM_STORAGE_TRACE,"op=LIKE\n");
		scan->next_vid = 0;
		p = (struct _FIXME *)scan->data;
		p->pos++;
		if(p->pos < p->size)
		{
				scan->next_vid = tm_topicset_get(p->set,p->pos);
		}

		break; }
	}

	TMTRACE(TM_STORAGE_TRACE,"exit\n");
	return TM_OK;
}

TMError _find_set_vid(T self,TMList list,TMVid *pvid)
{
	TMList lp;
	int i,len;
	TMVid vid;
	char **array;
	struct bucket *bp;

	*pvid = 0;

	/* lookup set by ANY of list (beware that this can only be done because the
	 * whole store relies on set-disjointness!!
	 * (FIXME!!)*/
	bp = (struct bucket*)tm_table_get( self->loctab , (char*)list->content );
	if(bp == NULL) /* at least one not found -> set not in store */
		return TM_OK;
	vid = bp->set_vid;
	array = self->sets[vid];
	len = (int)array[0];
	if(tm_list_size(list) != len)
		return TM_OK;
	for(i=1;i<=len;i++)
	{
		int found = 0;
		char *loc = array[i];
		for(lp=list;lp;lp=lp->next)
		{
			if( strcmp((char*)lp->content,loc) == 0)
			{
				found=1;
				break;
			}
		}
		if(!found)
			return TM_OK;
	}

	*pvid = vid;
	return TM_OK;

}
