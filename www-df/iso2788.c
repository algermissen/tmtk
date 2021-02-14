/*
 * $Id$
 *
 * Copyright (c) 2002 Jan Algermissen
 * See the file "COPYING" for copying permission.
 *
 */

#include <topicmaps.h>

#include <topicmaps.h>
#include <ctype.h>
#include <errno.h>

#include <tmdevel.h>            /* for development macros */                                   
#include "df.h"

static int _handle_line(void *self,const char* line, int isFinal);
static void _delete(void**);

static int _handle(void *v,const char *rel, const char *other);

typedef struct iso2788data *T;
struct iso2788data {

	/* processing model stuff */
	Omnivore omnivore;
	TMTopicMap topicmap;
	char *current_name;
	TMTopic current_topic;
	struct TMSubject current_sbj;

	TMList models;
	TMList rules;
	TMList ignores;
};

static char *models[] = {
	SAM_NAME,
/* you may #include stuff here */
	NULL
};

static char *ignores[] = {
	NULL,
};

struct rule {
	char *rel;
	char *at;
	char *active;
	char *passive;
	TMTopic at_topic;
	TMTopic active_topic;
	TMTopic passive_topic;
	/* FIXME: flag to mark static vs alloced rules to handle dealloc! */
};
static struct rule rules[] = {
	{ "NT", PSI_BASE"at-superclass-subclass",PSI_BASE"role-superclass",PSI_BASE"role-subclass",0,0,0},
	{ "HI", PSI_BASE"at-class-instance",PSI_BASE"role-class",PSI_BASE"role-instance",0,0,0},
	/*
	{ "", "At","Role","Role",0,0,0},
	*/
	{ NULL,NULL,NULL,NULL,0,0,0}
};


static TMError _read_config(T self, const char *filename,Omnivore o);

static TMError _init(Omnivore omnivore,void*arg,void **data)
{
	T self;
	TMError e;
	TMList lp;
	int i;
	struct TMList list;
	TM_NEW(self);
	*data = NULL;

	list.next = NULL;	

	self->topicmap = tm_omnivore_get_topicmap(omnivore);
	self->omnivore = omnivore;
	self->current_name = NULL;
	self->models = NULL;
	self->rules = NULL;
	self->ignores = NULL;

	/* FIXME: handle allocations/data stuff */
	
	if( ! self->topicmap )
	{
		*data = self;
		return TM_OK;
	}

	for(i=0; models[i];i++)
	{
		self->models = tm_list_push(self->models,models[i]);
	}

	for(i=0; rules[i].rel;i++)
	{
		self->rules = tm_list_push(self->rules,&(rules[i]));
	}
	for(i=0; ignores[i];i++)
	{
		self->ignores = tm_list_push(self->ignores,tm_strdup(ignores[i]));
	}

	if(arg)
	{
		if( (e = _read_config(self,(char*)arg,omnivore)) != TM_OK)
		{
			/* FIXME: free! */
			return e;
		}
	}
	for(lp=self->models;lp;lp=lp->next)
	{
		if( (e=tm_topicmap_require_model(self->topicmap,(const char*)lp->content)) != TM_OK)
		{
			tm_omnivore_set_error(omnivore, "unable to require %s , %s\n",
			(const char*)lp->content, tm_get_error(tm_omnivore_get_tm(omnivore)) );
			return e;
		}
	}

	for(lp=self->rules;lp;lp=lp->next)
	{
		struct rule *rp = (struct rule*)lp->content;
		list.content = rp->at;
		if( (e=tm_topicmap_get_topic(self->topicmap,SAM_NAME,"SubjectIndicators",&list,
				&(rp->at_topic),NULL )) != TM_OK)
		{
			tm_omnivore_set_error(omnivore, "unable to get topic for %s, %s\n",
			rp->at, tm_get_error(tm_omnivore_get_tm(omnivore)) );
			return e;
		}
		if(!rp->at_topic)
		{
			tm_omnivore_set_error(omnivore, "no topic for "SAM_NAME"::SubjectIndicators=%s\n",
			rp->at );
			return TM_ETOPIC_NOT_FOUND;
		}
		list.content = rp->active;
		if( (e=tm_topicmap_get_topic(self->topicmap,SAM_NAME,"SubjectIndicators",&list,
				&(rp->active_topic),NULL )) != TM_OK)
		{
			tm_omnivore_set_error(omnivore, "unable to get topic for %s, %s\n",
			rp->active, tm_get_error(tm_omnivore_get_tm(omnivore)) );
			return e;
		}
		if(!rp->active_topic)
		{
			tm_omnivore_set_error(omnivore, "no topic for "SAM_NAME"::SubjectIndicators=%s\n",
			rp->active );
			return TM_ETOPIC_NOT_FOUND;
		}
		list.content = rp->passive;
		if( (e=tm_topicmap_get_topic(self->topicmap,SAM_NAME,"SubjectIndicators",&list,
				&(rp->passive_topic),NULL )) != TM_OK)
		{
			tm_omnivore_set_error(omnivore, "unable to get topic for %s, %s\n",
			rp->passive, tm_get_error(tm_omnivore_get_tm(omnivore)) );
			return e;
		}
		if(!rp->passive_topic)
		{
			tm_omnivore_set_error(omnivore, "no topic for "SAM_NAME"::SubjectIndicators=%s\n",
			rp->passive );
			return TM_ETOPIC_NOT_FOUND;
		}
			
#if 0
		if( (e=tm_topicmap_get_topic(self->topicmap,"org.gooseworks.IS13250","UniqueName",rp->active,
				&(rp->active_topic),NULL )) != TM_OK)
		{
			tm_omnivore_set_error(omnivore, "unable to get topic for %s, %s\n",
			rp->active, tm_get_error(tm_omnivore_get_tm(omnivore)) );
			return e;
		}
		if(!rp->active_topic)
		{
			tm_omnivore_set_error(omnivore, "no topic for org.gooseworks.IS13250::UniqueName=%s\n",
			rp->active );
			return TM_ETOPIC_NOT_FOUND;
		}
		if( (e=tm_topicmap_get_topic(self->topicmap,"org.gooseworks.IS13250","UniqueName",rp->passive,
				&(rp->passive_topic),NULL )) != TM_OK)
		{
			tm_omnivore_set_error(omnivore, "unable to get topic for %s, %s\n",
			rp->passive, tm_get_error(tm_omnivore_get_tm(omnivore)) );
			return e;
		}
		if(!rp->passive_topic)
		{
			tm_omnivore_set_error(omnivore, "no topic for org.gooseworks.IS13250::UniqueName=%s\n",
			rp->passive );
			return TM_ETOPIC_NOT_FOUND;
		}
#endif
	}
	
	*data = self;
	return TM_OK;
}
void _delete(void **pself)
{
	T self = *pself;
	if(self->current_name) { TM_FREE(self->current_name); }
	TM_FREE(self);

}



