#include "tmvaluetype.h"
#include "tmtrace.h"
#include "tmmem.h"
#include "tmlist.h"
#include "tmutil.h"
#include "tmassert.h"
#include "tmbasictypes.h"


static int locatorset_equal(TMValueType, const void *, const void*);
static int locatorset_to_string(TMValueType, const void *set, char* buf, size_t size);
/*
static int locatorset_to_xml(TMValueType, const void *set, char* buf, size_t size);
*/
static void locatorset_free(TMValueType vtype, void *v);
static void *locatorset_new(TMValueType vtype);
static void *locatorset_new_from_string(TMValueType vtype, const char *s);

static void* locatorset_union(TMValueType, void *vself,va_list args);
static void *locatorset_parse_argstring(TMValueType vtype, int opcode, const char *argstring);

#undef TMTRACE
#define TMTRACE(flag,arg)


static struct function_table ftab[] = {
	{ "union" , locatorset_union },
	{ NULL , NULL}
};
static struct op_table ops[] = {
	{ "NOT_DISJOINT" , TM_OP_TextSet_NOT_DISJOINT },
	{ "INCLUDES" , TM_OP_TextSet_INCLUDES },
	{ "LIKE" , TM_OP_TextSet_LIKE },
	{ NULL, 0 }
};



struct TMValueType TextSet = {
        "TextSet",
	0,			/* values do not contain topics */
	locatorset_new,
	locatorset_new_from_string,
	locatorset_to_string,
	locatorset_to_string,
	locatorset_equal,
	locatorset_free,
	ftab,
	ops,
	locatorset_parse_argstring	
};

static char trunc_msg[] = "...[value truncated]";

void *locatorset_new(TMValueType vtype)
{
	/* the empty TMList is simply NULL */
	return NULL;
}

void *locatorset_new_from_string(TMValueType vtype, const char *s)
{
	TMList self = NULL;
	char *t;
	/* FIXME: support multiple elements! */
	assert(*s == '[');
	assert(s[strlen(s)-1] == ']');
	assert(strchr(s,',') == NULL);

	s++;
	t = tm_strdup(s);
	t[strlen(t)-1] = '\0';
	self = tm_list_push(self, t);
	return self;
}


int locatorset_to_string(TMValueType vtype, const void *set, char* buf, size_t size)
{
	static char errmsg[] = "*** ERROR *** [size of buffer too small]";
	TMList lp;
	char *save_buf = buf;
	int _size = size;
	int n;
	assert(size > strlen(trunc_msg));

	if(set == NULL)
	{
		*buf = '\0';
		/* FIXME: retval issue!! */
		return (1);
	}

	_size--;	/* room for trailing \0 */

	
	TMTRACE(TM_VALUETYPE_TRACE,"enter (name=%s)\n" _ vtype->name);
	assert(_size >= 1);
	assert(_size >= strlen(errmsg) );
	snprintf(buf,_size,"[");
	buf++;
	_size--;
	assert(set);


	for(lp= (TMList)set;lp;lp=lp->next)
	{
		char b[4096];
		assert(lp->content);
	
		n = snprintf(b,sizeof(b),"'%s'",lp->content);
		TMTRACE(TM_VALUETYPE_TRACE,"b=%s\n" _ b);
		TMTRACE(TM_VALUETYPE_TRACE,"n=%d\n" _ n);
		TMTRACE(TM_VALUETYPE_TRACE,"_size=%d\n" _ _size);
		if( n > _size)
		{
			return (snprintf(save_buf,size,"%s",errmsg));
		}
			
		n = snprintf(buf,_size,"%s,",b);
		assert(n>0);
		_size -= n;
		assert(n>=0);
		buf += n;
		
	}
	buf--;	/* delete last ,FIXME: _size++ ? */
	
	if(_size < 1)
	{
		return (snprintf(save_buf,size,"%s",errmsg));
	}
	n = snprintf(buf,_size,"]");
	assert(n>0);
	buf++;
	_size--;
	return (size - _size);
}
#if 0
int locatorset_to_xml(TMValueType vtype, const void *set, char* buf, size_t size)
{
	TMValueType subtype;
	TMList lp;
	int _size = size;
	char *save_buf = buf;
	int x;


	assert(_size > strlen(trunc_msg));
	subtype = (TMValueType)vtype->structure;
	x = snprintf(buf,_size,"    <set>\n");
	assert(x < _size);
	buf += x;
	_size -= x;
	assert(set);

	for(lp= (TMList)set;lp;lp=lp->next)
	{
		int n;
		char b[132000];
		assert(lp->content);
		n = subtype->primary_component->to_xml(subtype,lp->content,b,sizeof(b));
		
		if(_size < strlen(b) )
		{
			fprintf(stderr,"size: %d, [%s]\n",_size,b);
			assert(0);
			snprintf(buf,_size,"%s\n",b);
			return (sprintf(save_buf+(size-strlen(trunc_msg)),"%s",trunc_msg));
		}
		n = snprintf(buf,_size,"%s\n",b);
		assert(n < _size);
		_size -= n;
		buf += n;
		
	}
	
	if(_size < 11)
	{
		assert(0);
		return (sprintf(save_buf+(size-strlen(trunc_msg)),"%s",trunc_msg));
	}
	x = snprintf(buf,_size,"    </set>");
	assert(x < _size);
	buf += x;
	_size -= x;
	return (size - _size);
}
#endif

