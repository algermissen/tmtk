/*
 * $Id$
 *
 * Copyright (c) 2002 Jan Algermissen
 * See the file "COPYING" for copying permission.
 *
 */

#include <topicmaps.h>
#include "df.h"


#include <tmbasictypes.h>

/* element subtrees that we want to skip entirely: */
static struct TMList l3 = { NULL, "scope" };
static struct TMList l4 = { &l3, "variant" };

static struct TMList tel0 = { NULL, "property" };
static struct TMList tel1 = { &tel0, "resourceData" };
static struct TMList tel2 = { &tel1, "baseNameString" };

static int xtm_start_element(void *data, Omnivore ov, element *elem, const char **attr);
static int xtm_end_element(void *data, Omnivore ov, element *elem);


static const char * _get_att_val(const char **attr, const char *name)
{
        int i;
        for (i = 0; attr[i]; i += 2) {
                if( strcmp(name,attr[i]) == 0 )
                {
                        return attr[i + 1];
                }
        }
        return NULL;
}



/* data that the proc model needs; will be passed to all the callbacks */
struct data {
	TMTopicMap tm;
	TMTopic last_topic_topic;
	struct TMTopicAssertion topic_assertion;
	struct TMTopicAssertion topic_assertion_2;

	TMTable at_table;
	TMTable role_table;

	
	TMTopic at_class_instance_topic;
	TMTopic role_class_topic;
	TMTopic role_instance_topic;
	TMTopic at_basenamed_basename_topic;
	TMTopic role_basenamed_topic;
	TMTopic role_basename_topic;
	TMTopic at_occurring_occurrence_topic;
	TMTopic role_occurring_topic;
	TMTopic role_occurrence_topic;

	char last_modelname[1024]; /* FIXME */
	char last_propertyname[1024]; /* FIXME */
	
};

static TMError _init(Omnivore omnivore,void*arg,void**data)
{
	struct data *dp;
	struct TMList list;
	TM_NEW(dp);
	dp->tm = tm_omnivore_get_topicmap(omnivore);
	if(!dp->tm)
	{
		*data = dp;
		return TM_OK;
	}
	dp->last_topic_topic = 0;
	/* FIXME: the require stuff could be part of proc model structure! */
	if(tm_topicmap_require_model(dp->tm,SAM_NAME) != TM_OK)
	{
		assert(0);
	}

	dp->at_table = tm_table_new(100,tm_strcmp_v,tm_strhash_v);
	dp->role_table = tm_table_new(100,tm_strcmp_v,tm_strhash_v);

	list.next = NULL;
	


	list.content = PSI_BASE"at-class-instance";
        if( tm_topicmap_get_topic(dp->tm,SAM_NAME,"SubjectIndicators",&list,
                                &(dp->at_class_instance_topic),NULL) != TM_OK)
        {
                assert(0);
        }
	list.content = PSI_BASE"role-class";
        if( tm_topicmap_get_topic(dp->tm,SAM_NAME,"SubjectIndicators",&list,
                                &(dp->role_class_topic),NULL) != TM_OK )
        {
                assert(0);
        }
	list.content = PSI_BASE"role-instance";
        if( tm_topicmap_get_topic(dp->tm,SAM_NAME,"SubjectIndicators",&list,
                                &(dp->role_instance_topic),NULL) != TM_OK )
        {
                assert(0);
        }
	list.content = PSI_BASE"at-basenamed-basename";
        if( tm_topicmap_get_topic(dp->tm,SAM_NAME,"SubjectIndicators",&list,
                                &(dp->at_basenamed_basename_topic),NULL) != TM_OK)
        {
                assert(0);
        }
	list.content = PSI_BASE"role-basenamed";
        if( tm_topicmap_get_topic(dp->tm,SAM_NAME,"SubjectIndicators",&list,
                                &(dp->role_basenamed_topic),NULL) != TM_OK )
        {
                assert(0);
        }
	list.content = PSI_BASE"role-basename";
        if( tm_topicmap_get_topic(dp->tm,SAM_NAME,"SubjectIndicators",&list,
                                &(dp->role_basename_topic),NULL) != TM_OK )
        {
                assert(0);
        }
	list.content = PSI_BASE"at-occurring-occurrence";
        if( tm_topicmap_get_topic(dp->tm,SAM_NAME,"SubjectIndicators",&list,
                                &(dp->at_occurring_occurrence_topic),NULL) != TM_OK)
        {
                assert(0);
        }
	list.content = PSI_BASE"role-occurring";
        if( tm_topicmap_get_topic(dp->tm,SAM_NAME,"SubjectIndicators",&list,
                                &(dp->role_occurring_topic),NULL) != TM_OK )
        {
                assert(0);
        }
	list.content = PSI_BASE"role-occurrence";
        if( tm_topicmap_get_topic(dp->tm,SAM_NAME,"SubjectIndicators",&list,
                                &(dp->role_occurrence_topic),NULL) != TM_OK )
        {
                assert(0);
        }
	assert(dp->at_class_instance_topic);
	assert(dp->role_class_topic);
	assert(dp->role_instance_topic);
	assert(dp->at_basenamed_basename_topic);
	assert(dp->role_basenamed_topic);
	assert(dp->role_basename_topic);
	assert(dp->at_occurring_occurrence_topic);
	assert(dp->role_occurring_topic);
	assert(dp->role_occurrence_topic);

	*data = dp;
	return TM_OK;
}