int _handle_line(void *v,const char* line, int isFinal)
{
	T self;
	TMError e;
	struct TMList list;
	const char *p;
	char rel[256];
	const char *other;
	self = (T)v;
	TMTRACE(TM_PARSE_TRACE,"LINE: _%s_\n" _ line);

	if(! self->topicmap)
		return(1);

	if( tm_is_whitespace_string(line))
		return (1);	
	if( tm_is_comment_string(line,'#'))
		return (1);	

	line = tm_lstrip(line);

	bzero(rel,sizeof(rel));	
	TMTRACE(TM_PARSE_TRACE,"not empty or comment\n");
	TMTRACE(TM_PARSE_TRACE,"line now: _%s_\n" _ line);

	p = line;	
	while( *p && (!isspace(*p)) ) p++;
	if(*p)
	{

		strncpy(rel,line, p-line);	
		other = tm_lstrip(p);
		TMTRACE(TM_PARSE_TRACE,"REL: '%s'\n" _ rel);
		TMTRACE(TM_PARSE_TRACE,"OTHER: '%s'\n" _ other);

		if( _handle(self,rel,other) )
			return(1);

		/* else go make new current */
	}
	if(self->current_name) { TM_FREE(self->current_name); }
	self->current_name = tm_strdup(line);
	list.next = NULL;
	list.content = self->current_name;
	/*
	TM_SET_SUBJECT_N1(&(self->current_sbj),SAM_NAME,"BaseNames",&list);
	*/
#if 1
	{
	char sirbuf[1024];
	struct TMList list2;
	list2.next = NULL;
	sprintf(sirbuf,"http://itil.dkrz.de/cmdb/items/%s",self->current_name);
	list2.content = sirbuf;
	TM_SET_SUBJECT_N2(&(self->current_sbj),
		SAM_NAME,"BaseNames",&list,
		SAM_NAME,"SubjectIndicators",&list2);
	}
#endif

	e = tm_topicmap_add_subject(self->topicmap,&(self->current_sbj),&(self->current_topic) );
	assert(e == TM_OK);


	return (1);
}

struct TMProcModel iso2788procmodel = 
{
	"iso2788",	/* name */
	_init,
	_delete,
	NULL,	/* root element name (for lookup) */
	NULL,	/* XML namespace */
	NULL,   /* XML public ID */
	NULL,   /* URL to DTD */
	/* NULL, -- elements member of struct; unused */

	NULL,	/* XML ID attribute */
	NULL,   /* XML Ref attribute */
	NULL,	/* XML element subtrees to skip */
	NULL,  /* text elements */

