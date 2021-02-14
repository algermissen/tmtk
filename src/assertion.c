#include <tmassertion.h>
#include <tmassert.h>

/* as in struct TMTopicAssertion */
struct membership {
	TMTopic R;
	TMTopic C;
	TMTopic x;
};

static int _cmp(const void *x, const void *y)
{
	struct membership *m,*n;
	
	m = (struct membership*)x;
	n = (struct membership*)y;

	if(m->R > n->R)
		return 1;
	else if(m->R < n->R)
		return -1;
	/*
	assert(!"coming here implies multiple players of a single role and that is forbidden");
	*/
	return 0;
}

void tm_topic_assertion_sort(TMTopicAssertion self)
{
	int i;
	qsort(self->memberships,self->N,sizeof(struct membership),_cmp);
	for(i = 1;i<self->N; i++)
	{
			/* check the sort */
		assert(self->memberships[i].R >= self->memberships[i-1].R);
	}
#if 0
FIXME: debugging code commented 6. Dec. 2003, because we use this struct now also
for assembilng 'broken' assertions. and then restructuring them
	for(i = 1;i<self->N; i++)
	{
			/* multiple players of a role forbidden ! */
		assert(self->memberships[i].R != self->memberships[i-1].R);
			/* check the sort */
		assert(self->memberships[i].R > self->memberships[i-1].R);
	}
#endif
}
void tm_topic_assertion_init(TMTopicAssertion self)
{
	assert(self);
	bzero(self,sizeof(*self));
}
void tm_topic_assertion_print(TMTopicAssertion self,FILE* f)
{
	int i;
	assert(self);
	assert(f);
	fprintf(f,"[%6d %6d ",self->T,self->A);
	for(i=0;i<self->N;i++)
	{
		fprintf(f," (%6d %6d %6d)",
			self->memberships[i].R,
			self->memberships[i].C,
			self->memberships[i].x);
	}
	fprintf(f,"]\n");
}

TMTopic tm_topic_assertion_get_player_of_role(TMTopicAssertion self,TMTopic rtopic)
{
	int i;
	assert(self);
	assert(rtopic);

	for(i=0;i<self->N;i++)
	{
		if(self->memberships[i].R == rtopic)
			return self->memberships[i].x;
	}

	return TM_NO_NODE;	
}
TMTopic tm_topic_assertion_get_casting_of_role(TMTopicAssertion self,TMTopic rtopic)
{
	int i;
	assert(self);
	assert(rtopic);

	for(i=0;i<self->N;i++)
	{
		if(self->memberships[i].R == rtopic)
			return self->memberships[i].C;
	}

	return TM_NO_NODE;	
}

#if 0
void TMSubject_init(TMSubject self)
{
	assert(self);
	bzero(self,sizeof(*self));
}



void TMSubject_addProperty(TMSubject self, const char *model,const char* prop, const void *val)
{
	int offset = self->n * 3;
	assert(self);
	
	
	self->props[offset] = model;
	self->props[offset+1] = prop;
	self->props[offset+2] = (char*)val;
	self->n++;
}

int TMSubject_getNumberOfProperties(TMSubject self)
{
	assert(self);
	return self->n;
}
const char **TMSubject_getProperty(TMSubject self, int n)
{
	assert(self);
	assert(n < self->n);
	return self->props + (n * 3); 
}

 
void TMAssertion_init(TMAssertion self)
{
	int i;
        assert(self);
	TMSubject_init(&self->type_subject);
	TMSubject_init(&self->assertion_subject);

	self->nmember=0;
	for(i=0;i<GS_MAXMEMBERS;i++)
	{
		TMSubject_init( &(self->members[i].role_subject) );
		TMSubject_init( &(self->members[i].casting_subject) );
		TMSubject_init( &(self->members[i].player_subject) );
	}
	
	
}
 
void TMAssertion_setTypeSubject(TMAssertion self, const char *model, const char *propname, void *propval)
{
        assert(self);
        assert(model);
        assert(propname);
        assert(propval);

	TMSubject_addProperty( &self->type_subject, model, propname, propval);	 
 
}
TMSubject TMAssertion_getTypeSubject(TMAssertion self)
{
        assert(self);
	return &self->type_subject;
}
TMSubject TMAssertion_getAssertionSubject(TMAssertion self)
{
        assert(self);
	return &self->assertion_subject;
}
void TMAssertion_setAssertionSubject(TMAssertion self, const char *model, const char *propname, void *propval)
{
        assert(self);
        assert(model);
        assert(propname);
        assert(propval);

	TMSubject_addProperty( &self->assertion_subject, model, propname, propval);	 
 
}

int TMAssertion_getNumberOfMemberships(TMAssertion self)
{
	assert(self);
	return self->nmember;
}

void TMAssertion_setMembershipRoleSubject(TMAssertion self, int n, const char *model, const char *propname, void *propval)
{
	TMSubject s;
	assert(self);
	assert(n >= 0);
	assert(n < GS_MAXMEMBERS-1);
	assert(model);
	assert(propname);

	if(n > self->nmember-1)
		self->nmember++;

	s = &(self->members[n].role_subject);
	TMSubject_addProperty( s, model, propname, propval);	 
}
void TMAssertion_setMembershipCastingSubject(TMAssertion self, int n, const char *model, const char *propname, void *propval)
{
	TMSubject s;
	assert(self);
	assert(n >= 0);
	assert(n < GS_MAXMEMBERS-1);
	assert(model);
	assert(propname);

	if(n > self->nmember-1)
		self->nmember++;

	s = &(self->members[n].casting_subject);
	TMSubject_addProperty( s, model, propname, propval);	 
}
void TMAssertion_setMembershipPlayerSubject(TMAssertion self, int n, const char *model, const char *propname, void *propval)
{
	TMSubject s;
	assert(self);
	assert(n >= 0);
	assert(n < GS_MAXMEMBERS-1);
	assert(model);
	assert(propname);

	if(n > self->nmember-1)
		self->nmember++;

	s = &(self->members[n].player_subject);
	TMSubject_addProperty( s, model, propname, propval);	 
}

TMSubject TMAssertion_getMembershipRoleSubject(TMAssertion self, int n)
{
	assert(self);
	assert(n >= 0);
	assert(n < self->nmember);

	return &(self->members[n].role_subject);
}
TMSubject TMAssertion_getMembershipCastingSubject(TMAssertion self, int n)
{
	assert(self);
	assert(n >= 0);
	assert(n < self->nmember);

	return &(self->members[n].casting_subject);
}
TMSubject TMAssertion_getMembershipPlayerSubject(TMAssertion self, int n)
{
	assert(self);
	assert(n >= 0);
	assert(n < self->nmember);

	return &(self->members[n].player_subject);
}

#endif



#ifdef TEST

int main(int argc,char**argv)
{
	
	struct TMSubject s;

	TMSubject_init(&s);
	TMSubject_addProperty(&s,"SAM","SubjectIndicator","http://www.topicmaps.org/index.html");


	/*
	add_prop(&s,"SAM","SubjectIndicator",List_push(NULL,"http://www.topicmaps.org/" );
	*/


	printf("Unittest for %s\n",argv[0]);

	return 0;
}

#endif
