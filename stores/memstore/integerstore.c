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
static TMValueStore _memintegerstore_new(void *data,TMProperty prop, void* param);
static TMError _memintegerstore_open(TMValueStore self);
static void _memintegerstore_delete(TMValueStore self);
static TMError _memintegerstore_close(TMValueStore self);


/* TMValueStore API: */
static TMError _memintegerstore_lookup_vid(TMValueStore s,const void *v,int flag,TMVid *pvid);
static TMError _memintegerstore_get_value(TMValueStore s,TMVid vid, void** vp);
static TMError _memintegerstore_delete_value(TMValueStore s,TMVid vid);
static TMError _memintegerstore_print(TMValueStore s,FILE *f);
static TMError _memintegerstore_scan_open(TMValueStore self, int opcode, const void *arg, TMValueStoreScan *sp);
static TMError _memintegerstore_scan_close(TMValueStore s, TMValueStoreScan scan);
static TMError _memintegerstore_scan_prepare_next(TMValueStore s, TMValueStoreScan scan);


extern struct TMValueType Integer;

struct TMValueStoreType default_mem_integer_store_type = {
	&Integer,
		/* TMValueStore API */
	_memintegerstore_new,
	_memintegerstore_delete,
	_memintegerstore_open,
	_memintegerstore_close,

	_memintegerstore_lookup_vid,
	_memintegerstore_delete_value,
	_memintegerstore_get_value,

	_memintegerstore_print,
	_memintegerstore_scan_open,
	_memintegerstore_scan_close,
	_memintegerstore_scan_prepare_next,

};
typedef int MYINT;
#define INIT_SIZE 100
struct default_mem_integer_store {
	TMValueStoreType type;
	struct data *data;
	TMValueType vtype;
	int count;
	int size;
	MYINT **integers;
	TMTable tab;
	
};

typedef struct default_mem_integer_store *T;

static int _integers_find_value(T self,const void* v,TMVid *pvid);

TMValueStore _memintegerstore_new(void *data,TMProperty prop, void*param)
{
	T self;
	int i;
	TMTRACE(TM_STORAGE_TRACE,"enter [prop=%s,vtype=%s]\n" _
			prop->name _ prop->value_type->name);
	assert(sizeof(MYINT) <= sizeof(char*));  /* we return ints through void* !! */
	TM_NEW(self);
	self->type =  &default_mem_integer_store_type;
	self->data = (struct data *)data;
	self->vtype = prop->value_type;
	self->count = 0;
	self->integers = (MYINT**)TM_ALLOC(INIT_SIZE * sizeof(MYINT*) );
	self->size = INIT_SIZE;
	for(i=0;i<INIT_SIZE;i++)
		self->integers[i] = NULL;
	self->tab = tm_table_new(10000,NULL,NULL);


	TMTRACE(TM_STORAGE_TRACE,"exit\n");

	return (TMValueStore)self;
}

TMError _memintegerstore_open(TMValueStore s)
{
	T self;
	self = (T)s;
	assert(self);
	return TM_OK;
}
TMError _memintegerstore_close(TMValueStore s)
{
	T self;
	self = (T)s;
	assert(self);
	return TM_OK;
}
void _memintegerstore_delete(TMValueStore s)
{
	T self;
	int i;
	self = (T)s;
	assert(self);
	for(i=1;i<=self->count;i++)
	{
		if(self->integers[i])
		{
			TM_FREE(self->integers[i]);
		}
	}
	tm_table_delete(&(self->tab));
	TM_FREE(self->integers);
	TM_FREE(self);
}



