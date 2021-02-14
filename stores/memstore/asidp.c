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
 * The ASIDP store is implemented as an array of values, the array index being the VID.
 * Values are an array of TMTopics in the form:
 *
 *     +------+------+------+------+-...-+------+------+
 *     |      |Number|      |      |     |      |      |
 *     | Type |  of  |  R1  |  x1  |     |  Rn  |  xn  |
 *     |      |Membrs|      |      |     |      |      |
 *     +------+------+------+------+-...-+------+------+
 *
 * The memberships are ordered by role, which adds some useful properties
 * to the values:
 *
 * - equal values are byte-identical
 * - values can be hashed
 *
 */

#define NBYTES(array) ( ((((array)[1])*2)+2) * sizeof(TMTopic) )
#define NELEMS(array) ( ((((array)[1])*2)+2)  )

/* the store type */
extern struct TMValueType ASIDP;

#define INIT_SIZE		100	/* start size of core array */
#define TYPE_TABLE_SIZE_HINT	100	/* is passed to tm_table_new() */
#define VALUE_TABLE_SIZE_HINT	10000	/* is passed to tm_table_new() */
#define TOPIC_TABLE_SIZE_HINT	10000	/* is passed to tm_table_new() */
#define FOO 80000
#define THRESHOLD 100

/** @todo Consider parameterizing sizes and size-hints via user params */

/* Here are the prototypes for the functions stored in the object struct: */
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
static int _cmp_v(const void *lhs, const void *rhs);
static unsigned int _hash_v(const void *what);

/* store type implementation */
struct TMValueStoreType default_mem_asidp_store_type = {
	&ASIDP,

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
struct topic_idx_bucket
{
	int len;
	TMTable vid_table;
	TMList vid_list;
};
/* store implementation */
struct default_mem_asidp_store {
	TMValueStoreType type;
	struct data *data;
	TMValueType vtype;
	int count;
	int size;
	TMTopic **asidps;
	TMTable type_table;
	TMTable value_to_vid_table;
	int topic_index_size;
	struct topic_idx_bucket *topic_index;
	
};
/* for convenience we typedef a local T */
typedef struct default_mem_asidp_store *T;


/* local 'helper' functions */
static int _asidps_find_value(T self,const void* v,TMVid *pvid);
/*
static struct asidp_value *_asidp_value_copy(struct asidp_value *v);
*/
static TMTopic *_make_array(struct asidp_value *v); 
static struct asidp_value *_asidp_value_from_array(TMTopic *v) ;
static int _insert_into_indexes(T self,int vid,TMTopic *array);
static int _remove_from_indexes(T self,int vid,TMTopic *array);

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
	self->asidps = (TMTopic**)TM_ALLOC(INIT_SIZE * sizeof(TMTopic*) );
	self->size = INIT_SIZE;
	for(i=0;i<INIT_SIZE;i++)
		self->asidps[i] = NULL;
	self->type_table = tm_table_new(TYPE_TABLE_SIZE_HINT,tm_strcmp_v,tm_strhash_v);
	self->value_to_vid_table = tm_table_new(VALUE_TABLE_SIZE_HINT,_cmp_v,_hash_v);
	self->topic_index = (struct topic_idx_bucket *)TM_ALLOC(INIT_SIZE * sizeof(struct topic_idx_bucket) );
	self->topic_index_size = INIT_SIZE;
	for(i=0;i<INIT_SIZE;i++)
	{
		self->topic_index[i].len = 0;
		self->topic_index[i].vid_table = NULL;
		self->topic_index[i].vid_list = NULL;
	}
	
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

	/* if not explicit CREATE, try to find first */
	if(flag != TM_VALUE_CREATE)
	{
		if(_asidps_find_value(self,v,pvid))
			return TM_OK;
		/* if only lookup return in any case */
		if(flag == TM_VALUE_LOOKUP)
			return TM_OK;
	}


