/*
 * $Id$
 *
 * Copyright (c) 2002 Jan Algermissen
 * See the file "COPYING" for copying permission.
 *
 */

#include <topicmaps.h>
#include "df.h"



#define SAM_NAME "http://www.gooseworks.org/disclosures/SAM.xml"
#define DC_NAME "http://www.gooseworks.org/disclosures/DC.xml"

/* data that the proc model needs; will be passed to all the callbacks */
struct data {
	TMTopicMap tm;
	struct TMTopicAssertion topic_assertion;
	struct TMTopicAssertion topic_assertion_2;

	TMTable r_table;

	
	TMTopic at_class_instance_topic;
	TMTopic role_class_topic;
	TMTopic role_instance_topic;
	TMTopic at_basenamed_basename_topic;
	TMTopic role_basenamed_topic;
	TMTopic role_basename_topic;
	TMTopic at_occurring_occurrence_topic;
	TMTopic role_occurring_topic;
	TMTopic role_occurrence_topic;
};

static TMError _init(Omnivore omnivore,void*arg, void **data)
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
	if(tm_topicmap_require_model(dp->tm,DC_NAME) != TM_OK)
	{
		return TM_FAIL;
	}
	dp->r_table = tm_table_new(100,tm_strcmp_v,tm_strhash_v);

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

static TMError _dc_description(void *s,Omnivore om,TMRDFStatement stmt);
static TMError _dc_title(void *s,Omnivore om,TMRDFStatement stmt);
static TMError _dc_subject(void *s,Omnivore om,TMRDFStatement stmt);
static TMError _dc_date(void *s,Omnivore om,TMRDFStatement stmt);

static struct rdf_tab _rdf_tab[] = {
	{"http://www.purl.org/metadata/dublin_core#Title",	_dc_title },
	{"http://purl.org/rss/1.0/title",			_dc_title },
	{"http://purl.org/dc/elements/1.1/Title",		_dc_title },
	{"http://purl.org/dc/elements/1.1/title",		_dc_title },

	{"http://www.purl.org/metadata/dublin_core#Description",_dc_description },
	{"http://purl.org/rss/1.0/description",			_dc_description },
	{"http://purl.org/dc/elements/1.1/Description", 	_dc_description },
	{"http://purl.org/dc/elements/1.1/description", 	_dc_description },

	{"http://purl.org/dc/elements/1.1/subject",		_dc_subject },
	{"http://purl.org/dc/elements/1.1/Subject",		_dc_subject },

	{"http://purl.org/dc/elements/1.1/date",		_dc_date },
	{"http://purl.org/dc/elements/1.1/Date",		_dc_date },


	{ NULL , NULL}
};



/* A processing model for a subset of XTM
 * FIXME: remove XTM stuff from struct
 */
struct TMProcModel rdf_common = 
{
	"rdf_common",		/* name */
	_init,			/* init callback */
	_delete,		/* delete callback */
	NULL,		/* root element name (for lookup) */
	NULL,	/* XML namespace */
	NULL, /* DTD Public Identifier */
	NULL, /* URL of DTD */
	/* NULL, -- elements member of struct; unused */

	NULL,	/* id_attribute_name */
	NULL, /* ref_attribute_name */

	NULL,	/* subtrees to skip */
	NULL,  /* text elements */


	NULL,			/* Omnivore XML start callback */
	NULL,			/* Omnivore XML end callback */
	NULL,			/* no line-event handler needed in XML proc model */
	_rdf_tab
};



