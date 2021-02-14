/*
 * $Id$
 *
 * Copyright (c) 2002 Jan Algermissen
 * See the file "COPYING" for copying permission.
 *
 */

#include <topicmaps.h>


#include <tmdevel.h>		/* for development macros */

extern struct TMValueType Topic;
extern struct TMValueType Text;

/**
<!ELEMENT topic (occurrence*)>
<!ATTLIST topic
	topic		Topic
	address		Text
	indicators	TextSet
	basenames	TextSet
>

<!ELEMENT occurrence EMPTY>
<!ATTLIST occurrence
	data		Text
	address		Text
	title		Text
>

*/


#include "df.h"

TMError tm_view_hitlist(struct view *vp, TMParams params)
{
	TMError e;

	TMTopic at_basenamed_basename_topic;
        TMTopic role_basenamed_topic;
        TMTopic role_basename_topic;
	TMTopic at_occurring_occurrence_topic;
        TMTopic role_occurring_topic;
        TMTopic role_occurrence_topic;
	TMProperty prop_sbjadr;
	TMProperty prop_sbjind;
	TMProperty prop_sbjdata;
	TMProperty prop_basenames;

	char pairbuf[256];  /* 256 is surely enough for "(%d,%d)" */

	int i = 0;
	void *address, *indicators;
	void *attr[100];

	const char*query;
	TMTopic topic;
	TMTopicScan scan;
	void *basenames;
	struct TMList list;
	

	list.next = NULL;

	list.content = PSI_BASE"at-basenamed-basename";
	TM_GET_TOPIC(vp->topicmap,&at_basenamed_basename_topic,SAM_NAME,"SubjectIndicators",&list);

	list.content = PSI_BASE"role-basenamed";
	TM_GET_TOPIC(vp->topicmap,&role_basenamed_topic,SAM_NAME,"SubjectIndicators",&list);
	list.content = PSI_BASE"role-basename";
	TM_GET_TOPIC(vp->topicmap,&role_basename_topic,SAM_NAME,"SubjectIndicators",&list);
	list.content = PSI_BASE"at-occurring-occurrence";
	TM_GET_TOPIC(vp->topicmap,&at_occurring_occurrence_topic,SAM_NAME,"SubjectIndicators",&list);
	list.content = PSI_BASE"role-occurring";
	TM_GET_TOPIC(vp->topicmap,&role_occurring_topic,SAM_NAME,"SubjectIndicators",&list);
	list.content = PSI_BASE"role-occurrence";
	TM_GET_TOPIC(vp->topicmap,&role_occurrence_topic,SAM_NAME,"SubjectIndicators",&list);

	TM_GET_PARAM(tm_topicmap_get_tm(vp->topicmap),params,"query",query);
	attr[0] = "query";
	attr[1] = (void*)query;
	attr[2] = &Text;
	attr[3] = NULL;


	if(vp->start) vp->start(vp->user_data,"hitlist",attr);

	TMTRACE(TM_QUERY_TRACE,"query is %s\n" _ query);

	if( (e = tm_topicmap_topic_scan_open_with_single_condition(vp->topicmap,
		SAM_NAME,"BaseNames","LIKE",
		(void*)query,&scan)) != TM_OK)
	{
		return (e);
	}
	TMTRACE(TM_QUERY_TRACE,"scan is open now\n");
	while(! tm_topicmap_topic_scan_finished(vp->topicmap,scan) )
	{
		TMTopic y;
		TMTopic o_topic;
		int i=0;
		void *attr[200];
		void *asidp,*data,*address;
		void *basenames;
		TMProperty prop_basenames;

		TMTRACE(TM_QUERY_TRACE,"before fetch\n");
		tm_topicmap_topic_scan_fetch(vp->topicmap,scan,&y);
		TMTRACE(TM_QUERY_TRACE,"after fetch topic=%d\n" _ y);
		
		TM_GET_PROPVAL(vp->topicmap,y,SAM_NAME,"BaseNames",&basenames,&prop_basenames);
		TMTRACE(TM_QUERY_TRACE,"after get propval\n");

		attr[0] = "topic";
		attr[1] = (void*)y;
		attr[2] = &Topic;

		attr[3] = "basenames";
		attr[4] = (void*)basenames;
		attr[5] = prop_basenames->value_type;
		attr[6] = NULL;
		if(vp->start) vp->start(vp->user_data,"topic",attr);


		if(vp->end) vp->end(vp->user_data,"topic");
	}
	if( (e = tm_topicmap_topic_scan_close(vp->topicmap,scan)) != TM_OK)
		return (e);


	if(vp->end) vp->end(vp->user_data,"hitlist");

	
#if 0
XXXX



	TM_GET_PARAM(tm_topicmap_get_tm(vp->topicmap),params,"topic",t_str);
	topic = atoi(t_str);
	if(topic <= 0)
	{
		tm_set_error(tm_topicmap_get_tm(vp->topicmap),
			"\"%s\" does not look like a positive integer" , t_str);
		return TM_E_PARSE;
	}


	TM_GET_PROPVAL(vp->topicmap,topic,SAM_NAME,"SubjectIndicators",&indicators,&prop_sbjind);
	TM_GET_PROPVAL(vp->topicmap,topic,SAM_NAME,"SubjectAddress",&address,&prop_sbjadr);
	TM_GET_PROPVAL(vp->topicmap,topic,SAM_NAME,"BaseNames",&basenames,&prop_basenames);

	attr[i++] = "topic";
	attr[i++] = (void*)topic;
	attr[i++] = &Topic; 

	attr[i++] = "address";
	attr[i++] = address;
	attr[i++] = prop_sbjadr->value_type;

	attr[i++] = "indicators";
	attr[i++] = indicators;
	attr[i++] = prop_sbjind->value_type;

	attr[i++] = "basenames";
	attr[i++] = basenames;
	attr[i++] = prop_basenames->value_type;

	attr[i] = NULL;
	if(vp->start) vp->start(vp->user_data,"topic",attr);

	if(address) tm_value_delete(prop_sbjadr->value_type,&address);
	if(indicators) tm_value_delete(prop_sbjind->value_type,&indicators);
	if(basenames) tm_value_delete(prop_basenames->value_type,&basenames);

	snprintf(pairbuf,sizeof(pairbuf),"(%d,%d)",
		role_occurring_topic,topic);

	if( (e=tm_topicmap_topic_scan_open_with_double_condition(vp->topicmap,
		NULL,"a-sidp","HAS_TYPE", (void*)at_occurring_occurrence_topic,
		"AND",
		NULL,"a-sidp","HAS_PLAYER_PAIR", pairbuf,
		&occ_scan)) != TM_OK)
	{
		return (e);
	}

	while(! tm_topicmap_topic_scan_finished(vp->topicmap,occ_scan) )
	{
		TMTopic y;
		TMProperty prop_asidp;
		TMTopic o_topic;
		int i=0;
		void *attr[200];
		void *asidp,*data,*address,*basenames;

		if( (e = tm_topicmap_topic_scan_fetch(vp->topicmap,occ_scan,&y)) != TM_OK)
			return (e);
		if( (e = tm_topicmap_get_property(vp->topicmap,y,NULL,"a-sidp",&asidp,&prop_asidp)) != TM_OK)
			return (e);

		/* this could be done in STMQL via GET IS13250::a-sidp::getPlayer(6) ... */
		o_topic = (TMTopic)tm_value_call(prop_asidp->value_type,asidp,
					"getPlayer",(void*)role_occurrence_topic);
		tm_value_delete(prop_asidp->value_type,&asidp);

		TM_GET_PROPVAL(vp->topicmap,o_topic,SAM_NAME,"SubjectData",&data,&prop_sbjdata);
		TM_GET_PROPVAL(vp->topicmap,o_topic,SAM_NAME,"SubjectAddress", &address,&prop_sbjadr);
		TM_GET_PROPVAL(vp->topicmap,o_topic,SAM_NAME,"BaseNames", &basenames,&prop_basenames);

		attr[i++] = "data";
		attr[i++] = data;
		attr[i++] = prop_sbjdata->value_type;

		attr[i++] = "address";
		attr[i++] = address;
		attr[i++] = prop_sbjadr->value_type;

		attr[i++] = "title";
			/* get first basename as title if any */
		attr[i++] = basenames ? ((TMList)basenames)->content : NULL;
		attr[i++] = prop_sbjadr->value_type;

		attr[i] = NULL;


		if(vp->start) vp->start(vp->user_data,"occurrence",attr);
		if(data) tm_value_delete(prop_sbjdata->value_type,&data);
		if(address) tm_value_delete(prop_sbjadr->value_type,&address);
		if(basenames) tm_value_delete(prop_basenames->value_type,&basenames);

		if(vp->end) vp->end(vp->user_data,"occurrence");

	}

	if( (e = tm_topicmap_topic_scan_close(vp->topicmap,occ_scan)) != TM_OK)
		return (e);


	if(vp->end) vp->end(vp->user_data,"topic");
#endif

	return (TM_OK);
}