	self->count++;	/* we apend to array. FIXME: propably reuse VIDS later on (with freelist) */
	/* grow core array if too small for new VID */
	if(self->count >= self->size)
	{
		int i;
		self->size *= 2;
		TM_RESIZE(self->asidps,(self->size * sizeof(TMTopic*)) );
		for(i=self->count;i<self->size;i++)
			self->asidps[i] = NULL;
		TMTRACE(TM_STORAGE_TRACE,"asidp store: resize to %d\n" _ self->size);
	}

	self->asidps[self->count] = _make_array((struct asidp_value *)v); 
	_insert_into_indexes(self,self->count,self->asidps[self->count]);
	tm_table_put(self->value_to_vid_table,self->asidps[self->count],(void*)self->count);
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
	*vp = (void*) _asidp_value_from_array(self->asidps[vid]);

	return TM_OK;
}
TMError _memasidpstore_delete_value(TMValueStore s,TMVid vid)
{
	T self;
	TMVid check_vid = 0;
	TMTopic *array;
	assert(s);
	self = (T)s;

	assert(vid > 0);
	assert(vid <= self->count);
	assert(self->asidps[vid]);	/* caller mustr make sure no to-be-deleted values exist */
	array = self->asidps[vid];
	self->asidps[vid] = NULL;	/* FIXME: propably reuse VIDs with freelist (see other FIXME) */

	_remove_from_indexes(self,vid,array);
	check_vid = (TMVid)tm_table_remove(self->value_to_vid_table,array);
	assert(check_vid == vid);
	check_vid = (TMVid)tm_table_get(self->value_to_vid_table,array);
	assert(check_vid == 0);

	TM_FREE(array);
	return TM_OK;
}

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
			void *v;
			v = _asidp_value_from_array(self->asidps[i]);
			tm_value_to_string(self->vtype,v,buf,sizeof(buf));
               		fprintf(f,"%6d -> %s\n", i, self->asidps[i] ? buf : "[deleted]" );
			tm_value_delete(self->vtype,&v);
		}
        }
	return TM_OK;
}


/* -----------------------------------------------------------------------
 *
 *                             STATIC FUNCTIONS
 *
 * ----------------------------------------------------------------------- */
int _insert_into_indexes(T self,int vid,TMTopic *array)
{
	TMTopic i,len;
	len = NELEMS(array);
	TMTRACE(TM_STORAGE_TRACE,"enter (len is %d ) \n" _ len);
	assert(len >= 6);
	for(i=0;i<len;i++)
	{
		TMTopic t;
		struct topic_idx_bucket *bp;
		
		if(i == 1) continue;  /* skip length field */
		t = array[i];



		if(t >= self->topic_index_size)
		{
			int j;
			int old_size = self->topic_index_size;
			while(self->topic_index_size <= t)
				self->topic_index_size *=2;
			TM_RESIZE(self->topic_index,(self->topic_index_size * sizeof(struct topic_idx_bucket)) );
			for(j=old_size;j<self->topic_index_size;j++)
			{
				self->topic_index[j].len = 0;
				self->topic_index[j].vid_table = NULL;
				self->topic_index[j].vid_list = NULL;
			}
			TMTRACE(TM_STORAGE_TRACE,"topic index store: resize to %d\n" _ self->topic_index_size);
		}

		assert(t < self->topic_index_size);
		TMTRACE(TM_STORAGE_TRACE,"now putting %d to topic index\n" _ t);
		bp = &(self->topic_index[t]);
		bp->len++;
		if(bp->vid_table)    /* if table, store there in any case. */
		{
			tm_table_put(bp->vid_table,
				(void*)vid,(void*)1);
			continue;
			/* ARRRGGGG!
			return (1);
			*/
		}
		if(bp->len <= THRESHOLD)
		{
			int found = 0;
			TMList lp;
			for(lp=bp->vid_list;lp;lp=lp->next)
			{
				if(vid == (TMVid)lp->content)
				{
					found=1;
					break;
				}
			}
			if(!found)
			{
				bp->vid_list = tm_list_push(
					bp->vid_list,(void*)vid);
			}
		}
		else
		{
			TMList lp;
			assert(bp->vid_table == NULL);
			assert(bp->vid_list);
			bp->vid_table = tm_table_new(10000,NULL,NULL);
			for(lp = bp->vid_list;lp;lp=lp->next)
			{
				tm_table_put(bp->vid_table,
					lp->content,(void*)1);
			}
			tm_list_delete(&(bp->vid_list));
			assert(bp->vid_list == NULL);
			
			tm_table_put(bp->vid_table, (void*)vid,(void*)1);
		}
		
	}
	return (1);
}
int _remove_from_indexes(T self,int vid,TMTopic *array)
{
	TMTopic i,len;
	len = NELEMS(array);
	assert(len >= 6);
	for(i=0;i<len;i++)
	{
		TMTopic t;
		struct topic_idx_bucket *bp;
		if(i == 1) continue;  /* skip length field */
		t = array[i];
		bp = &(self->topic_index[t]);
		
		if(bp->vid_table)
			tm_table_remove(bp->vid_table,(void*)vid);
		else
		{
			TMList lp;
			bp->vid_list = tm_list_remove(bp->vid_list,(void*)vid,NULL);
			for(lp= bp->vid_list;lp;lp=lp->next)
			{ assert( (TMVid)lp->content != vid); }
			
		}
		bp->len--;
	}
	return (1);
}

