

/*
 * $Id$
 *
 * Copyright (c) 2002 Jan Algermissen
 * See the file "COPYING" for copying permission.
 *
 */
#include <topicmaps.h>

#include <tmdevel.h>		/* for development macros */
#include "df.h"

extern struct TMValueType Topic;

/*

<!ELEMENT role_playings (role_playing*)>
<!ATTLIST role_playings
	topic			Topic
	player_basenames	TextSet
	role_basenames		TextSet
	other_role_basenames	TextSet
>
<!ELEMENT role_playing EMPTY>
<!ATTLIST role_playing
	topic			Topic
	basenames		TextSet
>

*/
TMError tm_view_role_playings(struct view *vp, TMParams params)
{
	TMError e;
	TMTopicScan scan;
	TMTopic at_basenamed_basename_topic;
        TMTopic role_basenamed_topic;
        TMTopic role_basename_topic;
	/*
	TMTopic at_occurring_occurrence_topic;
        TMTopic role_occurring_topic;
        TMTopic role_occurrence_topic;
	*/

        TMTopic at_topic;
        TMTopic role_topic;
        TMTopic other_role_topic;
	/*
	TMProperty prop_asidp;
	TMProperty prop_sbjadr;
	TMProperty prop_sbjind;
	TMProperty prop_sbjdata;
	TMProperty prop_inddata;
	TMProperty prop_basenames;
	*/
	TMProperty prop_basenames;
	const char *topic_string, *at_string, *role_string, *other_role_string;
	TMTopic topic;
	struct TMList list;


	int i = 0;
	/*
	void *address, *indicators;
	*/
	void *attr[100];
	void *role_basenames, *other_role_basenames;
	void *player_basenames, *other_player_basenames;

	/*
	TMTopic basename_topic;
	const char*t_str;
	*/
	char pairbuf[100];

	list.next = NULL;


	list.content = PSI_BASE"at-basenamed-basename";
	TM_GET_TOPIC(vp->topicmap,&at_basenamed_basename_topic,SAM_NAME,"SubjectIndicators",&list);
	list.content = PSI_BASE"role-basenamed";
	TM_GET_TOPIC(vp->topicmap,&role_basenamed_topic,SAM_NAME,"SubjectIndicators",&list);
	list.content = PSI_BASE"role-basename";
	TM_GET_TOPIC(vp->topicmap,&role_basename_topic,SAM_NAME,"SubjectIndicators",&list);

	/*
	GET_TOPIC(vp->topicmap,at_occurring_occurrence_topic,"org.gooseworks.IS13250","UniqueName","AtOccurringOccurrence");
	GET_TOPIC(vp->topicmap,role_occurring_topic,"org.gooseworks.IS13250","UniqueName","RoleOccurring");
	GET_TOPIC(vp->topicmap,role_occurrence_topic,"org.gooseworks.IS13250","UniqueName","RoleOccurrence");
	*/

	TM_GET_PARAM(tm_topicmap_get_tm(vp->topicmap),params,"topic",topic_string);
	topic = atoi(topic_string);
	TM_GET_PARAM(tm_topicmap_get_tm(vp->topicmap),params,"at",at_string);
	TM_GET_PARAM(tm_topicmap_get_tm(vp->topicmap),params,"role",role_string);
	TM_GET_PARAM(tm_topicmap_get_tm(vp->topicmap),params,"other_role",other_role_string);

	list.content = (void*)at_string;
	TM_GET_TOPIC(vp->topicmap,&at_topic,SAM_NAME,"SubjectIndicators",&list);

	list.content = (void*)role_string;
	TM_GET_TOPIC(vp->topicmap,&role_topic,SAM_NAME,"SubjectIndicators",&list);

	list.content =(void*)other_role_string;
	TM_GET_TOPIC(vp->topicmap,&other_role_topic,SAM_NAME,"SubjectIndicators",&list);



	TM_GET_PROPVAL(vp->topicmap,topic,SAM_NAME,"BaseNames",&player_basenames,&prop_basenames);
	TM_GET_PROPVAL(vp->topicmap,role_topic,SAM_NAME,"BaseNames",&role_basenames,NULL);
	TM_GET_PROPVAL(vp->topicmap,other_role_topic,SAM_NAME,"BaseNames",&other_role_basenames,NULL);


	attr[i++] = "topic";
	attr[i++] = (void*)topic;
	attr[i++] = &Topic; 

	attr[i++] = "player_basenames";
	attr[i++] = (void*)player_basenames;
	attr[i++] = prop_basenames->value_type; 

	attr[i++] = "role_basenames";
	attr[i++] = (void*)role_basenames;
	attr[i++] = prop_basenames->value_type; 

	attr[i++] = "other_role_basenames";
	attr[i++] = (void*)other_role_basenames;
	attr[i++] = prop_basenames->value_type; 

	attr[i] = NULL;
	if(vp->start) vp->start(vp->user_data,"role_playings",attr);

	snprintf(pairbuf,sizeof(pairbuf),"(%d,%d)",
		role_topic,topic);

	if( (e=tm_topicmap_topic_scan_open_with_double_condition(vp->topicmap,
		NULL,"a-sidp","HAS_TYPE", (void*)at_topic,
		"AND",
		NULL,"a-sidp","HAS_PLAYER_PAIR",
		pairbuf,
						&scan)) != TM_OK)
	{
		return (e);
	}

	while(! tm_topicmap_topic_scan_finished(vp->topicmap,scan) )
	{
		int i = 0;
		TMTopic a_topic;
        	TMTopic other_player_topic;
		void *asidp;
		void *attr[100];
		TMProperty prop_asidp;
		TMProperty prop_basenames;

		tm_topicmap_topic_scan_fetch(vp->topicmap,scan,&a_topic);
		TM_GET_PROPVAL(vp->topicmap,a_topic,NULL,"a-sidp",&asidp,&prop_asidp);

		other_player_topic = (TMTopic)tm_value_call(prop_asidp->value_type,asidp,"getPlayer",(void*)other_role_topic);
		tm_value_delete(prop_asidp->value_type,&asidp);
		assert(asidp == NULL);

		TM_GET_PROPVAL(vp->topicmap,other_player_topic,SAM_NAME,"BaseNames",&other_player_basenames,&prop_basenames);



		attr[i++] = "topic";
		attr[i++] = (void*)other_player_topic;
		attr[i++] = &Topic; 

		attr[i++] = "basenames";
		attr[i++] = (void*)other_player_basenames;
		attr[i++] = prop_basenames->value_type; 

		attr[i] = NULL;

		if(vp->start) vp->start(vp->user_data,"role_playing",attr);

		if(vp->end) vp->end(vp->user_data,"role_playing");
	}
	/* FIXME: free name here??? */
	tm_topicmap_topic_scan_close(vp->topicmap,scan);



	if(vp->end) vp->end(vp->user_data,"role_playings");


	return (TM_OK);
}
#if 0
/*
 * $Id$
 *
 * Copyright (c) 2002 Jan Algermissen
 * See the file "COPYING" for copying permission.
 *
 */

#include <topicmaps.h>

#include <tmdevel.h>		/* for development macros */
#include "df.h"

extern struct TMValueType Topic;

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



TMError tm_view_sam_topic(struct view *vp, TMParams params)
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

	const char*t_str;
	TMTopic topic;
	TMTopicScan occ_scan;
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

	return (TM_OK);
}
#endif