#if 0
TMError tm_view_sam_hitlist(TMTopicMap topicmap, TMParams params)
{
	TMError e;
	TMTopicScan scan;
	TMTopic at_basenamed_basename_topic;
        TMTopic role_basenamed_topic;
        TMTopic role_basename_topic;
	TMTopic at_occurring_occurrence_topic;
        TMTopic role_occurring_topic;
        TMTopic role_occurrence_topic;
	TMModel m;
	TMProperty prop_asidp;
	TMProperty prop_sbjadr;
	TMProperty prop_sbjind;
	TMProperty prop_sbjdata;
	TMProperty prop_inddata;
	const char*attr, *query;

	if( (e=tmtk_default_model_lookup_function("org.gooseworks.SAM",&m)) != TM_OK)
	{
		tm_topicmap_set_error(topicmap,"unable to load model 'SAM', %s",
			tmtk_get_error());
		return (e);
	}
	assert(m); /* FIXEM: is that correct? e==TM_OK => m has been found? */

	GET_PROP(topicmap,prop_asidp,m,"org.gooseworks.IS13250","a-sidp");	
	GET_PROP(topicmap,prop_sbjadr,m,"org.gooseworks.SAM","SubjectAddress");	
	GET_PROP(topicmap,prop_sbjind,m,"org.gooseworks.SAM","SubjectIndicators");	
	GET_PROP(topicmap,prop_sbjdata,m,"org.gooseworks.SAM","SubjectData");	
	GET_PROP(topicmap,prop_inddata,m,"org.gooseworks.SAM","IndicatorData");	


	GET_TOPIC(topicmap,at_basenamed_basename_topic,"org.gooseworks.IS13250","UniqueName","AtBaseNamedBaseName");
	GET_TOPIC(topicmap,role_basenamed_topic,"org.gooseworks.IS13250","UniqueName","RoleBaseNamed");
	GET_TOPIC(topicmap,role_basename_topic,"org.gooseworks.IS13250","UniqueName","RoleBaseName");
	GET_TOPIC(topicmap,at_occurring_occurrence_topic,"org.gooseworks.IS13250","UniqueName","AtOccurringOccurrence");
	GET_TOPIC(topicmap,role_occurring_topic,"org.gooseworks.IS13250","UniqueName","RoleOccurring");
	GET_TOPIC(topicmap,role_occurrence_topic,"org.gooseworks.IS13250","UniqueName","RoleOccurrence");

	attr = tm_params_get(params,"attr");
	query = tm_params_get(params,"query");
	if(!attr)
	{
		tm_topicmap_set_error(topicmap,"attr is a required parameter");
		return (TM_E_PARAM);
	}
	if(!query)
	{
		tm_topicmap_set_error(topicmap,"query is a required parameter");
		return (TM_E_PARAM);
	}
	
	/* get all basenames that match.... */	

	if( (e = tm_topicmap_topic_scan_open_with_single_condition(topicmap,
		"org.gooseworks.SAM","IndicatorData","LIKE",
		(void*)query,&scan)) != TM_OK)
	{
		return (e);
	}
	tm_topicmap_view_start_event(topicmap,"hitlist",NULL);
	while(! tm_topicmap_topic_scan_finished(topicmap,scan) )
	{
		TMTopic basename_topic;
		struct a_membership pair;
		char pairbuf[100];
		TMTopicScan scan2;
		void *idata;

		tm_topicmap_topic_scan_fetch(topicmap,scan,&basename_topic);
		tm_topicmap_get_property(topicmap,basename_topic,"org.gooseworks.SAM","IndicatorData",&idata);

		pair.role = role_basename_topic;
		pair.player = basename_topic;
		snprintf(pairbuf,sizeof(pairbuf),"(%d,%d)",
			role_basename_topic,basename_topic);

		if( (e=tm_topicmap_topic_scan_open_with_double_condition(topicmap,
			"org.gooseworks.IS13250","a-sidp","HAS_TYPE", (void*)at_basenamed_basename_topic,
			"AND",
			"org.gooseworks.IS13250","a-sidp","HAS_PLAYER_PAIR",
			pairbuf,
							&scan2)) != TM_OK)
		{
			return (e);
		}

		while(! tm_topicmap_topic_scan_finished(topicmap,scan2) )
		{
			int i = 0;
			TMTopic a_topic,t_topic;
			void *asidp,*address, *indicators;
			void *attr[100];

			tm_topicmap_topic_scan_fetch(topicmap,scan2,&a_topic);
			tm_topicmap_get_property(topicmap,a_topic,"org.gooseworks.IS13250","a-sidp",&asidp);

			t_topic = (TMTopic)tm_value_call(prop_asidp->value_type,asidp,"getPlayer",(void*)role_basenamed_topic);
			tm_value_delete(prop_asidp->value_type,&asidp);
			assert(asidp == NULL);

			tm_topicmap_get_property(topicmap,t_topic,"org.gooseworks.SAM","SubjectIndicators",&indicators);
			tm_topicmap_get_property(topicmap,t_topic,"org.gooseworks.SAM","SubjectAddress",&address);


			attr[i++] = "basename";
			attr[i++] = (void*)idata;
			attr[i++] = &Text; 

			attr[i++] = "topic";
			attr[i++] = (void*)t_topic;
			attr[i++] = &Topic; 
			if(address)
			{
				attr[i++] = "address";
				attr[i++] = address;
				attr[i++] = prop_sbjadr->value_type;
			}
			if(indicators)
			{
				attr[i++] = "indicators";
				attr[i++] = indicators;
				attr[i++] = prop_sbjind->value_type;
			}
			attr[i] = NULL;

			tm_topicmap_view_start_event(topicmap,"hit",attr);

			if(address)
				tm_value_delete(prop_sbjadr->value_type,&address);
			if(indicators)
				tm_value_delete(prop_sbjind->value_type,&indicators);


			tm_topicmap_view_end_event(topicmap,"hit");
		}
		/* FIXME: free name here??? */

	}

	tm_topicmap_topic_scan_close(topicmap,scan);





#if 0


	if( (e = tm_topicmap_topic_scan_open_with_single_condition(topicmap,
		"org.gooseworks.IS13250","a-sidp","HAS_TYPE",
		(void*)at_basenamed_basename_topic,&scan)) != TM_OK)
	{
		return (e);
	}

	tm_topicmap_view_start_event(topicmap,"index",NULL);
	while(! tm_topicmap_topic_scan_finished(topicmap,scan) )
	{
		int i = 0;
		TMTopic t;
		void *attr[100];
		TMTopicScan occ_scan;
		TMTopic b_topic,t_topic;
		void *asidp,*idata,*address, *indicators;

		tm_topicmap_topic_scan_fetch(topicmap,scan,&t);
		tm_topicmap_get_property(topicmap,t,"org.gooseworks.IS13250","a-sidp",&asidp);

		b_topic = (TMTopic)tm_value_call(prop_asidp->value_type,asidp,"getPlayer",(void*)role_basename_topic);
		t_topic = (TMTopic)tm_value_call(prop_asidp->value_type,asidp,"getPlayer",(void*)role_basenamed_topic);
		tm_value_delete(prop_asidp->value_type,&asidp);
		assert(asidp == NULL);

		tm_topicmap_get_property(topicmap,b_topic,"org.gooseworks.SAM","IndicatorData",&idata);
		tm_topicmap_get_property(topicmap,t_topic,"org.gooseworks.SAM","SubjectIndicators",&indicators);
		tm_topicmap_get_property(topicmap,t_topic,"org.gooseworks.SAM","SubjectAddress",&address);

		
		attr[i++] = "topic";
		attr[i++] = (void*)t_topic;
		attr[i++] = &Topic; 
		if(idata)
		{
			attr[i++] = "label";
			attr[i++] = idata;
			attr[i++] = prop_inddata->value_type;
		}
		if(address)
		{
			attr[i++] = "address";
			attr[i++] = address;
			attr[i++] = prop_sbjadr->value_type;
		}
		if(indicators)
		{
			attr[i++] = "indicators";
			attr[i++] = indicators;
			attr[i++] = prop_sbjind->value_type;
		}
		attr[i] = NULL;

		tm_topicmap_view_start_event(topicmap,"entry",attr);

		if(idata)
			tm_value_delete(prop_inddata->value_type,&idata);
		if(address)
			tm_value_delete(prop_sbjadr->value_type,&address);
		if(indicators)
			tm_value_delete(prop_sbjind->value_type,&indicators);

		/* ================ OCCURRENCES ================= */
		/*
		  GET IS13250::a-sidp::getPlayer( %d )
		WHERE IS13250::a-sidp HAS_TYPE %d
		  AND IS13250::a-sidp HAS_MEMBERSHIP ( %d, %d )

		*/

		if( (e=tm_topicmap_topic_scan_open_with_single_condition(topicmap,
			"org.gooseworks.IS13250","a-sidp","HAS_TYPE",
			(void*)at_occurring_occurrence_topic,&occ_scan)) != TM_OK)
		{
			return (e);
		}
		while(! tm_topicmap_topic_scan_finished(topicmap,occ_scan) )
		{
			TMTopic y;
			TMTopic o_topic;
			int i=0;
			void *attr[200];
			void *asidp,*data,*address;

			tm_topicmap_topic_scan_fetch(topicmap,occ_scan,&y);
			tm_topicmap_get_property(topicmap,y,"org.gooseworks.IS13250","a-sidp",&asidp);
			
			/* FIXME: cond must go into query! */
			if( t_topic != (TMTopic)tm_value_call(prop_asidp->value_type,
					asidp,"getPlayer",(void*)role_occurring_topic))
			{
				tm_value_delete(prop_asidp->value_type,&asidp);
				continue;
			}

			/* this could be done in STMQL via GET IS13250::a-sidp::getPlayer(6) ... */
			o_topic = (TMTopic)tm_value_call(prop_asidp->value_type,asidp,
						"getPlayer",(void*)role_occurrence_topic);
			tm_value_delete(prop_asidp->value_type,&asidp);

			tm_topicmap_get_property(topicmap,o_topic,"org.gooseworks.SAM","SubjectData",&data);
			tm_topicmap_get_property(topicmap,o_topic,"org.gooseworks.SAM","SubjectAddress",
										&address);

			if(data)
			{
				attr[i++] = "data";
				attr[i++] = data;
				attr[i++] = prop_sbjdata->value_type;
			}
			else if(address)
			{
				attr[i++] = "address";
				attr[i++] = address;
				attr[i++] = prop_sbjadr->value_type;
			}
			attr[i] = NULL;


			tm_topicmap_view_start_event(topicmap,"occurrence",attr);
			if(data)
				tm_value_delete(prop_sbjdata->value_type,&data);
			if(address)
				tm_value_delete(prop_sbjadr->value_type,&address);

			tm_topicmap_view_end_event(topicmap,"occurrence");

		}

		tm_topicmap_topic_scan_close(topicmap,occ_scan);


		/* ================= END OCCURRENCES ============== */

		tm_topicmap_view_end_event(topicmap,"entry");

	}

	tm_topicmap_topic_scan_close(topicmap,scan);
#endif
	tm_topicmap_view_end_event(topicmap,"hitlist");
	return (TM_OK);
}
#endif