int _asidps_find_value(T self,const void* v,TMVid *pvid)
{
	TMVid vid;
	TMTopic *av_array;

	*pvid = 0;
	av_array = _make_array((struct asidp_value*)v);
	vid = (TMVid)tm_table_get(self->value_to_vid_table,av_array);
	if(vid)
	{
		/*
		int i;
		*/
		assert(self->asidps[vid]);
		assert(self->asidps[vid][1] == av_array[1]);
		/*
		for(i=0;i<(av_array[1]*2)+2;i++) { fprintf(stderr,"%d,",self->asidps[vid][i]);} fprintf(stderr,"\n"); 
		for(i=0;i<(av_array[1]*2)+2;i++) { fprintf(stderr,"%d,",av_array[i]);} fprintf(stderr,"\n\n\n"); 
		*/
		assert(memcmp(self->asidps[vid],av_array,NBYTES(av_array)) == 0);
		*pvid = vid;
		TM_FREE(av_array);  /* FIXME: add possibility to pass automatic array to make_array?? */
		return (1);
	}
	TM_FREE(av_array);  /* FIXME: add possibility to pass automatic array to make_array?? */
	return (0);
}
/*
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
*/
TMTopic *_make_array(struct asidp_value *v) 
{
	TMTopic *n;
	int len,i;
	TMList lp;
	TM_NEW(n);
	TMTRACE(TM_STORAGE_TRACE,"enter\n");
	len = tm_list_size(v->memberships);
			/* |T|N|R1|x1|R2|x2|...| */
	n = (TMTopic*)TM_ALLOC( (2+ (len*2)) * sizeof(TMTopic)  );
	n[0] = v->type;
	n[1] = len;
	TMTRACE(TM_STORAGE_TRACE,"type=%d, len=%d\n" _ n[0] _ n[1] );
	for(i=2,lp=v->memberships;lp;i+=2,lp=lp->next)
	{
		struct a_membership *m;
		TM_NEW(m);
		n[i] = ((struct a_membership *)lp->content)->role;
		n[i+1] = ((struct a_membership *)lp->content)->player;
		if(i>2) assert(n[i] > n[i-2]); /* check that memberships are really sorted */	
		TMTRACE(TM_STORAGE_TRACE,"pair: (%d,%d)\n" _ n[i] _ n[i+1]);
	}

	return n;
}
struct asidp_value *_asidp_value_from_array(TMTopic *v) 
{
	struct asidp_value *n;
	int len,i;
	TM_NEW(n);
	n->type = v[0];
	n->memberships = NULL;
	len = v[1];
	for(i=0; i< len; i++)
	{
		struct a_membership *m;
		int offset = 2 + (i*2);
		TM_NEW(m);
		m->role = v[ offset  ];
		m->player = v[ offset+1 ];
		n->memberships = tm_list_append(n->memberships,m);
	}

	return n;
}

