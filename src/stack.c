#include <tmstack.h>
#include <tmmem.h>
#include <tmassert.h>

struct TMStack {
	int count;
	struct elem {
		 void *content;
		struct elem *link;
	} *head;
};


TMStack tm_stack_new(int size_hint)
{
	TMStack s;

	TM_NEW(s);
	s->count = 0;
	s->head = NULL;
	return s;
}

void tm_stack_delete(TMStack *pself)
{
	struct elem *t, *u;

	assert(pself && *pself);
	for( t = (*pself)->head; t; t = u)
	{
		u = t->link;
		TM_FREE(t);
	}
	TM_FREE(*pself);
}

int tm_stack_empty(TMStack self)
{
	assert(self);
	return(self->count == 0);
}

int tm_stack_size(TMStack self)
{
	assert(self);
	return(self->count);
}

void tm_stack_push(TMStack self, void *content)
{
	struct elem *t;

	TM_NEW(t);
	t->content = content;
	t->link = self->head;
	self->head = t;
	self->count++;
}

void *tm_stack_pop(TMStack self)
{
	 void *content;
	struct elem *t;

	assert(self);
	assert(self->count > 0);
	t = self->head;
	self->head = t->link;
	self->count--;
	content = t->content;
	TM_FREE(t);
	return content;
}

void *tm_stack_top(TMStack self) 
{
	assert(self);
	assert(self->count > 0);
	return self->head->content;
}


#ifdef TEST 
#include <tmutil.h>
 
int main(int argc, char** args)
{
	GW gw;
	TMStack s;
	printf("\nUnit tests for %s:\n",__FILE__);
 
	gw = GW_init(__FILE__,"1.0");
	GWTEST(gw != NULL);
 
        s = gs_stack_new(gw);

	gs_stack_push(s,(void*)1); 
	GWTEST(gs_stack_top(s) == (void*)1);

	gs_stack_push(s,(void*)2); 
	GWTEST(gs_stack_top(s) == (void*)2);

	GWTEST(gs_stack_pop(s) == (void*)2); 
	GWTEST(gs_stack_top(s) == (void*)1);

	gs_stack_pop(s); 
	GWTEST(gs_stack_size(s) == 0);

	gs_stack_delete(&s);
	GWTEST(s == NULL);
	return 0;
}
 
 
#endif

