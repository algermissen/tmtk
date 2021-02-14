
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

extern struct TMValueType Integer;
extern struct TMValueType Topic;
extern struct TMValueType Text;

TMError tm_view_sam_index(struct view *vp, TMParams params)
{
	TMError e;
	TMTopicScan scan;
	TMTopic at_basenamed_basename_topic;
        TMTopic role_basenamed_topic;
        TMTopic role_basename_topic;
	TMTopic at_occurring_occurrence_topic;
        TMTopic role_occurring_topic;
        TMTopic role_occurrence_topic;
	struct TMList list;
	void *index_attr[4];
	TMTopicSet topics_with_occurrences;
	int j,i,n;

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
 
	/*
        TM_GET_PARAM(tm_topicmap_get_tm(vp->topicmap),params,"topic",t_str);
	*/



	topics_with_occurrences = tm_topicset_new(100);

	if( (e = tm_topicmap_topic_scan_open_with_single_condition(vp->topicmap,
		NULL,"a-sidp","HAS_TYPE",
		(void*)at_occurring_occurrence_topic,&scan)) != TM_OK)
	{
		return (e);
	}

	while(! tm_topicmap_topic_scan_finished(vp->topicmap,scan) )
	{
		TMTopic t,a;
		void *asidp;
		TMProperty prop_asidp,prop_sbjadr;
		void *scr = NULL;

		tm_topicmap_topic_scan_fetch(vp->topicmap,scan,&a);
		TM_GET_PROPVAL(vp->topicmap,a,NULL,"a-sidp",&asidp,&prop_asidp);

		t = (TMTopic)tm_value_call(prop_asidp->value_type,asidp,"getPlayer",(void*)role_occurring_topic);
		tm_value_delete(prop_asidp->value_type,&asidp);
		TM_GET_PROPVAL(vp->topicmap,t,SAM_NAME,"SubjectAddress",&scr,&prop_sbjadr);
		if(scr)
		{
			tm_value_delete(prop_sbjadr->value_type,&scr);
			continue;
		}
			
		tm_topicset_add(topics_with_occurrences,t);
	}
	index_attr[0] = "count";
	index_attr[1] = (void*)tm_topicset_size(topics_with_occurrences);
	index_attr[2] = &Integer;
	index_attr[3] = NULL;


	if(vp->start)
		vp->start(vp->user_data,"index",index_attr);


	n = tm_topicset_size(topics_with_occurrences);
	for(j =0; j<n;j++)
	{
		TMTopicScan occ_scan;
		TMProperty prop_basenames;
		void *basenames;
		void *entry_attr[100];
		TMTopic topic = tm_topicset_get(topics_with_occurrences,j);

		TM_GET_PROPVAL(vp->topicmap,topic,SAM_NAME,"BaseNames",&basenames,&prop_basenames);
		i=0;
		entry_attr[i++] = "topic";
		entry_attr[i++] = (void*)topic;
		entry_attr[i++] = &Topic; 
		if(basenames)
		{
			entry_attr[i++] = "basenames";
			entry_attr[i++] = basenames;
			entry_attr[i++] = prop_basenames->value_type;
		}
		entry_attr[i] = NULL;
	
		if(vp->start)
			vp->start(vp->user_data,"entry",entry_attr);


		if( (e=tm_topicmap_topic_scan_open_with_single_condition(vp->topicmap,
			NULL,"a-sidp","HAS_TYPE",
			(void*)at_occurring_occurrence_topic,&occ_scan)) != TM_OK)
		{
			return (e);
		}
		while(! tm_topicmap_topic_scan_finished(vp->topicmap,occ_scan) )
		{
			TMTopic y;
			TMTopic o_topic;
			TMProperty prop_asidp;
			TMProperty prop_sbjadr;
			TMProperty prop_sbjdata;
			TMProperty prop_basenames;
			int i=0;
			void *attr[100];
			void *asidp,*data,*address,*basenames;

			tm_topicmap_topic_scan_fetch(vp->topicmap,occ_scan,&y);
			TM_GET_PROPVAL(vp->topicmap,y,NULL,"a-sidp",&asidp,&prop_asidp);
			
			/* FIXME: cond must go into query! */
			if( topic != (TMTopic)tm_value_call(prop_asidp->value_type,
					asidp,"getPlayer",(void*)role_occurring_topic))
			{
				tm_value_delete(prop_asidp->value_type,&asidp);
				continue;
			}

			/* this could be done in STMQL via GET IS13250::a-sidp::getPlayer(6) ... */
			o_topic = (TMTopic)tm_value_call(prop_asidp->value_type,asidp,
						"getPlayer",(void*)role_occurrence_topic);
			tm_value_delete(prop_asidp->value_type,&asidp);

			TM_GET_PROPVAL(vp->topicmap,o_topic,SAM_NAME,"SubjectData",&data,&prop_sbjdata);
			TM_GET_PROPVAL(vp->topicmap,o_topic,SAM_NAME,"SubjectAddress", &address,&prop_sbjadr);
			TM_GET_PROPVAL(vp->topicmap,o_topic,SAM_NAME,"BaseNames", &basenames,&prop_basenames);

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

				if(basenames)
				{
					attr[i++] = "title";
					attr[i++] = ((TMList)basenames)->content;
					attr[i++] = &Text;
				}
			}
			attr[i] = NULL;


			if(vp->start)
				vp->start(vp->user_data,"occurrence",attr);
			if(data)
				tm_value_delete(prop_sbjdata->value_type,&data);
			if(address)
				tm_value_delete(prop_sbjadr->value_type,&address);
			if(basenames)
				tm_value_delete(prop_basenames->value_type,&basenames);

			if(vp->end)
				vp->end(vp->user_data,"occurrence");

		}

		tm_topicmap_topic_scan_close(vp->topicmap,occ_scan);



		if(vp->end)
			vp->end(vp->user_data,"entry");
		if(basenames)
		{
			tm_value_delete(prop_basenames->value_type,&basenames);
		}

	}