static void _delete(void **pself)
{
	/* FIXME */
	/*
	tm_table_delete(&( (*pself)->role_table));
	*/
}



/* A processing model for a subset of XTM
 */
struct TMProcModel xtm_simple = 
{
	"xtm_simple",		/* name */
	_init,			/* init callback */
	_delete,		/* delete callback */
	"topicMap",		/* root element name (for lookup) */
	"http://www.topicmaps.org/xtm/1.0/",	/* XML namespace */
	"-//TopicMaps.Org//DTD XML Topic Map (XTM) 1.0//EN", /* DTD Public Identifier */
	"http://www.topicmaps.org/xtm/1.0/xtm1.dtd", /* URL of DTD */
	/* NULL, -- elements member of struct; unused */

	"id",	/* id_attribute_name */
	"href", /* ref_attribute_name */

	&l4,	/* subtrees to skip */
	&tel2,  /* text elements */

	&xtm_start_element,	/* Omnivore XML start callback */
	&xtm_end_element,	/* Omnivore XML end callback */
	NULL,			/* no line-event handler needed in XML proc model */
	NULL			/* no RDF Statement-Handler-Table */
};



int xtm_start_element(void *s,Omnivore ov, element *elem, const char **attr)
{
	TMError e;
	struct data *self = (struct data *)s;

	TMTRACE(TM_PROC_TRACE,"enter\n");
	if(!self->tm)
	{
		TMTRACE(TM_PROC_TRACE,"exit (no topicmap)\n");
		return (1);
	}

       if(strcmp(elem_path(elem),"/topicMap") == 0) {
		;
       } else if(strcmp(elem_path(elem),"/topicMap/topic") == 0) {
	       struct TMSubject sbj;
	       struct TMList sirs;
		sirs.next = NULL;
		sirs.content = (void*)elem_uri(elem);

		TM_SET_SUBJECT_N1(&sbj,SAM_NAME,"SourceLocators",&sirs);

		if( (e = tm_topicmap_add_subject(self->tm,&sbj,&(self->last_topic_topic) )) != TM_OK)
		{
			tm_omnivore_set_error(ov,"cannot add subject %s (element URI is %s)" ,
					tm_get_error(tm_omnivore_get_tm(ov) ) , elem_uri(elem) );
			return (0);
		}
	} else if(strcmp(elem_path(elem),"/topicMap/topic/subjectIdentity") == 0) {
	       ;
	} else if(strcmp(elem_path(elem),"/topicMap/topic/instanceOf") == 0) {
		TMTopic a_topic;
		struct TMSubject sbj;
		struct TMList sirs;
		tm_topic_assertion_init(&(self->topic_assertion));
		sirs.next = NULL;
		sirs.content = (void*)elem_uri(elem);

		TM_SET_SUBJECT_N1(&sbj,SAM_NAME,"SourceLocators",&sirs);

		if( (e = tm_topicmap_add_subject(self->tm,&sbj,&a_topic )) != TM_OK)
		{
			tm_omnivore_set_error(ov,"xtm_simple: proce error: %s" ,
					tm_get_error(tm_omnivore_get_tm(ov)) );
			return (0);
		}
		tm_topic_assertion_init(&(self->topic_assertion));
		self->topic_assertion.A = a_topic;
		self->topic_assertion.T = self->at_class_instance_topic;
		self->topic_assertion.N = 1;
		self->topic_assertion.memberships[0].R = self->role_instance_topic;
		self->topic_assertion.memberships[0].x = self->last_topic_topic;

	} else if(strcmp(elem_path(elem),"/topicMap/topic/instanceOf/topicRef") == 0) {
		TMTopic c_topic;
		struct TMSubject sbj;
                struct TMList sirs;
                sirs.next = NULL;
                sirs.content = (void*)elem_ref_uri(elem);

		TM_SET_SUBJECT_N1(&sbj,SAM_NAME,"SourceLocators",&sirs);
		/* FIXME: propably use a table since classes are likely to be used often */
		if( (e = tm_topicmap_add_subject(self->tm,&sbj,&c_topic )) != TM_OK)
		{
			tm_omnivore_set_error(ov,"xtm_simple: proce error: %s" ,
					tm_get_error(tm_omnivore_get_tm(ov)));
			return (0);
		}
		self->topic_assertion.N = 2;
		self->topic_assertion.memberships[1].R = self->role_class_topic;
		self->topic_assertion.memberships[1].x = c_topic;

		if( tm_topicmap_add_topic_assertion(self->tm,&(self->topic_assertion)) != TM_OK)
		{
			tm_omnivore_set_error(ov,"unable to add assertion to topic map, %s, element URI is %s", tm_get_error(tm_omnivore_get_tm(ov)),elem_uri(elem));
			return (0);
		}



	} else if(strcmp(elem_path(elem),"/topicMap/topic/instanceOf/subjectIndicatorRef") == 0) {
		TMTopic c_topic;
		struct TMSubject sbj;
                struct TMList sirs;
                sirs.next = NULL;
                sirs.content = (void*)elem_ref_uri(elem);

		TM_SET_SUBJECT_N1(&sbj,SAM_NAME,"SubjectIndicators",&sirs);
		/* FIXME: propably use a table since classes are likely to be used often */
		if( (e = tm_topicmap_add_subject(self->tm,&sbj,&c_topic )) != TM_OK)
		{
			tm_omnivore_set_error(ov,"xtm_simple: proce error: %s" ,
					tm_get_error(tm_omnivore_get_tm(ov)));
			return (0);
		}
		self->topic_assertion.N = 2;
		self->topic_assertion.memberships[1].R = self->role_class_topic;
		self->topic_assertion.memberships[1].x = c_topic;

		if( tm_topicmap_add_topic_assertion(self->tm,&(self->topic_assertion)) != TM_OK)
		{
			tm_omnivore_set_error(ov,"unable to add assertion to topic map, %s, element URI is %s", tm_get_error(tm_omnivore_get_tm(ov)),elem_uri(elem));
			return (0);
		}

        } else if(strcmp(elem_path(elem),"/topicMap/topic/subjectIdentity/resourceRef") == 0 ) {
		assert(elem_ref_uri(elem) && *elem_ref_uri(elem));

		if( tm_topicmap_add_property_to_topic(self->tm,self->last_topic_topic,
			SAM_NAME,"SubjectAddress",elem_ref_uri(elem)) != TM_OK)
		{
			tm_omnivore_set_error(ov,"xtm_simple: proce error: %s" ,
					tm_get_error(tm_omnivore_get_tm(ov)));
			return (1);
			assert(0);
		}

        } else if(strcmp(elem_path(elem),"/topicMap/topic/subjectIdentity/topicRef") == 0) {
		struct TMList list;
		list.next = NULL;
		list.content = (char*)elem_ref_uri(elem);
		if( tm_topicmap_add_property_to_topic(self->tm,self->last_topic_topic,
			SAM_NAME,"SourceLocators",&list) != TM_OK)
		{
			tm_omnivore_set_error(ov,"xtm_simple: proce error: %s" ,
					tm_get_error(tm_omnivore_get_tm(ov)));
			assert(0);
		}

	} else if(strcmp(elem_path(elem),"/topicMap/topic/subjectIdentity/subjectIndicatorRef")==0){
		struct TMList list;
		list.next = NULL;
		list.content = (char*)elem_ref_uri(elem);
		if( tm_topicmap_add_property_to_topic(self->tm,self->last_topic_topic,
			SAM_NAME,"SubjectIndicators",&list) != TM_OK)
		{
			tm_omnivore_set_error(ov,"xtm_simple: proce error: %s" ,
					tm_get_error(tm_omnivore_get_tm(ov)));
			assert(0);
		}

	/*
	 * ----------------------------------------------------------------
	 *
	 *                   <baseName> and subelements
	 *
	 * ----------------------------------------------------------------
	 */

 
        } else if(strcmp(elem_path(elem),"/topicMap/topic/baseName") == 0) {
		TMTopic a_topic;
		struct TMSubject sbj;
		struct TMList sirs;
		tm_topic_assertion_init(&(self->topic_assertion));
		sirs.next = NULL;
		sirs.content = (void*)elem_uri(elem);

		TM_SET_SUBJECT_N1(&sbj,SAM_NAME,"SourceLocators",&sirs);

		if( (e = tm_topicmap_add_subject(self->tm,&sbj,&a_topic )) != TM_OK)
		{
			tm_omnivore_set_error(ov,"xtm_simple: proce error: %s" ,
					tm_get_error(tm_omnivore_get_tm(ov)));
			return (0);
		}
		tm_topic_assertion_init(&(self->topic_assertion));
		self->topic_assertion.A = a_topic;
		self->topic_assertion.T = self->at_basenamed_basename_topic;
		self->topic_assertion.N = 1;
		self->topic_assertion.memberships[0].R = self->role_basenamed_topic;
		self->topic_assertion.memberships[0].x = self->last_topic_topic;

        } else if(strcmp(elem_path(elem),"/topicMap/topic/baseName/baseNameString") == 0) {
		;

	/*
	 * ----------------------------------------------------------------
	 *
	 *                   <occurrence> and subelements
	 *
	 * ----------------------------------------------------------------
	 */

 
        } else if(strcmp(elem_path(elem),"/topicMap/topic/occurrence") == 0) {
		TMTopic a_topic;
		struct TMSubject sbj;
		struct TMList sirs;
		tm_topic_assertion_init(&(self->topic_assertion));
		sirs.next = NULL;
		sirs.content = (void*)elem_uri(elem);

		TM_SET_SUBJECT_N1(&sbj,SAM_NAME,"SourceLocators",&sirs);

		if( (e = tm_topicmap_add_subject(self->tm,&sbj,&a_topic )) != TM_OK)
		{
			tm_omnivore_set_error(ov,"xtm_simple: proce error: %s" ,
					tm_get_error(tm_omnivore_get_tm(ov)));
			return (0);
		}
		tm_topic_assertion_init(&(self->topic_assertion));
		self->topic_assertion.A = a_topic;
		self->topic_assertion.T = self->at_occurring_occurrence_topic;
		self->topic_assertion.N = 1;
		self->topic_assertion.memberships[0].R = self->role_occurring_topic;
		self->topic_assertion.memberships[0].x = self->last_topic_topic;

        } else if(strcmp(elem_path(elem),"/topicMap/topic/occurrence/instanceOf") == 0) {
		TMTopic a_topic;
		struct TMSubject sbj;
		struct TMList sirs;
		sirs.next = NULL;
		sirs.content = (void*)elem_uri(elem);

		TM_SET_SUBJECT_N1(&sbj,SAM_NAME,"SourceLocators",&sirs);

		if( (e = tm_topicmap_add_subject(self->tm,&sbj,&a_topic )) != TM_OK)
		{
			tm_omnivore_set_error(ov,"xtm_simple: proce error: %s" ,
					tm_get_error(tm_omnivore_get_tm(ov)));
			return (0);
		}
		tm_topic_assertion_init(&(self->topic_assertion_2));
		self->topic_assertion_2.A = a_topic;
		self->topic_assertion_2.T = self->at_class_instance_topic;
		self->topic_assertion_2.N = 1;
		self->topic_assertion_2.memberships[0].R = self->role_instance_topic;
			/* <occurrence> a-topic is stored in A of 'topic_assertion' */
		self->topic_assertion_2.memberships[0].x = self->topic_assertion.A;

        } else if(strcmp(elem_path(elem),"/topicMap/topic/occurrence/instanceOf/topicRef")==0){
		TMTopic c_topic;
		struct TMSubject sbj;
                struct TMList sirs;
                sirs.next = NULL;
                sirs.content = (void*)elem_ref_uri(elem);

		TM_SET_SUBJECT_N1(&sbj,SAM_NAME,"SourceLocators",&sirs);
		/* FIXME: propably use a table since classes are likely to be used often */
		if( (e = tm_topicmap_add_subject(self->tm,&sbj,&c_topic )) != TM_OK)
		{
			tm_omnivore_set_error(ov,"xtm_simple: proce error: %s" ,
					tm_get_error(tm_omnivore_get_tm(ov)));
			return (0);
		}
		self->topic_assertion_2.N = 2;
		self->topic_assertion_2.memberships[1].R = self->role_class_topic;
		self->topic_assertion_2.memberships[1].x = c_topic;

		if( tm_topicmap_add_topic_assertion(self->tm,&(self->topic_assertion_2)) != TM_OK)
		{
			tm_omnivore_set_error(ov,"unable to add assertion to topic map, %s, element URI is %s", tm_get_error(tm_omnivore_get_tm(ov)),elem_uri(elem));
			return (0);
		}

	} else if(strcmp(elem_path(elem),"/topicMap/topic/occurrence/instanceOf/subjectIndicatorRef") == 0) {
		TMTopic c_topic;
		struct TMSubject sbj;
                struct TMList sirs;
                sirs.next = NULL;
                sirs.content = (void*)elem_ref_uri(elem);

		TM_SET_SUBJECT_N1(&sbj,SAM_NAME,"SubjectIndicators",&sirs);
		/* FIXME: propably use a table since classes are likely to be used often */
		if( (e = tm_topicmap_add_subject(self->tm,&sbj,&c_topic )) != TM_OK)
		{
			tm_omnivore_set_error(ov,"xtm_simple: proce error: %s" ,
					tm_get_error(tm_omnivore_get_tm(ov)));
			return (0);
		}
		self->topic_assertion_2.N = 2;
		self->topic_assertion_2.memberships[1].R = self->role_class_topic;
		self->topic_assertion_2.memberships[1].x = c_topic;

		if( tm_topicmap_add_topic_assertion(self->tm,&(self->topic_assertion_2)) != TM_OK)
		{
			tm_omnivore_set_error(ov,"unable to add assertion to topic map, %s, element URI is %s", tm_get_error(tm_omnivore_get_tm(ov)),elem_uri(elem));
			return (0);
		}

        } else if(strcmp(elem_path(elem),"/topicMap/topic/occurrence/resourceData") == 0) {
		; /* handled in end element */
        } else if(strcmp(elem_path(elem),"/topicMap/topic/occurrence/resourceRef") == 0) {
		TMTopic x_topic;
		struct TMSubject sbj;
		int N;
		assert(elem_ref_uri(elem) && *elem_ref_uri(elem));
		N = self->topic_assertion.N; /* for shorter code lines below */

		TM_SET_SUBJECT_N1(&sbj,SAM_NAME,"SubjectAddress",elem_ref_uri(elem));

		if( (e = tm_topicmap_add_subject(self->tm,&sbj,&x_topic )) != TM_OK)
		{
			tm_omnivore_set_error(ov,"xtm_simple: proce error: %s" ,
					tm_get_error(tm_omnivore_get_tm(ov)));
			return (0);
		}
		assert(x_topic);
		self->topic_assertion.memberships[N].R = self->role_occurrence_topic;
		self->topic_assertion.memberships[N].x = x_topic;
		self->topic_assertion.N = 2;

	/*
	 * ----------------------------------------------------------------
	 *
	 *                   <association> and subelements
	 *
	 * ----------------------------------------------------------------
	 */


        } else if(strcmp(elem_path(elem),"/topicMap/association") == 0) {
		TMTopic a_topic;
		struct TMSubject sbj;
		struct TMList sirs;
		sirs.next = NULL;
		sirs.content = (void*)elem_uri(elem);

		TM_SET_SUBJECT_N1(&sbj,SAM_NAME,"SourceLocators",&sirs);

		if( (e = tm_topicmap_add_subject(self->tm,&sbj,&a_topic )) != TM_OK)
		{
			tm_omnivore_set_error(ov,"xtm_simple: proce error: %s" ,
					tm_get_error(tm_omnivore_get_tm(ov)));
			return (0);
		}
		tm_topic_assertion_init(&(self->topic_assertion));
		self->topic_assertion.A = a_topic;
        } else if(strcmp(elem_path(elem),"/topicMap/association/instanceOf") == 0) {
		;

        } else if(strcmp(elem_path(elem),"/topicMap/association/instanceOf/topicRef") == 0){
		TMTopic t_topic;
		struct TMSubject sbj;
		struct TMList sirs;
		assert(elem_ref_uri(elem) && *elem_ref_uri(elem));

		t_topic = (TMTopic)tm_table_get(self->at_table,elem_ref_uri(elem));

		if( ! t_topic)
		{
		sirs.next = NULL;
		sirs.content = (void*)elem_ref_uri(elem);

		TM_SET_SUBJECT_N1(&sbj,SAM_NAME,"SourceLocators",&sirs);

		if( (e = tm_topicmap_add_subject(self->tm,&sbj,&t_topic )) != TM_OK)
		{
			tm_omnivore_set_error(ov,"xtm_simple: proce error: %s" ,
					tm_get_error(tm_omnivore_get_tm(ov)));
			return (0);
		}
		assert(t_topic);


		tm_table_put(self->at_table,tm_strdup(elem_ref_uri(elem)),(void*)t_topic);
		}
		self->topic_assertion.T = t_topic;


	} else if(strcmp(elem_path(elem),"/topicMap/association/instanceOf/subjectIndicatorRef")==0){
		TMTopic t_topic;
		struct TMSubject sbj;
		struct TMList sirs;
		assert(elem_ref_uri(elem) && *elem_ref_uri(elem));

		t_topic = (TMTopic)tm_table_get(self->at_table,elem_ref_uri(elem));

		if( ! t_topic)
		{
		sirs.next = NULL;
		sirs.content = (void*)elem_ref_uri(elem);

		TM_SET_SUBJECT_N1(&sbj,SAM_NAME,"SubjectIndicators",&sirs);

		if( (e = tm_topicmap_add_subject(self->tm,&sbj,&t_topic )) != TM_OK)
		{
			tm_omnivore_set_error(ov,"xtm_simple: proce error: %s" ,
					tm_get_error(tm_omnivore_get_tm(ov)));
			return (0);
		}
		assert(t_topic);


		tm_table_put(self->at_table,tm_strdup(elem_ref_uri(elem)),(void*)t_topic);
		}
		self->topic_assertion.T = t_topic;

        } else if(strcmp(elem_path(elem),"/topicMap/association/member") == 0) {
		int N = self->topic_assertion.N;
		self->topic_assertion.memberships[N].R = 0;
		self->topic_assertion.memberships[N].x = 0;

        } else if(strcmp(elem_path(elem),"/topicMap/association/member/roleSpec") == 0) {
		;

        } else if(strcmp(elem_path(elem),"/topicMap/association/member/roleSpec/topicRef")==0){
		TMTopic r_topic;
		struct TMSubject sbj;
		struct TMList sirs;
		int N;
		assert(elem_ref_uri(elem) && *elem_ref_uri(elem));
		N = self->topic_assertion.N; /* for shorter code lines below */

		r_topic = (TMTopic)tm_table_get(self->role_table,elem_ref_uri(elem));

		if( ! r_topic)
		{



		sirs.next = NULL;
		sirs.content = (void*)elem_ref_uri(elem);

		TM_SET_SUBJECT_N1(&sbj,SAM_NAME,"SourceLocators",&sirs);

		if( (e = tm_topicmap_add_subject(self->tm,&sbj,&r_topic )) != TM_OK)
		{
			tm_omnivore_set_error(ov,"xtm_simple: proce error: %s" ,
					tm_get_error(tm_omnivore_get_tm(ov)));
			return (0);
		}
		assert(r_topic);
		tm_table_put(self->role_table,tm_strdup(elem_ref_uri(elem)),(void*)r_topic);
		}

		assert(self->topic_assertion.memberships[N].R == 0);
		assert(self->topic_assertion.memberships[N].x == 0);
#if 0
FIXME: the following was debug code for preventiong multiple rol eplayers.
		{
			int i;
			for(i=0;i<N;i++)
			{
				if(self->topic_assertion.memberships[i].R == r_topic)
				{
					tm_omnivore_set_error(ov,"multiple role players not supported (1), element uri is '%s'", elem_uri(elem));
					return (0);

				}
			}

		}
#endif
		self->topic_assertion.memberships[N].R = r_topic;


	} else if(strcmp(elem_path(elem),"/topicMap/association/member/roleSpec/subjectIndicatorRef") == 0) {
		TMTopic r_topic;
		struct TMSubject sbj;
		struct TMList sirs;
		int N;
		assert(elem_ref_uri(elem) && *elem_ref_uri(elem));
		N = self->topic_assertion.N; /* for shorter code lines below */

		r_topic = (TMTopic)tm_table_get(self->role_table,elem_ref_uri(elem));

		if( ! r_topic)
		{



		sirs.next = NULL;
		sirs.content = (void*)elem_ref_uri(elem);

		TM_SET_SUBJECT_N1(&sbj,SAM_NAME,"SubjectIndicators",&sirs);

		if( (e = tm_topicmap_add_subject(self->tm,&sbj,&r_topic )) != TM_OK)
		{
			tm_omnivore_set_error(ov,"xtm_simple: proce error: %s" ,
					tm_get_error(tm_omnivore_get_tm(ov)));
			return (0);
		}
		assert(r_topic);
		tm_table_put(self->role_table,tm_strdup(elem_ref_uri(elem)),(void*)r_topic);
		}

		assert(self->topic_assertion.memberships[N].R == 0);
		assert(self->topic_assertion.memberships[N].x == 0);
#if 0
FIXME (see same debug code above)
		{
			int i;
			for(i=0;i<N;i++)
			{
				if(self->topic_assertion.memberships[i].R == r_topic)
				{
					tm_omnivore_set_error(ov,"multiple role players not supported (2), element uri is '%s'", elem_uri(elem));
					return (0);

				}
			}

		}
#endif
		self->topic_assertion.memberships[N].R = r_topic;

        } else if(strcmp(elem_path(elem),"/topicMap/association/member/topicRef") == 0){

		TMTopic x_topic;
		struct TMSubject sbj;
		struct TMList sirs;
		int N;
		assert(elem_ref_uri(elem) && *elem_ref_uri(elem));
		N = self->topic_assertion.N; /* for shorter code lines below */
		sirs.next = NULL;
		sirs.content = (void*)elem_ref_uri(elem);

		TM_SET_SUBJECT_N1(&sbj,SAM_NAME,"SourceLocators",&sirs);

		if( (e = tm_topicmap_add_subject(self->tm,&sbj,&x_topic )) != TM_OK)
		{
			tm_omnivore_set_error(ov,"xtm_simple: proce error: %s" ,
					tm_get_error(tm_omnivore_get_tm(ov)));
			return (0);
		}
		if(self->topic_assertion.memberships[N].R == 0)
		{
			tm_omnivore_set_error(ov,"players without a role not supported, element uri is '%s'", elem_uri(elem));
			return (0);
		}
		/* if there has already been a player inside this <member> element... */
		if(self->topic_assertion.memberships[N].x != 0)
		{
			self->topic_assertion.N++;
			N++;
			self->topic_assertion.memberships[N].R = self->topic_assertion.memberships[N-1].R;

		}
		self->topic_assertion.memberships[N].x = x_topic;



	} else if(strcmp(elem_path(elem),"/topicMap/association/member/subjectIndicatorRef") == 0 ){

		TMTopic x_topic;
		struct TMSubject sbj;
		struct TMList sirs;
		int N;
		assert(elem_ref_uri(elem) && *elem_ref_uri(elem));
		N = self->topic_assertion.N; /* for shorter code lines below */
		sirs.next = NULL;
		sirs.content = (void*)elem_ref_uri(elem);

		TM_SET_SUBJECT_N1(&sbj,SAM_NAME,"SubjectIndicators",&sirs);

		if( (e = tm_topicmap_add_subject(self->tm,&sbj,&x_topic )) != TM_OK)
		{
			tm_omnivore_set_error(ov,"xtm_simple: proce error: %s" ,
					tm_get_error(tm_omnivore_get_tm(ov)));
			return (0);
		}
		if(self->topic_assertion.memberships[N].R == 0)
		{
			tm_omnivore_set_error(ov,"players without a role not supported, element uri is '%s'", elem_uri(elem));
			return (0);
		}

		/* if there has already been a player inside this <member> element... */
		if(self->topic_assertion.memberships[N].x != 0)
		{
			self->topic_assertion.N++;
			N++;
			self->topic_assertion.memberships[N].R = self->topic_assertion.memberships[N-1].R;

		}

		self->topic_assertion.memberships[N].x = x_topic;


        } else if(strcmp(elem_path(elem),"/topicMap/association/member/resourceRef") == 0) {

		TMTopic x_topic;
		struct TMSubject sbj;
		int N;
		assert(elem_ref_uri(elem) && *elem_ref_uri(elem));
		N = self->topic_assertion.N; /* for shorter code lines below */

		TM_SET_SUBJECT_N1(&sbj,SAM_NAME,"SubjectAddress",elem_ref_uri(elem));

		if( (e = tm_topicmap_add_subject(self->tm,&sbj,&x_topic )) != TM_OK)
		{
			tm_omnivore_set_error(ov,"xtm_simple: proce error: %s" ,
					tm_get_error(tm_omnivore_get_tm(ov)));
			return (0);
		}
		assert(self->topic_assertion.memberships[N].R != 0);
		assert(self->topic_assertion.memberships[N].x == 0);
		self->topic_assertion.memberships[N].x = x_topic;

        } else if(strcmp(elem_path(elem),"/topicMap/topic/property") == 0) {
		const char *modelname, *propertyname;

		modelname = _get_att_val(attr,"disclosure");
		if(!modelname)
		{
			tm_omnivore_set_error(ov,"syntax error: missing attribute 'disclosure'");
			return (0);
		}

		TMTRACE(TM_PROC_TRACE,"modelname=%s\n" _ modelname);
		propertyname = _get_att_val(attr,"name");
		if(!propertyname)
		{
			tm_omnivore_set_error(ov,"syntax error: missing attribute 'name'");
			return (0);
		}
		TMTRACE(TM_PROC_TRACE,"propertyname=%s\n" _ propertyname);
		assert(strlen(modelname) < sizeof(self->last_modelname));
		strcpy(self->last_modelname,modelname);
		assert(strlen(propertyname) < sizeof(self->last_propertyname));
		strcpy(self->last_propertyname,propertyname);

		if( tm_topicmap_require_model(self->tm,modelname) != TM_OK)
		{
			tm_omnivore_set_error(ov,"xtm_simple: error: %s" ,
					tm_get_error(tm_omnivore_get_tm(ov)));
			return (0);
		}


        } else {
		tm_omnivore_set_error(ov,"syntax error: unsupported path %s", elem_uri(elem));
		return (0);
	}
 
        return 1;
}