	NULL,  /* XML start elem */
	NULL,  /* XML end elem */


	_handle_line,  /* line handler */
	NULL                    /* no RDF Statement-Handler-Table */
};



int _handle(void *v,const char *rel, const char *other)
{
	T self;
	TMError e;
	struct TMList list;
	struct TMSubject sbj;
	struct TMTopicAssertion astn;
	TMTopic t_other;
	TMList lp;
	struct rule *rule = NULL;

	self = (T)v;
	astn.A = 0;

	TMTRACE(TM_PARSE_TRACE,"enter\n");

	for(lp=self->ignores;lp;lp=lp->next)
	{
		if( strcmp(rel,(char*)lp->content) == 0 )
		{
			return(1);  /* say we handled it to prevent making of new current */
		}
	}
	TMTRACE(TM_PARSE_TRACE,"1\n");

	for(lp=self->rules;lp;lp=lp->next)
	{
		struct rule *rp = (struct rule *)lp->content;	

		if( strcmp(rel,rp->rel) == 0 )
		{
			rule = rp;
			break;
		}
	}
	TMTRACE(TM_PARSE_TRACE,"2\n");

	if( ! rule)
	{
		return (0); /* not handled */
	}
	astn.T = rule->at_topic;
	astn.N = 2;
	astn.memberships[0].R = rule->passive_topic;

	astn.memberships[1].R = rule->active_topic;
	astn.memberships[1].x = self->current_topic;

	assert(self->current_name);
	list.next = NULL;
	list.content = (char*)other;
	/*
	TM_SET_SUBJECT_N1(&sbj,SAM_NAME,"BaseNames",&list);
	*/
#if 1
	{
	char sirbuf[1024];
	struct TMList list2;
	list2.next = NULL;
	sprintf(sirbuf,"http://itil.dkrz.de/cmdb/items/%s",(char*)other);
	list2.content = sirbuf;
	TM_SET_SUBJECT_N2(&sbj,
		SAM_NAME,"BaseNames",&list,
		SAM_NAME,"SubjectIndicators",&list2);
	}
#endif
	TMTRACE(TM_PARSE_TRACE,"3\n");
	e = tm_topicmap_add_subject(self->topicmap,&sbj,&t_other);
	assert(e == TM_OK);

	astn.memberships[0].x = t_other;

	if( tm_topicmap_add_topic_assertion(self->topicmap,&astn) != TM_OK)
	{
		/*
		tm_omnivore_set_error(self->omnivore,"cannot add assertioon, %s" ,
			tm_get_error(tm_omnivore_get_tm(omnivore)));	
		*/
		assert(0);
	}

	TMTRACE(TM_PARSE_TRACE,"exit\n");
	return(1);	
}


/* FIXME sscanf is not very secure => fix! */
TMError _read_config(T self, const char *filename, Omnivore o)
{
	char buf[1024];
	char *p;
	FILE *file;

	if( (file = fopen(filename,"r")) == NULL)
	{
		tm_omnivore_set_error(o,"unable to open config file %s, %s",
			filename, strerror(errno));
		/* FIXME: portability! */
		return TM_ESYS;
	}

	while( (p = fgets(buf,sizeof(buf), file)) != NULL)
	{
		char b1[1024],b2[1024],b3[1024],b4[1024];
		if(tm_is_whitespace_string(p))
			continue;
		
		if(tm_is_comment_string(p,'#'))
			continue;
		p = (char*)tm_lstrip(p);
		if( strstr(p,"Require") == p)
		{
			TMError e;
			sscanf(p,"Require %s",b1);
			if( (e = tm_topicmap_require_model(self->topicmap,b1)) != TM_OK)
			{
				tm_omnivore_set_error(o,"unable to require %s, %s\n",
					b1, tm_get_error(tm_omnivore_get_tm(o)) );
				return e;
			}

		}
		else if( strstr(p,"Map") == p)
		{
			struct rule *rp;
			TM_NEW(rp);
			sscanf(p,"Map %s %s %s %s",b1,b2,b3,b4);
			rp->rel = tm_strdup(b1);
			rp->at = tm_strdup(b2);
			rp->active = tm_strdup(b3);
			rp->passive = tm_strdup(b4);
			rp->at_topic = 0;
			rp->active_topic = 0;
			rp->passive_topic = 0;
			/*FIXME make dynamic flag 'true' here */
			self->rules = tm_list_push(self->rules,rp);
		}
		else if( strstr(p,"Ignore") == p)
		{
			sscanf(p,"Ignore %s",b1);
			self->ignores= tm_list_push(self->ignores,tm_strdup(b1));
		}
		else
		{
			assert(!"unknown command");
		}
	}
	fclose(file);
	return TM_OK;
}
