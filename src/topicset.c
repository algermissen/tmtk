#include "tmtopicset.h" 
#include "tmmem.h"
#include "tmtrace.h"
#include "tmassert.h"

struct TMTopicSet {
	int count;
	int size;
	TMTopic *topics;
};


TMTopicSet tm_topicset_new(int size_hint)
{

	/* ONCE HINT IS IMPLEMENTED, assert(hint) to break 0 values used now */ 
	TMTopicSet s;

	TM_NEW(s);
	s->count = 0;
	s->size = 10;
	s->topics = (TMTopic*)TM_ALLOC(10 * sizeof(TMTopic));
	assert(s->topics);
	return s;
}

void tm_topicset_delete(TMTopicSet *pself)
{

	assert(pself && *pself);
	TM_FREE((*pself)->topics);
	TM_FREE(*pself);
}
void tm_topicset_clear(TMTopicSet self)
{
	assert(self);
	self->count = 0;
}
void tm_topicset_print(TMTopicSet self, FILE* f)
{
	int i;
	if(self->count == 0) {
		fprintf(f,"[]");
		return;
	}
	fprintf(f,"[");
	for(i = 0; i < self->count-1; i++)
		fprintf(f,"%d,",self->topics[i] );
	fprintf(f,"%d]",self->topics[self->count-1] );
	/*
	fprintf(f,"]");
	*/

}

int tm_topicset_empty(TMTopicSet self)
{
	assert(self);
	return(self->count == 0);
}
int tm_topicset_size(TMTopicSet self)
{
	assert(self);
	TMTRACE(TM_GRAPH_TRACE,"tm_topicset_size(): size=%d\n" _ self->count);
	return(self->count);
}

void tm_topicset_add(TMTopicSet self, TMTopic topic)
{
	int i;
	assert(self);
	i=0;
	/*
	fprintf(stderr,"NODESET: adding topic %d\n",topic);
	*/
	for(i = 0 ; i < self->count; i++)
		if(self->topics[i] == topic)
			return;

	if(self->count >= self->size)
	{
		self->size *= 2;
		TM_RESIZE(self->topics,self->size * sizeof(TMTopic) );
	}
	/* TBD: sort !!! */
	self->topics[self->count] = topic;
	self->count++;
}
int tm_topicset_remove(TMTopicSet self, TMTopic topic)
{
	int i;
	assert(self);
	i=0;
	for(i = 0 ; i < self->count; i++)
	{
		if(self->topics[i] == topic)
		{
			int j;
			for(j=i;j<self->count-1;j++)
			{
				self->topics[j] = self->topics[j+1];

			}
			self->count--;
			return 1;	
		}
	}

	return (0);

}
TMTopic tm_topicset_get(TMTopicSet self, int i)
{
	assert(self);
	assert(i >= 0 && i < self->count);
	return self->topics[i];
}
int tm_topicset_contains(TMTopicSet self, TMTopic topic)
{
	int i;
	assert(self);
	assert(topic);
	for(i = 0 ; i < self->count; i++)
		if(self->topics[i] == topic)
			return 1;
	return 0;
}
int tm_topicset_disjoint(TMTopicSet self, TMTopicSet other)
{
	int i;
	assert(self);
	assert(other);
	for(i = 0 ; i < other->count; i++)
	{
		if(tm_topicset_contains(self,other->topics[i]) )
			return 0;
	}
	return 1;
}
int tm_topicset_equal(TMTopicSet self, TMTopicSet other)
{
	int i;
	assert(self);
	assert(other);
	
	if(self->count != other->count)
		return 0;

	for(i = 0 ; i < other->count; i++)
	{
		if(! tm_topicset_contains(self,other->topics[i]) )
			return 0;
	}
	return 1;
}
TMTopicSet tm_topicset_copy(TMTopicSet self)
{
	TMTopicSet cpy;
 
        TM_NEW(cpy);
        cpy->count = self->count;
        cpy->size = self->size;
        cpy->topics = (TMTopic*)TM_ALLOC(self->size * sizeof(TMTopic));
        assert(cpy->topics);
	memcpy(cpy->topics,self->topics, self->count * sizeof(TMTopic));
	
	assert( tm_topicset_equal(self,cpy));
	assert( tm_topicset_equal(cpy,self));
        return cpy;
}


#ifdef TEST
 
int main(int argc, char** args)
{
	GW gw;
	TMTopicSet s;
	printf("\nUnit tests for %s:\n",__FILE__);
 
	gw = GW_init(__FILE__,"1.0");
	GWTEST(gw != NULL);
 
        s = tm_topicset_new(gw);

	tm_topicset_push(s,(void*)1); 
	GWTEST(tm_topicset_top(s) == (void*)1);

	tm_topicset_push(s,(void*)2); 
	GWTEST(tm_topicset_top(s) == (void*)2);

	GWTEST(tm_topicset_pop(s) == (void*)2); 
	GWTEST(tm_topicset_top(s) == (void*)1);

	tm_topicset_pop(s); 
	GWTEST(tm_topicset_size(s) == 0);

	tm_topicset_delete(&s);
	GWTEST(s == NULL);
	return 0;
}
 
 
#endif
