#include "tmparams.h"
#include "tmmem.h"
#include "tmassert.h"
#include "tmutil.h"
#include "tmlist.h"


struct bucket {
	char *name;
#define SINGLE 1
#define MULTI  2
	int type;
	char *value;
	char **values;
};

struct TMParams {
	TMList list;
};
 

TMParams tm_params_new(const char *s,int type)
{
	TMParams p;
	const char *q = s;
	assert(s);
	assert(type == TM_PARAMS_SIMPLE);

	TM_NEW(p);
	p->list = NULL;

	while(*q)
	{
		struct bucket *bp;
		const char *k, *v;
		TM_NEW(bp);
		k = q;
		while(*q && *q != '=') q++;
		assert(*q);
		bp->name = tm_strndup(k,q-k);
		q++; /* skip = */
		assert(*q);
		v = q;
		while(*q && *q != ';') q++;
		bp->value = tm_strndup(v,q-v);
		bp->type = SINGLE;
		p->list = tm_list_push(p->list,bp);	
		if(*q == ';') q++;
		/*
		printf("_%s_ -> _%s_\n",bp->name,bp->value);
		*/
	}	
	
	return (p);
}

void tm_params_delete(TMParams *pself)
{
	assert(pself && *pself);
	/* FIXME: free rest */
	TM_FREE(*pself);
}

const char *tm_params_get(TMParams self, const char *name)
{
	TMList lp;
	assert(self);
	assert(name);
	for(lp=self->list;lp;lp=lp->next)
	{
		struct bucket *bp = (struct bucket*)lp->content;
		
		if( strcmp(bp->name,name) == 0)
		{
			assert(bp->type == SINGLE);
			return (bp->value);
		}
	}
	return NULL;
}