TMError _memintegerstore_lookup_vid(TMValueStore s,const void *v,int flag,TMVid *pvid)
{
	T self;
	assert(s);
	self = (T)s;
	assert(v);
	assert(pvid);
	*pvid = 0;

	if(flag != TM_VALUE_CREATE)
	{
		if(_integers_find_value(self,v,pvid))
			return TM_OK;
	}
	if(flag == TM_VALUE_LOOKUP)
		return TM_OK;

	self->count++;
	if(self->count >= self->size)
	{
		int i;
		self->size *= 2;
		TM_RESIZE(self->integers,(self->size * sizeof(MYINT*)) );

		for(i=self->count;i<self->size;i++)
			self->integers[i] = NULL;

		TMTRACE(TM_STORAGE_TRACE,"integer store: resize to %d\n" _ self->size);
	}

	self->integers[self->count] = (MYINT*)TM_ALLOC(sizeof(MYINT));
	*(self->integers[self->count]) = (MYINT)v;
	tm_table_put(self->tab,(void*)*(self->integers[self->count]),(void*)self->count);
	TMTRACE(TM_STORAGE_TRACE,"integer store: added %d -> %d\n" _
		self->count _ *(self->integers[self->count]) );

	*pvid = self->count;

	return TM_OK;
}
TMError _memintegerstore_get_value(TMValueStore s,TMVid vid, void** vp)
{
	T self;
	assert(s);
	self = (T)s;

	assert(vid > 0);
	assert(vid <= self->count);
	assert(self->integers[vid]);	/* caller mustr make sure no deleted values are getted */
	*vp = (void*)*(self->integers[vid]);

	return TM_OK;
}
TMError _memintegerstore_delete_value(TMValueStore s,TMVid vid)
{
	T self;
	assert(s);
	self = (T)s;

	assert(vid > 0);
	assert(vid <= self->count);
	assert(self->integers[vid]);	/* caller mustr make sure no to-be-deleted values exist */
	tm_table_remove(self->tab,(void*)*(self->integers[vid]));
	TM_FREE(self->integers[vid]);
	self->integers[vid] = NULL;

	return TM_OK;
}

TMError _memintegerstore_print(TMValueStore s,FILE *f)
{
        T self;
        int i;
        assert(s);
        self = (T)s;
 
        fprintf(f,"Integer Store\n\n");
 
        for(i = 1; i<= self->count; i++)
        {
		if(self->integers[i])
                	fprintf(f,"%6d -> %d\n", i, *(self->integers[i])  );
		else
                	fprintf(f,"%6d -> [deleted]\n", i );
        }
	return TM_OK;
}


/* -----------------------------------------------------------------------
 *
 *                             STATIC FUNCTIONS
 *
 * ----------------------------------------------------------------------- */

int _integers_find_value(T self,const void* v,TMVid *pvid)
{
	assert(v);
	*pvid = (TMVid)tm_table_get(self->tab,(void*)((MYINT)v));
	return (*pvid != 0);
}

TMError _memintegerstore_scan_open(TMValueStore s, int opcode, const void *arg, TMValueStoreScan *sp)
{
	T self;
	/*
	TMValueStoreScan scan;
	int i;
	*/
	assert(s);
	self = (T)s;

	assert(0);
#if 0


	TMTRACE(TM_STORAGE_TRACE,"enter\n");
	TM_NEW(scan);
	scan->op = opcode;
	scan->arg = arg;
	scan->next_vid = 0; /* preset so that nothing to be done after loop terminates */

	switch(opcode) {
	case TM_OP_Integer_LIKE:
	for(i=1; i<=self->count; i++)
	{
		if( ! self->integers[i] )
			continue;
		if( strstr(self->integers[i], (char*)scan->arg) != NULL )
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
#endif

	TMTRACE(TM_STORAGE_TRACE,"exit\n");
	return TM_OK;
}
TMError _memintegerstore_scan_close(TMValueStore s, TMValueStoreScan scan)
{
	TMTRACE(TM_STORAGE_TRACE,"enter\n");
	TM_FREE(scan);
	TMTRACE(TM_STORAGE_TRACE,"exit\n");
	return TM_OK;
}
TMError _memintegerstore_scan_prepare_next(TMValueStore s, TMValueStoreScan scan)
{
	/*
	int i;
	T self;
	*/
	assert(s);

	assert(0);
#if 0
	self = (T)s;
	if( (int)(scan->data) > self->count)
	{
		scan->next_vid = 0;
	}
	scan->next_vid = 0; /* presetting (see above) */
	for(i=((int)scan->data)+1 ; i<=self->count; i++)
	{
		if( ! self->integers[i] )
			continue;
		if( strstr(self->integers[i], (char*)scan->arg) != NULL )
		{
			scan->next_vid = i;
			break;
		}
			

	}
	scan->data = (void*)i;  /* index */
#endif
	return TM_OK;
}

