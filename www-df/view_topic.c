
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


