#include <tm.h>
#include <tmassert.h>
#include <tmmem.h>
#include <tmtrace.h>
#include <tmutil.h> 
#include <tmtable.h> 
#include <tmstorage.h> 
#include <tmbasictypes.h> 

#include "descriptor.h"                                                         

/*
TM_OP_ASIDP_HAS_TYPE
*/

/* TMStore API: */
static TMValueStore _memasidpstore_new(void *data,TMProperty prop, void* param);
static TMError _memasidpstore_open(TMValueStore self);
static void _memasidpstore_delete(TMValueStore self);
static TMError _memasidpstore_close(TMValueStore self);


/* TMValueStore API: */
static TMError _memasidpstore_lookup_vid(TMValueStore s,const void *v,int flag,TMVid *pvid);
static TMError _memasidpstore_get_value(TMValueStore s,TMVid vid, void** vp);
static TMError _memasidpstore_delete_value(TMValueStore s,TMVid vid);
static TMError _memasidpstore_print(TMValueStore s,FILE *f);

static TMError _memasidpstore_scan_open(TMValueStore self, int opcode, const void *arg, TMValueStoreScan *sp);
static TMError _memasidpstore_scan_close(TMValueStore s, TMValueStoreScan scan);
static TMError _memasidpstore_scan_prepare_next(TMValueStore s, TMValueStoreScan scan);


extern struct TMValueType ASIDP;

struct TMValueStoreType default_mem_asidp_store_type = {
	&ASIDP,
		/* TMValueStore API */
	_memasidpstore_new,
	_memasidpstore_delete,
	_memasidpstore_open,
	_memasidpstore_close,

	_memasidpstore_lookup_vid,
	_memasidpstore_delete_value,
	_memasidpstore_get_value,
	_memasidpstore_print,
	_memasidpstore_scan_open,
	_memasidpstore_scan_close,
	_memasidpstore_scan_prepare_next
};
#define INIT_SIZE 100
struct default_mem_asidp_store {
	TMValueStoreType type;
	struct data *data;
	TMValueType vtype;
	int count;
	int size;
	struct asidp_value **asidps;
	TMTable type_table;
	
};

typedef struct default_mem_asidp_store *T;

static int _asidps_find_value(T self,const void* v,TMVid *pvid);
static struct asidp_value *_asidp_value_copy(struct asidp_value *v);

TMValueStore _memasidpstore_new(void *data,TMProperty prop, void*param)
{
	T self;
	int i;
	TMTRACE(TM_STORAGE_TRACE,"enter [prop=%s,vtype=%s]\n" _
			prop->name _ prop->value_type->name);
	TM_NEW(self);
	self->type =  &default_mem_asidp_store_type;
	self->data = (struct data *)data;
	self->vtype = prop->value_type;
	self->count = 0;
	self->asidps = (struct asidp_value**)TM_ALLOC(INIT_SIZE * sizeof(struct asidp_value*) );
	self->size = INIT_SIZE;
	for(i=0;i<INIT_SIZE;i++)
		self->asidps[i] = NULL;
	self->type_table = tm_table_new(100,tm_strcmp_v,tm_strhash_v);
	TMTRACE(TM_STORAGE_TRACE,"exit\n");

	return (TMValueStore)self;
}

TMError _memasidpstore_open(TMValueStore s)
{
	T self;
	self = (T)s;
	assert(self);
	tm_table_delete(&(self->type_table));
	return TM_OK;
}
TMError _memasidpstore_close(TMValueStore s)
{
	T self;
	self = (T)s;
	assert(self);
	return TM_OK;
}
void _memasidpstore_delete(TMValueStore s)
{
	T self;
	int i;
	self = (T)s;
	assert(self);
	for(i=1;i<=self->count;i++)
	{
		if(self->asidps[i])
		{
			TM_FREE(self->asidps[i]);
		}
	}
	TM_FREE(self->asidps);
	TM_FREE(self);
}