TMError _dc_title(void *s,Omnivore om,TMRDFStatement stmt)
{
	TMError e;
	TMTopic r_topic;
	TMTopic b_topic;
	struct TMTopicAssertion a;
	struct TMSubject sbj;
	struct data *self = (struct data *)s;

	if(!self->tm)
		return (1);
	tm_topic_assertion_init( &a );
	r_topic = (TMTopic)tm_table_get(self->r_table,stmt->subject);

	if(! r_topic)
	{
		TM_SET_SUBJECT_N1(&sbj,SAM_NAME,"SubjectAddress",stmt->subject);

		if( (e = tm_topicmap_add_subject(self->tm,&sbj,&r_topic )) != TM_OK)
		{
			assert(0);
			/*
			tm_omnivore_set_error(ov,"cannot add subject %s (element URI is %s)" ,
				tm_topicmap_get_error(self->tm) , elem_uri(elem) );
			*/
			return (0);
		}
		tm_table_put(self->r_table,tm_strdup(stmt->subject),(void*)r_topic);
	}
	assert(r_topic);

	 {
                struct TMList list;
                list.next = NULL;
                list.content = (char*)stmt->object;
                if( tm_topicmap_add_property_to_topic(self->tm,r_topic,
                        SAM_NAME,"BaseNames",&list) != TM_OK)
                {
                        assert(0);
                }
                }

	a.T = self->at_basenamed_basename_topic;
	a.A = 0;

	TM_SET_SUBJECT_N1(&sbj,SAM_NAME,"IndicatorData",stmt->object);
	if( (e = tm_topicmap_add_subject(self->tm,&sbj,&b_topic )) != TM_OK)
	{
		assert(0);
		/*
		tm_omnivore_set_error(ov,"cannot add subject %s (element URI is %s)" ,
			tm_topicmap_get_error(self->tm) , elem_uri(elem) );
		*/
		return (0);
	}
	assert(b_topic);
	a.memberships[0].R = self->role_basenamed_topic;
	a.memberships[0].x = r_topic; 
	a.memberships[1].R = self->role_basename_topic;
	a.memberships[1].x = b_topic; 
	a.N=2;
	if( tm_topicmap_add_topic_assertion(self->tm,&a ) != TM_OK)
	{
		assert(0);
		/*
		tm_omnivore_set_error(ov,"unable to add assertion to topic map, %s, element URI is %s", tm_topicmap_get_error(self->tm),elem_uri(elem));
		return (0);
		*/
	}

	return TM_OK;
}
TMError _dc_description(void *s,Omnivore om,TMRDFStatement stmt)
{
	TMError e;
	TMTopic r_topic;
	TMTopic o_topic;
	struct TMTopicAssertion a;
	struct TMSubject sbj;
	struct data *self = (struct data *)s;

	if(!self->tm)
		return (1);
	tm_topic_assertion_init( &a );
	r_topic = (TMTopic)tm_table_get(self->r_table,stmt->subject);

	if(! r_topic)
	{
		TM_SET_SUBJECT_N1(&sbj,SAM_NAME,"SubjectAddress",stmt->subject);

		if( (e = tm_topicmap_add_subject(self->tm,&sbj,&r_topic )) != TM_OK)
		{
			assert(0);
			/*
			tm_omnivore_set_error(ov,"cannot add subject %s (element URI is %s)" ,
				tm_topicmap_get_error(self->tm) , elem_uri(elem) );
			*/
			return (0);
		}
		tm_table_put(self->r_table,tm_strdup(stmt->subject),(void*)r_topic);
	}
	assert(r_topic);

	a.T = self->at_occurring_occurrence_topic;
	a.A = 0;

	TM_SET_SUBJECT_N1(&sbj,SAM_NAME,"SubjectData",stmt->object);
	if( (e = tm_topicmap_add_subject(self->tm,&sbj,&o_topic )) != TM_OK)
	{
		assert(0);
		/*
		tm_omnivore_set_error(ov,"cannot add subject %s (element URI is %s)" ,
			tm_topicmap_get_error(self->tm) , elem_uri(elem) );
		*/
		return (0);
	}
	assert(o_topic);
	a.memberships[0].R = self->role_occurring_topic;
	a.memberships[0].x = r_topic; 
	a.memberships[1].R = self->role_occurrence_topic;
	a.memberships[1].x = o_topic; 
	a.N=2;
	if( tm_topicmap_add_topic_assertion(self->tm,&a ) != TM_OK)
	{
		assert(0);
		/*
		tm_omnivore_set_error(ov,"unable to add assertion to topic map, %s, element URI is %s", tm_topicmap_get_error(self->tm),elem_uri(elem));
		return (0);
		*/
	}

	return TM_OK;
}
	