struct sdl
{
	TMList list;
	TMList lp;
};

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
                	if( self->asidps[i][0] == (TMTopic)scan->arg)
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
			int j,len;
			TMTopic *array;
			if( ! self->asidps[i] )
				continue;
			array = self->asidps[i];
			len = array[1];
			for(j=0;j<len;j++)
			{
				int offset = 2 + (j*2);
				if(array[offset+1] == argpair->player && array[offset] == argpair->role)
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
	case TM_OP_ASIDP_VALUE_INCLUDES_TOPICS: {
		TMList vid_list = NULL;
		
		
		TMTRACE(TM_STORAGE_TRACE,"op=VALUE_INCLUDES_TOPICS, topic=%d\n" _
				(TMTopic)scan->arg );
		/* FIXME: is that correct? */
		/*
		assert(self->topic_index_size > (TMTopic)scan->arg);
		*/

		if(self->topic_index_size > (TMTopic)scan->arg)
		{
		if(self->topic_index[(TMTopic)scan->arg].vid_table)
			tm_table_keys(self->topic_index[(TMTopic)scan->arg].vid_table , &vid_list);
		else
			vid_list = tm_list_copy(self->topic_index[(TMTopic)scan->arg].vid_list);
		}

		scan->data = TM_ALLOC(sizeof(struct sdl));
		((struct sdl*)scan->data)->list = vid_list;
		((struct sdl*)scan->data)->lp = vid_list;
		if( ((struct sdl*)scan->data)->list)
		{
			scan->next_vid = (TMVid)vid_list->content;
		}
#if 0
		else
		{
			int i;
			for(i=1; i<=self->count; i++)
			{
				int j,len;
				TMTopic *array;
				if( ! self->asidps[i] )
					continue;
				array = self->asidps[i];
				assert(array[0] != (TMTopic)scan->arg );
				len = array[1];
				for(j=0;j<len;j++)
				{
					int offset = 2 + (j*2);
					assert(array[offset] != (TMTopic)scan->arg );
					assert(array[offset+1] != (TMTopic)scan->arg );
				}
			}

		}
#endif
		
		break; }
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
			if( self->asidps[i][0] == (TMTopic)scan->arg)
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
			int j,len;
			TMTopic *array;
			if( ! self->asidps[i] )
				continue;
			array = self->asidps[i];
			len = array[1];
			for(j=0;j<len;j++)
			{
				int offset = 2 + (j*2);
				if(array[offset+1] == argpair->player && array[offset] == argpair->role)
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
		scan->next_vid = 0;	/* assume */
		if( ((struct sdl*)scan->data)->lp == NULL)
		{
			tm_list_delete( &(((struct sdl*)scan->data)->list) );
			TM_FREE(scan->data);
			break;
		}
		((struct sdl*)scan->data)->lp = ((struct sdl*)scan->data)->lp->next;
		if( ((struct sdl*)scan->data)->lp)
		{
			scan->next_vid = (TMVid)((struct sdl*)scan->data)->lp->content; 
		}
		break;
	}

	TMTRACE(TM_STORAGE_TRACE,"exit\n");
	return TM_OK;
}

int _cmp_v(const void *lhs, const void *rhs)
{
	TMTopic *lhs_array,*rhs_array;
	lhs_array = (TMTopic*)lhs;
	rhs_array = (TMTopic*)rhs;
	if(lhs_array[1] != rhs_array[1])
		return -1;
	return memcmp(lhs,rhs, NBYTES(lhs_array));
}
 
/*
unsigned int tm_strhash_v(const void *what)
{
        char *s = (char *)what;
        unsigned long h = 0;
        while (*s)
                h = (h << 5) + h + (unsigned char)*s++;
        return h;
}
*/
unsigned int _hash_v(const void *what)
{
	TMTopic *array;
	int len,i;
        unsigned char *s;
        unsigned int h = 0;
	array = (TMTopic*)what;
	len = NBYTES(array);
        for(i=0,s=(unsigned char *)what; i<len; i++,s++)
                h = 31 * h + *s;
        return h;
}

