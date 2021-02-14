/*
 * $Id$
 *
 * Copyright (c) 2002 Jan Algermissen
 * See the file "COPYING" for copying permission.
 *
 */

#include <tmstack.h>
#include <tmmem.h>
#include <tmtrace.h>
#include <tmutil.h>
#include <tmlog.h>
#include <tmtable.h>
#include <tmtopicset.h>
#include <tmparams.h>
#include <tmerror.h>

#include <tmassertion.h>
#include <tmtopicmap.h>
#include <tmstorage.h>
#include <tmdefaultstores.h>
#include <tmbasictypes.h>
#include <tmassert.h>


#include "../base/view.h"
#include <tmdevel.h>            /* for development macros */                                   

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
	TMTopic at_occurring_occurrence_topic;
        TMTopic role_occurring_topic;
        TMTopic role_occurrence_topic;

        TMTopic at_topic;
        TMTopic role_topic;
        TMTopic other_role_topic;

	TMModel m;
	TMProperty prop_asidp;
	TMProperty prop_sbjadr;
	TMProperty prop_sbjind;
	TMProperty prop_sbjdata;
	TMProperty prop_inddata;
	TMProperty prop_basenames;
	const char *topic_string, *at_string, *role_string, *other_role_string;
	TMTopic topic;

	int i = 0;
	void *address, *indicators;
	void *attr[100];
	void *role_basenames, *other_role_basenames;
	void *player_basenames, *other_player_basenames;

	const char*t_str;

	TMTopic basename_topic;
	char pairbuf[100];
	void *idata;

	if( (e=tmtk_default_model_lookup_function(vp->tm,"org.gooseworks.SAM",&m)) != TM_OK)
	{
		tm_topicmap_set_error(vp->topicmap,"unable to load model 'SAM', %s",
			tmtk_get_error());
		return (e);
	}
	assert(m); /* FIXEM: is that correct? e==TM_OK => m has been found? */

	GET_PROP(vp->topicmap,prop_asidp,m,"org.gooseworks.IS13250","a-sidp");	
	GET_PROP(vp->topicmap,prop_sbjadr,m,"org.gooseworks.SAM","SubjectAddress");	
	GET_PROP(vp->topicmap,prop_sbjind,m,"org.gooseworks.SAM","SubjectIndicators");	
	GET_PROP(vp->topicmap,prop_sbjdata,m,"org.gooseworks.SAM","SubjectData");	
	GET_PROP(vp->topicmap,prop_inddata,m,"org.gooseworks.SAM","IndicatorData");	
	GET_PROP(vp->topicmap,prop_basenames,m,"org.gooseworks.SAM","BaseNames");	


	GET_TOPIC(vp->topicmap,at_basenamed_basename_topic,"org.gooseworks.IS13250","UniqueName","AtBaseNamedBaseName");
	GET_TOPIC(vp->topicmap,role_basenamed_topic,"org.gooseworks.IS13250","UniqueName","RoleBaseNamed");
	GET_TOPIC(vp->topicmap,role_basename_topic,"org.gooseworks.IS13250","UniqueName","RoleBaseName");
	GET_TOPIC(vp->topicmap,at_occurring_occurrence_topic,"org.gooseworks.IS13250","UniqueName","AtOccurringOccurrence");
	GET_TOPIC(vp->topicmap,role_occurring_topic,"org.gooseworks.IS13250","UniqueName","RoleOccurring");
	GET_TOPIC(vp->topicmap,role_occurrence_topic,"org.gooseworks.IS13250","UniqueName","RoleOccurrence");

	TM_GET_PARAM(vp->topicmap,params,"topic",topic_string);
	topic = atoi(topic_string);
	TM_GET_PARAM(vp->topicmap,params,"at",at_string);
	TM_GET_PARAM(vp->topicmap,params,"role",role_string);
	TM_GET_PARAM(vp->topicmap,params,"other_role",other_role_string);


	GET_TOPIC(vp->topicmap,at_topic,"org.gooseworks.IS13250","UniqueName",at_string);
	GET_TOPIC(vp->topicmap,role_topic,"org.gooseworks.IS13250","UniqueName",role_string);
	GET_TOPIC(vp->topicmap,other_role_topic,"org.gooseworks.IS13250","UniqueName",other_role_string);


	TM_GET_PROPVAL(vp->topicmap,topic,"org.gooseworks.SAM","BaseNames",&player_basenames,NULL);
	TM_GET_PROPVAL(vp->topicmap,role_topic,"org.gooseworks.SAM","BaseNames",&role_basenames,NULL);
	TM_GET_PROPVAL(vp->topicmap,other_role_topic,"org.gooseworks.SAM","BaseNames",&other_role_basenames,NULL);


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
		"org.gooseworks.IS13250","a-sidp","HAS_TYPE", (void*)at_topic,
		"AND",
		"org.gooseworks.IS13250","a-sidp","HAS_PLAYER_PAIR",
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
		void *asidp,*idata;
		void *attr[100];

		tm_topicmap_topic_scan_fetch(vp->topicmap,scan,&a_topic);
		TM_GET_PROPVAL(vp->topicmap,a_topic,"org.gooseworks.IS13250","a-sidp",&asidp,NULL);

		other_player_topic = (TMTopic)tm_value_call(prop_asidp->value_type,asidp,"getPlayer",(void*)other_role_topic);
		tm_value_delete(prop_asidp->value_type,&asidp);
		assert(asidp == NULL);

		TM_GET_PROPVAL(vp->topicmap,other_player_topic,"org.gooseworks.SAM","BaseNames",&other_player_basenames,NULL);



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