TMError _memasidpstore_lookup_vid(TMValueStore s,const void *v,int flag,TMVid *pvid)
{
	T self;
	assert(s);
	self = (T)s;
	assert(v);
	assert(pvid);
	*pvid = 0;

	if(flag != TM_VALUE_CREATE)
	{
		if(_asidps_find_value(self,v,pvid))
			return TM_OK;
	}
	if(flag == TM_VALUE_LOOKUP)
		return TM_OK;



	self->count++;
	if(self->count >= self->size)
	{
		int i;
		self->size *= 2;
		TM_RESIZE(self->asidps,(self->size * sizeof(struct asidp_value*)) );

		for(i=self->count;i<self->size;i++)
			self->asidps[i] = NULL;

		TMTRACE(TM_STORAGE_TRACE,"asidp store: resize to %d\n" _ self->size);
	}

	self->asidps[self->count] = _asidp_value_copy((struct asidp_value *)v); 
	/*FIXME
	TMTRACE(TM_STORAGE_TRACE,"asidp store: added %d -> %s\n" _
		self->count _ self->asidps[self->count]);
	*/

	*pvid = self->count;

	return TM_OK;
}
TMError _memasidpstore_get_value(TMValueStore s,TMVid vid, void** vp)
{
	T self;
	assert(s);
	self = (T)s;

	assert(vid > 0);
	assert(vid <= self->count);
	assert(self->asidps[vid]);	/* caller mustr make sure no deleted values are getted */
	*vp = (void*) _asidp_value_copy(self->asidps[vid]);

	return TM_OK;
}
TMError _memasidpstore_delete_value(TMValueStore s,TMVid vid)
{
	T self;
	assert(s);
	self = (T)s;

	assert(vid > 0);
	assert(vid <= self->count);
	assert(self->asidps[vid]);	/* caller mustr make sure no to-be-deleted values exist */
	/* FIXME: free is not fully correct, use vt function !!! */
	TM_FREE(self->asidps[vid]);
	self->asidps[vid] = NULL;

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
TMError _memasidpstore_get_vids_like(TMValueStore s,const void* v,TMTopicSet set)
{
	/* FIXME */
	T self;
	/*
	int i;
	*/
	assert(s);
	self = (T)s;
	assert(0);
	/*	
	for(i=1;i<=self->count; i++)
	{
		assert(self->asidps[i]);
		if( strstr(self->asidps[i],(char*)v) != NULL )
		{
			tm_topicset_add(set,(TMTopic)i);	
		}
	}
	*/

	return TM_OK;
}
#endif


TMError _memasidpstore_print(TMValueStore s,FILE *f)
{
        T self;
        int i;
        assert(s);
        self = (T)s;
 
        fprintf(f,"ASIDP Store\n\n");
 
        for(i = 1; i<= self->count; i++)
        {
		char buf[1024];
		if(!self->asidps[i])
		{
			fprintf(f,"%6d -> [deleted]\n" , i);
		}
		else
		{
			tm_value_to_string(self->vtype,self->asidps[i],buf,sizeof(buf));
               		fprintf(f,"%6d -> %s\n", i, self->asidps[i] ? buf : "[deleted]" );
		}
        }
	return TM_OK;
}


/* -----------------------------------------------------------------------
 *
 *                             STATIC FUNCTIONS
 *
 * ----------------------------------------------------------------------- */

int _asidps_find_value(T self,const void* v,TMVid *pvid)
{
	int i;
	TMList mthis,mthat;
	struct asidp_value *av = (struct asidp_value*)v;
	*pvid = 0;
	for(i=1;i<=self->count; i++)
	{
		int equal = 1; /* assume */
		
		if(! self->asidps[i])
			continue;

		if(self->asidps[i]->type != av->type)
			continue;
		for(mthis=self->asidps[i]->memberships,mthat=av->memberships; mthis && mthat; mthis=mthis->next,mthat=mthat->next)
		{
			struct a_membership *this,*that;
			this = (struct a_membership*)mthis->content;
			that = (struct a_membership*)mthat->content;
			if(this->role != that->role)
			{
				equal = 0;
				break;
			}
			if(this->player != that->player)
			{
				equal = 0;
				break;
			}
		}

		if( ! equal)
			continue;
		if(mthis == NULL && mthat == NULL)
		{
			*pvid = i;
			return (1);
		}
	}
	/* not found */
	return (0);
}
struct asidp_value *_asidp_value_copy(struct asidp_value *v) 
{
	struct asidp_value *n;
	TMList lp;
	TM_NEW(n);
	n->type = v->type;
	n->memberships = NULL;
	for(lp=v->memberships;lp;lp=lp->next)
	{
		struct a_membership *m;
		TM_NEW(m);
		m->role = ((struct a_membership *)lp->content)->role;
		m->player = ((struct a_membership *)lp->content)->player;
		n->memberships = tm_list_append(n->memberships,m);

	}

	return n;
}
#if 0
	for(i=1;i<=self->count; i++)
	{
		int equal = 1; /* assume */
		assert(self->asidps[i]);
		/* FIXME: we'll have deletes later on, so this assertion will fail then! */
		if(self->asidps[i]->type != av->type)
			continue;
		for(mthis=self->asidps[i]->memberships,mthat=av->memberships; mthis && mthat; mthis=mthis->next,mthat=mthat->next)
		{
			struct a_membership *this,*that;
			this = (struct a_membership*)mthis->content;
			that = (struct a_membership*)mthat->content;
			if(this->role != that->role)
#endif

TMError _memasidpstore_scan_open(TMValueStore s, int opcode, const void *arg, TMValueStoreScan *sp)
{
	T self;
	int i;
	TMValueStoreScan scan;

	assert(s);
	self = (T)s;


	TMTRACE(TM_STORAGE_TRACE,"enter\n");
	TM_NEW(scan);
	scan->op = opcode;
	scan->arg = arg;
	scan->next_vid = 0; /* preset so that nothing to be done after loop terminates */

	switch(scan->op) {
	case TM_OP_ASIDP_HAS_TYPE:
		TMTRACE(TM_STORAGE_TRACE,"op=HAS_TYPE\n");
		for(i=1; i<=self->count; i++)
		{
			if( ! self->asidps[i] )
				continue;
                	if( self->asidps[i]->type == (TMTopic)scan->arg)
			{
				scan->next_vid = i;
				break;
			}
		}
 
        	scan->data = (void*)i;  /* index */
		break;
	case TM_OP_ASIDP_HAS_PLAYER_PAIR: {
		struct a_membership *argpair = (struct a_membership *)scan->arg;
		TMTRACE(TM_STORAGE_TRACE,"op=HAS_PLAYER_PAIR\n");
		for(i=1; i<=self->count; i++)
		{
			int found = 0;
			TMList lp;
			if( ! self->asidps[i] )
				continue;

			for(lp=self->asidps[i]->memberships;lp;lp=lp->next)
			{
				struct a_membership *mp =
					(struct a_membership *)lp->content;
				if(mp->player == argpair->player && mp->role == argpair->role)
				{
					found = 1;
					break;
				}
			}
	
                	if( found )
			{
				scan->next_vid = i;
				break;
			}
		}
 
        	scan->data = (void*)i;  /* index */
		break; }
	case TM_OP_ASIDP_VALUE_INCLUDES_TOPICS:
		TMTRACE(TM_STORAGE_TRACE,"op=VALUE_INCLUDES_TOPICS\n");
		for(i=1; i<=self->count; i++)
		{
			int found = 0;
			TMList lp;
			if( ! self->asidps[i] )
				continue;
                	if( self->asidps[i]->type == (TMTopic)scan->arg)
			{
				scan->next_vid = i;
				break;
			}
			for(lp=self->asidps[i]->memberships; lp; lp=lp->next)
			{

				struct a_membership *mb;
				mb = (struct a_membership*)lp->content;
                		if(mb->role == (TMTopic)scan->arg
				|| mb->player == (TMTopic)scan->arg)
				{
					scan->next_vid = i;
					found = 1;
					break;
				}
			}	
			if(found)
				break;
		}
 
        	scan->data = (void*)i;  /* index */
		break;
        }
	*sp = scan;

	TMTRACE(TM_STORAGE_TRACE,"exit\n");
	return TM_OK;
}
TMError _memasidpstore_scan_close(TMValueStore s, TMValueStoreScan scan)
{
	TMTRACE(TM_STORAGE_TRACE,"enter\n");
	TM_FREE(scan);
	TMTRACE(TM_STORAGE_TRACE,"exit\n");
	return TM_OK;
}
TMError _memasidpstore_scan_prepare_next(TMValueStore s, TMValueStoreScan scan)
{

	T self;
	int i;
	assert(s);
	self = (T)s;

	TMTRACE(TM_STORAGE_TRACE,"enter\n");
	switch(scan->op) {
	case TM_OP_ASIDP_HAS_TYPE:
		if( (int)(scan->data) > self->count)
		{
			scan->next_vid = 0;
			break;
		}
		scan->next_vid = 0; /* presetting (see above) */
		for(i=((int)scan->data)+1 ; i<=self->count; i++)
		{
			if( ! self->asidps[i] )
				continue;
			if( self->asidps[i]->type == (TMTopic)scan->arg)
			{
				scan->next_vid = i;
				break;
			}
 
		}
		scan->data = (void*)i;  /* index */
		break;
	case TM_OP_ASIDP_HAS_PLAYER_PAIR: {
		struct a_membership *argpair = (struct a_membership *)scan->arg;
		if( (int)(scan->data) > self->count)
		{
			scan->next_vid = 0;
			break;
		}
		scan->next_vid = 0; /* presetting (see above) */
		for(i=((int)scan->data)+1 ; i<=self->count; i++)
		{
			int found = 0;
			TMList lp;
			if( ! self->asidps[i] )
				continue;
			for(lp=self->asidps[i]->memberships;lp;lp=lp->next)
			{
				struct a_membership *mp =
					(struct a_membership *)lp->content;
				if(mp->player == argpair->player && mp->role == argpair->role)
				{
					found = 1;
					break;
				}
			}
			if( found )
			{
				scan->next_vid = i;
				break;
			}
 
		}
		scan->data = (void*)i;  /* index */
		break; }
	case TM_OP_ASIDP_VALUE_INCLUDES_TOPICS:
		TMTRACE(TM_STORAGE_TRACE,"op=VALUE_INCLUDES_TOPICS\n");
		if( (int)(scan->data) > self->count)
		{
			scan->next_vid = 0;
			break;
		}
		scan->next_vid = 0; /* presetting (see above) */
		for(i=((int)scan->data)+1; i<=self->count; i++)
		{
			int found = 0;
			TMList lp;
			if( ! self->asidps[i] )
				continue;
                	if( self->asidps[i]->type == (TMTopic)scan->arg)
			{
				scan->next_vid = i;
				break;
			}
			for(lp=self->asidps[i]->memberships; lp; lp=lp->next)
			{

				struct a_membership *mb;
				mb = (struct a_membership*)lp->content;
                		if(mb->role == (TMTopic)scan->arg
				|| mb->player == (TMTopic)scan->arg)
				{
					scan->next_vid = i;
					found = 1;
					break;
				}
			}	
			if(found)
				break;
		}
 
        	scan->data = (void*)i;  /* index */
		break;
	}

	TMTRACE(TM_STORAGE_TRACE,"exit\n");
	return TM_OK;
}