int locatorset_equal(TMValueType vtype, const void *lhs, const void *rhs)
{
        TMList p1,p2;
	assert(vtype);
	assert(lhs);
	assert(rhs);

	TMTRACE(TM_VALUETYPE_TRACE,"enter, type=%s\n" _ vtype->name);


	TMTRACE(TM_VALUETYPE_TRACE,"loop1\n"); 
        for(p1 = (TMList)lhs; p1; p1 = p1->next)
        {
		int found = 0;
		assert(p1->content);
		TMTRACE(TM_VALUETYPE_TRACE,"outer \n"); 
                for(p2 = (TMList)rhs; p2; p2 = p2->next)
                {
			TMTRACE(TM_VALUETYPE_TRACE,"inner before \n"); 
			
                        if( strcmp( (char*)p1->content,(char*)p2->content) == 0)
			{
				found = 1;
				break;
			}
			TMTRACE(TM_VALUETYPE_TRACE,"inner after\n"); 
                }
		if(!found)
			return 0;
        }
	TMTRACE(TM_VALUETYPE_TRACE,"loop2\n"); 
        for(p2 = (TMList)rhs; p2; p2 = p2->next)
        {
                int found = 0;
                for(p1 = (TMList)lhs; p1; p1 = p1->next)
                {
                        if( strcmp( (char*)p1->content,(char*)p2->content) == 0)
			{
				found = 1;
				break;
			}
                }
		if(!found)
			return 0;
        }
        return 1;

}
void locatorset_free(TMValueType vtype, void *v)
{
	TMList lp;
		
	for(lp= (TMList)v;lp;lp=lp->next)
	{
		assert(lp->content);
		TM_FREE(lp->content);
	}
	lp = (TMList)v;
	tm_list_delete(&lp);
#if 0
	assert( v == NULL); /* just a check if delete works the way I expect (FIXME) */
#endif
}

void *locatorset_union(TMValueType vtype, void *vself,va_list args)
{
	TMList res = NULL;
	TMList other;
	TMList self = (TMList)vself;
	other = va_arg(args,TMList);
	for( ;self;self=self->next)
	{
		res = tm_list_push(res,self->content);
	}
	for( ;other;other=other->next)
	{
		int found = 0;
		TMList lp;
		for(lp=res;lp;lp=lp->next)
		{
			if( strcmp((char*)lp->content,(char*)other->content) == 0)
			{
				found = 1;
				break;
			}
		}
		if(found)
			continue;
		res = tm_list_push(res,other->content);
	}
	return res;
}
void *locatorset_parse_argstring(TMValueType vtype, int opcode, const char *argstring)
{
	switch(opcode) {
	case TM_OP_TextSet_NOT_DISJOINT: {
		/* FIXME parse [ '....' , '....' ] from argstsring and make list */
		/*
		TMList set = NULL;
		*/
		assert(0);
		return NULL; }
	case TM_OP_TextSet_INCLUDES:
		/* arg is a text (currently just a simple string) */
		return tm_strdup(argstring);
	case TM_OP_TextSet_LIKE:
		/* arg is a text fragment/pattern */
		return tm_strdup(argstring);
	default:
		assert(0);
	}
	return NULL;
}