TMError _dc_subject(void *s,Omnivore om,TMRDFStatement stmt)
{
	TMError e;
	TMTopic r_topic;
	TMTopic b_topic;
	TMTopic t_topic;
	struct TMTopicAssertion a;
	struct TMSubject sbj;
	struct data *self = (struct data *)s;

	if(!self->tm)
		return (1);
	tm_topic_assertion_init( &a );
	r_topic = (TMTopic)tm_table_get(self->r_table,stmt->subject);

	if(! r_topic)
	{
		TM_SET_SUBJECT_N1(&sbj,SAM_NAME,"SubjectAddress",stmt->subject);

		if( (e = tm_topicmap_add_subject(self->tm,&sbj,&r_topic )) != TM_OK)
		{
			assert(0);
			/*
			tm_omnivore_set_error(ov,"cannot add subject %s (element URI is %s)" ,
				tm_topicmap_get_error(self->tm) , elem_uri(elem) );
			*/
			return (0);
		}
		tm_table_put(self->r_table,tm_strdup(stmt->subject),(void*)r_topic);
	}
	assert(r_topic);

	TM_SET_SUBJECT_N1(&sbj,SAM_NAME,"IndicatorData",stmt->object);
	if( (e = tm_topicmap_add_subject(self->tm,&sbj,&b_topic )) != TM_OK)
	{
		assert(0);
		/*
		tm_omnivore_set_error(ov,"cannot add subject %s (element URI is %s)" ,
			tm_topicmap_get_error(self->tm) , elem_uri(elem) );
		*/
		return (0);
	}
	assert(b_topic);

{
		struct TMSubject sbj2;
                struct TMList list;
                list.next = NULL;
                list.content = (char*)stmt->object;
	TM_SET_SUBJECT_N1(&sbj2,SAM_NAME,"BaseNames",&list);

	/* FIXME later we could just build in the SAM::Names property or so...*/
	if( (e = tm_topicmap_add_subject(self->tm,&sbj2,&t_topic )) != TM_OK)
	{
		assert(0);
		/*
		tm_omnivore_set_error(ov,"cannot add subject %s (element URI is %s)" ,
			tm_topicmap_get_error(self->tm) , elem_uri(elem) );
		*/
		return (0);
	}
 }


	a.T = self->at_basenamed_basename_topic;
	a.A = 0;

	a.memberships[0].R = self->role_basenamed_topic;
	a.memberships[0].x = t_topic; 
	a.memberships[1].R = self->role_basename_topic;
	a.memberships[1].x = b_topic; 
	a.N=2;
	if( tm_topicmap_add_topic_assertion(self->tm,&a ) != TM_OK)
	{
		assert(0);
		/*
		tm_omnivore_set_error(ov,"unable to add assertion to topic map, %s, element URI is %s", tm_topicmap_get_error(self->tm),elem_uri(elem));
		return (0);
		*/
	}


	tm_topic_assertion_init( &a );

	a.T = self->at_occurring_occurrence_topic;
	a.A = 0;

	a.memberships[0].R = self->role_occurring_topic;
	a.memberships[0].x = t_topic; 
	a.memberships[1].R = self->role_occurrence_topic;
	a.memberships[1].x = r_topic; 
	a.N=2;
	if( tm_topicmap_add_topic_assertion(self->tm,&a ) != TM_OK)
	{
		assert(0);
		/*
		tm_omnivore_set_error(ov,"unable to add assertion to topic map, %s, element URI is %s", tm_topicmap_get_error(self->tm),elem_uri(elem));
		return (0);
		*/
	}

	return TM_OK;
}
	
TMError _dc_date(void *s,Omnivore om,TMRDFStatement stmt)
{
	TMError e;
	TMTopic r_topic = 0;
	struct data *self = (struct data *)s;
	if(!self->tm)
		return TM_OK;
	r_topic = (TMTopic)tm_table_get(self->r_table,stmt->subject);

	if(! r_topic)
	{
		struct TMSubject sbj;
		TM_SET_SUBJECT_N2(&sbj,SAM_NAME,"SubjectAddress",stmt->subject,DC_NAME,"Date",stmt->object);
		if( (e = tm_topicmap_add_subject(self->tm,&sbj,&r_topic )) != TM_OK)
		{
			tm_omnivore_set_error(om,"cannot add subject, %s" ,
				tm_get_error(tm_topicmap_get_tm(self->tm)) );
			return (TM_E_PARSE);
		}
		tm_table_put(self->r_table,tm_strdup(stmt->subject),(void*)r_topic);
		return TM_OK;
	}

	if( (e = tm_topicmap_add_property_to_topic(self->tm,r_topic,DC_NAME,"Date",stmt->object )) != TM_OK)
	{
		tm_omnivore_set_error(om,"cannot add property, %s" ,
			tm_get_error(tm_topicmap_get_tm(self->tm)) );
		return (TM_E_PARSE);
	}

	return TM_OK;
}
	