#if 0



	if( (e = tm_topicmap_topic_scan_open_with_single_condition(vp->topicmap,
		NULL,"a-sidp","HAS_TYPE",
		(void*)at_basenamed_basename_topic,&scan)) != TM_OK)
	{
		return (e);
	}

	if(vp->start)
		vp->start(vp->user_data,"index",NULL);

	while(! tm_topicmap_topic_scan_finished(vp->topicmap,scan) )
	{
		int i = 0;
		TMTopic t;
		void *attr[100];
		TMTopicScan occ_scan;
		TMTopic b_topic,t_topic;
		void *asidp,*idata,*address, *indicators;
		TMProperty prop_asidp;

		tm_topicmap_topic_scan_fetch(vp->topicmap,scan,&t);
		tm_topicmap_get_property(vp->topicmap,t,NULL,"a-sidp",&asidp,&prop_asidp);

		b_topic = (TMTopic)tm_value_call(prop_asidp->value_type,asidp,"getPlayer",(void*)role_basename_topic);
		t_topic = (TMTopic)tm_value_call(prop_asidp->value_type,asidp,"getPlayer",(void*)role_basenamed_topic);
		tm_value_delete(prop_asidp->value_type,&asidp);
		assert(asidp == NULL);

		tm_topicmap_get_property(vp->topicmap,b_topic,"org.gooseworks.SAM","IndicatorData",&idata,NULL);
		tm_topicmap_get_property(vp->topicmap,t_topic,"org.gooseworks.SAM","SubjectIndicators",&indicators,NULL);
		tm_topicmap_get_property(vp->topicmap,t_topic,"org.gooseworks.SAM","SubjectAddress",&address,NULL);

		
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

		if(vp->start)
			vp->start(vp->user_data,"entry",attr);

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



		/* ================= END OCCURRENCES ============== */

		if(vp->end)
			vp->end(vp->user_data,"entry");

	}

	tm_topicmap_topic_scan_close(vp->topicmap,scan);
#endif
	if(vp->end)
		vp->end(vp->user_data,"index");
	tm_topicset_delete(&topics_with_occurrences);
	return (TM_OK);
}