int xtm_end_element(void *s,Omnivore ov, element *elem)
{
	TMError e;
	struct data *self = (struct data *)s;

	TMTRACE(TM_PROC_TRACE,"enter\n");
	if(!self->tm)
	{
		TMTRACE(TM_PROC_TRACE,"exit (no topicmap)\n");
		return (1);
	}

	if(!self->tm)
		return (1);

	if(strcmp(elem_path(elem),"/topicMap") == 0) {
		;


        } else if(strcmp(elem_path(elem),"/topicMap/association/member") == 0) {
		int N = self->topic_assertion.N;
		if(self->topic_assertion.memberships[N].R == 0)
		{
			tm_omnivore_set_error(ov,"member without role not supported, element uri is '%s'", elem_uri(elem));
			return (0);
		}
		if(self->topic_assertion.memberships[N].x == 0)
		{
			tm_omnivore_set_error(ov,"member without role-player not supported, element uri is '%s'", elem_uri(elem));
			return (0);
		}
		self->topic_assertion.N++;

        } else if(strcmp(elem_path(elem),"/topicMap/association") == 0) {
		int i;
		TMTopicAssertion ap;
		/* for easier codereading */
		ap = &(self->topic_assertion);

		if(ap->T == 0)
		{
			tm_omnivore_set_error(ov,"untyped association not supported, element uri is '%s'", elem_uri(elem));
			return (0);
		}

		/* FIXME error here or wait for topic map to detect it?? 
		*/
		assert(ap->N >= 2);

		/* FIXME: if we allow multiple players to be distributed over several
		 * <member> elements, we need to sort assertion first for the following code
		 * to work
		 */
		tm_topic_assertion_sort(ap);
		for(i=0;i< ap->N - 1; i++)
		{
			if(ap->memberships[i].R == ap->memberships[i+1].R)
			{
				/*
				void *v;
				*/
				struct TMSubject sbj;
				TMTopic set_topic;
				TMTopicSet set;
				tm_log(tm_omnivore_get_tm(ov),TM_LOG_INFO, "multiple role players found");
				set = tm_topicset_new(10);
				tm_topicset_add(set,ap->memberships[i].x);
				while(i< ap->N-1 && (ap->memberships[i].R
					   	     == ap->memberships[i+1].R))
				{
					int j;
					tm_topicset_add(set,ap->memberships[i+1].x);
					tm_log(tm_omnivore_get_tm(ov),TM_LOG_INFO, "   --*");
					for(j = i+1; j< ap->N-1; j++)
					{
						ap->memberships[j].R = ap->memberships[j+1].R;
						ap->memberships[j].x = ap->memberships[j+1].x;
					}
					ap->N--;
				}
				/*
				v = tm_value_new(&TopicSet);
				tm_value_call(&TopicSet, v , "addMember" ,
						self->topic_assertion.memberships[N].x);
				*/

				TM_SET_SUBJECT_N1(&sbj,SAM_NAME,"SetMembers",set);
 
                		if( (e = tm_topicmap_add_subject(self->tm,&sbj,&set_topic )) != TM_OK)
				{
					tm_omnivore_set_error(ov,"unable to add assertion to topic map, %s, element URI is %s", tm_get_error(tm_omnivore_get_tm(ov)),elem_uri(elem));
					return (0);
				}
				assert(set_topic);
				ap->memberships[i].x = set_topic;
			}
		}

		if( tm_topicmap_add_topic_assertion(self->tm,&(self->topic_assertion)) != TM_OK)
		{
			tm_omnivore_set_error(ov,"unable to add assertion to topic map, %s, element URI is %s", tm_get_error(tm_omnivore_get_tm(ov)),elem_uri(elem));
			return (0);
		}

	/*
	 * ----------------------------------------------------------------
	 *
	 *                   <baseName> and subelements
	 *
	 * ----------------------------------------------------------------
	 */

        } else if(strcmp(elem_path(elem),"/topicMap/topic/baseName/baseNameString") == 0) {
		TMTopic b_topic;
		struct TMSubject sbj;
                struct TMList sirs;
                sirs.next = NULL;
                sirs.content = (void*)elem_uri(elem);

		TM_SET_SUBJECT_N2(&sbj,SAM_NAME,"SourceLocators",&sirs,SAM_NAME,"IndicatorData",elem_text(elem));

		if( (e = tm_topicmap_add_subject(self->tm,&sbj,&b_topic )) != TM_OK)
		{
			tm_omnivore_set_error(ov,"xtm_simple: proce error: %s" ,
					tm_get_error(tm_omnivore_get_tm(ov)));
			return (0);
		}
		self->topic_assertion.N = 2;
		self->topic_assertion.memberships[1].R = self->role_basename_topic;
		self->topic_assertion.memberships[1].x = b_topic;


		{
		struct TMList list;
		list.next = NULL;
		list.content = (char*)elem_text(elem);
		if( tm_topicmap_add_property_to_topic(self->tm,self->last_topic_topic,
			SAM_NAME,"BaseNames",&list) != TM_OK)
		{
			tm_omnivore_set_error(ov,"xtm_simple: proce error: %s" ,
					tm_get_error(tm_omnivore_get_tm(ov)));
			assert(0);
		}
		}

        } else if(strcmp(elem_path(elem),"/topicMap/topic/baseName") == 0) {
		if( tm_topicmap_add_topic_assertion(self->tm,&(self->topic_assertion)) != TM_OK)
		{
			tm_omnivore_set_error(ov,"unable to add assertion to topic map, %s, element URI is %s", tm_get_error(tm_omnivore_get_tm(ov)),elem_uri(elem));
			return (0);
		}

	/*
	 * ----------------------------------------------------------------
	 *
	 *                   <occurrence> and subelements
	 *
	 * ----------------------------------------------------------------
	 */

        } else if(strcmp(elem_path(elem),"/topicMap/topic/occurrence/resourceData") == 0) {
		TMTopic r_topic;
		struct TMSubject sbj;

		TM_SET_SUBJECT_N2(&sbj,SAM_NAME,"SubjectAddress",elem_uri(elem),SAM_NAME,"SubjectData",elem_text(elem));

		if( (e = tm_topicmap_add_subject(self->tm,&sbj,&r_topic )) != TM_OK)
		{
			tm_omnivore_set_error(ov,"xtm_simple: proce error: %s" ,
					tm_get_error(tm_omnivore_get_tm(ov)));
			return (0);
		}
		self->topic_assertion.N = 2;
		self->topic_assertion.memberships[1].R = self->role_occurrence_topic;
		self->topic_assertion.memberships[1].x = r_topic;

        } else if(strcmp(elem_path(elem),"/topicMap/topic/occurrence") == 0) {
		if( tm_topicmap_add_topic_assertion(self->tm,&(self->topic_assertion)) != TM_OK)
		{
			tm_omnivore_set_error(ov,"unable to add assertion to topic map, %s, element URI is %s", tm_get_error(tm_omnivore_get_tm(ov)),elem_uri(elem));
			return (0);
		}

        } else if(strcmp(elem_path(elem),"/topicMap/mergeMap") == 0) {
		;

		/* FIXME: added themes !*/
		/*
		if(ov->merge_map_handler)
			ov->merge_map_handler(ov->user_data, elem_ref_uri(elem));
		*/
        } else if(strcmp(elem_path(elem),"/topicMap/topic/property") == 0) {
		if( tm_topicmap_add_property_to_topic_from_string(self->tm,self->last_topic_topic,
			self->last_modelname,self->last_propertyname,elem_text(elem)) != TM_OK)
		{
			tm_omnivore_set_error(ov,"xtm_simple: proce error: %s" ,
					tm_get_error(tm_omnivore_get_tm(ov)));
			return(0);
		}


        } else {
		/* no action for other paths. Syntax errors have been handled by
		 * start element.
		 */
	}
 
        return 1;
}

