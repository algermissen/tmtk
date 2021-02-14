
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
#include <tmdevel.h>		/* for development macros */


#if 0
void tm_view_sam_index(TMTopicMap topicmap)
{
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
	TMProperty prop_sbjdata;
	TMProperty prop_inddata;

	tmtk_default_model_lookup_function("org.gooseworks.SAM",&m);
	prop_asidp = tm_model_get_property(m,"a-sidp");
	prop_sbjadr = tm_model_get_property(m,"SubjectAddress");
	prop_sbjdata = tm_model_get_property(m,"SubjectData");
	prop_inddata = tm_model_get_property(m,"IndicatorData");
	assert(prop_asidp);
	assert(prop_sbjadr);
	assert(prop_sbjdata);
	assert(prop_inddata);

	if( tm_topicmap_get_topic(topicmap,"org.gooseworks.IS13250","UniqueName","AtBaseNamedBaseName",
                                &(at_basenamed_basename_topic)) != TM_OK)
        {
                assert(0);
        }
        if( tm_topicmap_get_topic(topicmap,"org.gooseworks.IS13250","UniqueName","RoleBaseNamed",
                                &(role_basenamed_topic)) != TM_OK )
        {
                assert(0);
        }
        if( tm_topicmap_get_topic(topicmap,"org.gooseworks.IS13250","UniqueName","RoleBaseName",
                                &(role_basename_topic)) != TM_OK )
        {
                assert(0);
        }
	if( tm_topicmap_get_topic(topicmap,"org.gooseworks.IS13250","UniqueName","AtOccurringOccurrence",
                                &(at_occurring_occurrence_topic)) != TM_OK)
        {
                assert(0);
        }
        if( tm_topicmap_get_topic(topicmap,"org.gooseworks.IS13250","UniqueName","RoleOccurring",
                                &(role_occurring_topic)) != TM_OK )
        {
                assert(0);
        }
        if( tm_topicmap_get_topic(topicmap,"org.gooseworks.IS13250","UniqueName","RoleOccurrence",
                                &(role_occurrence_topic)) != TM_OK )
        {
                assert(0);
        }
	assert(at_basenamed_basename_topic);
        assert(role_basenamed_topic);
        assert(role_basename_topic);
	assert(at_occurring_occurrence_topic);
        assert(role_occurring_topic);
        assert(role_occurrence_topic);



	if( tm_topicmap_topic_scan_open_with_single_condition(topicmap,
			"org.gooseworks.IS13250","a-sidp","HAS_TYPE",(void*)at_basenamed_basename_topic,&scan) != TM_OK)
	{
		fprintf(stderr,"cannot open scan, %s\n", tm_topicmap_get_error(topicmap));
		return;
	}

	fprintf(f,"Extracted Index:\n\n");
	/*
	tm->view_start_handler(tm->view_user_data,"index",NULL);
	*/
	tm_topicmap_view_start_event("index",NULL);

	while(! tm_topicmap_topic_scan_finished(topicmap,scan) )
	{
		TMTopic t;
		TMTopic b_topic;
		TMTopic t_topic;
		TMTopicScan occ_scan;
		/*
		char buf[1024];
		*/
		void *v = NULL,*v2 = NULL, *v3 = NULL;
		tm_topicmap_topic_scan_fetch(topicmap,scan,&t);
		tm_topicmap_get_property(topicmap,t,"org.gooseworks.IS13250","a-sidp",&v);

		b_topic = (TMTopic)tm_value_call(prop_asidp->value_type,v,"getPlayer",(void*)role_basename_topic);
		t_topic = (TMTopic)tm_value_call(prop_asidp->value_type,v,"getPlayer",(void*)role_basenamed_topic);
		tm_value_delete(prop_asidp->value_type,&v);

		tm_topicmap_get_property(topicmap,b_topic,"org.gooseworks.SAM","IndicatorData",&v2);
		tm_topicmap_get_property(topicmap,t_topic,"org.gooseworks.SAM","SubjectAddress",&v3);

		/* we know these are strings!  */
		/*
		tm_value_to_string(p->value_type,v,buf,sizeof(buf));
		*/
		
		fprintf(f,"%s (%d)\n",v2 ? (char*)v2 : "[no-basename]",b_topic);
		if(v2)
		{
			void *attr[20];
			attr[0] = "label";
			attr[1] = prop_sbjdata->value_type;
			attr[2] = v2;
			attr[0] = "href";
			attr[1] = prop_sbjadr->value_type;
			attr[2] = v3;
			tm_topicmap_view_start_event("entry",attr);
			tm_value_delete(prop_inddata->value_type,&v2);
		}
		if( v3 != NULL)
		{
			fprintf(f,"  SubjectAddress: [%s] (%d)\n\n",  (char*)v3 ,t_topic);
			/*
			tm_value_delete(prop_sbjadr->value_type,&v3);
			*/
		}

		/* ================ OCCURRENCES ================= */
		/*
		  GET IS13250::a-sidp::getPlayer( %d )
		WHERE IS13250::a-sidp HAS_TYPE %d
		  AND IS13250::a-sidp HAS_MEMBERSHIP ( %d, %d )

		*/

		if( tm_topicmap_topic_scan_open_with_single_condition(topicmap,
			"org.gooseworks.IS13250","a-sidp","HAS_TYPE",(void*)at_occurring_occurrence_topic,&occ_scan) != TM_OK)
		{
			fprintf(stderr,"cannot open scan, %s\n", tm_topicmap_get_error(topicmap));
			return;
		}
		while(! tm_topicmap_topic_scan_finished(topicmap,occ_scan) )
		{
			TMTopic y;
			TMTopic o_topic;
			/*
			char buf[1024];
			*/
			void *asidp,*v2, *v3;
			tm_topicmap_topic_scan_fetch(topicmap,occ_scan,&y);
			tm_topicmap_get_property(topicmap,y,"org.gooseworks.IS13250","a-sidp",&asidp);

			if( t_topic != (TMTopic)tm_value_call(prop_asidp->value_type,asidp,"getPlayer",(void*)role_occurring_topic))
			{
				tm_value_delete(prop_asidp->value_type,&asidp);
				continue;
			}

			/* this could be done in STMQL via GET IS13250::a-sidp::getPlayer(6) ... */
			o_topic = (TMTopic)tm_value_call(prop_asidp->value_type,asidp,"getPlayer",(void*)role_occurrence_topic);
			tm_value_delete(prop_asidp->value_type,&asidp);

			tm_topicmap_get_property(topicmap,o_topic,"org.gooseworks.SAM","SubjectData",&v2);
			tm_topicmap_get_property(topicmap,o_topic,"org.gooseworks.SAM","SubjectAddress",&v3);


			/*
			FIXME: this might be done like:
			tm_value_call(prop_asidp->value_type,v,"recodeAs","latin1",buf,sizeof(buf));
			*/
			/*
			if( v2 == NULL)	
			{
				fprintf(f,"   [%s] (%d)\n",v3 ? (char*)v3 : "--ERR: no SubjectAddress --",o_topic);
			}
			*/
			if( v2 == NULL && v3)	
			{
				fprintf(f,"   [%s] (%d)\n",(char*)v3 ,o_topic);
			}
			else
			{
				if(v2) {
				fprintf(f,"  [%s] (%d)\n\n", (char*)v2 ,t_topic);
				tm_value_delete(prop_sbjdata->value_type,&v2);
				}

			}
			if(v3)
			{
				tm_value_delete(prop_sbjadr->value_type,&v3);
			}
			else
			{
				/*
				fprintf(f,"     ---ERROR: no SubjectAdress ---\n");
				*/
			}
		}

		tm_topicmap_topic_scan_close(topicmap,occ_scan);
		fprintf(f,"\n");


		/* ================= END OCCURRENCES ============== */


	}

	tm_topicmap_topic_scan_close(topicmap,scan);

}
#endif

#define GET_PROP(tm,propvar,m,modelname,propname) \
	do { \
	if( ( (propvar) = tm_model_get_property((m),modelname,propname)) == NULL) \
		{ \
			tm_topicmap_set_error((tm),"property %s::%s not found", \
							modelname, propname); \
			return TM_E_PROP_NOT_FOUND; \
		} \
	} while(0)

#define GET_TOPIC(tm,topicvar,modelname,propname,propvalue) \
	do { \
		TMError e; \
		if( (e=tm_topicmap_get_topic((tm), \
			modelname,propname,propvalue, \
                                &((topicvar)),NULL)) != TM_OK) \
        	{ \
			return (e); \
        	} \
		if(!topicvar) \
        	{ \
			tm_topicmap_set_error( (tm),"topic with %s::%s=%s not found", \
							modelname, propname, propvalue); \
			return (TM_E_TOPIC_NOT_FOUND); \
        	} \
	} while(0)




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
	TMModel m;
	TMProperty prop_asidp;
	TMProperty prop_sbjadr;
	TMProperty prop_sbjind;
	TMProperty prop_sbjdata;
	TMProperty prop_inddata;

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


	GET_TOPIC(vp->topicmap,at_basenamed_basename_topic,"org.gooseworks.IS13250","UniqueName","AtBaseNamedBaseName");
	GET_TOPIC(vp->topicmap,role_basenamed_topic,"org.gooseworks.IS13250","UniqueName","RoleBaseNamed");
	GET_TOPIC(vp->topicmap,role_basename_topic,"org.gooseworks.IS13250","UniqueName","RoleBaseName");
	GET_TOPIC(vp->topicmap,at_occurring_occurrence_topic,"org.gooseworks.IS13250","UniqueName","AtOccurringOccurrence");
	GET_TOPIC(vp->topicmap,role_occurring_topic,"org.gooseworks.IS13250","UniqueName","RoleOccurring");
	GET_TOPIC(vp->topicmap,role_occurrence_topic,"org.gooseworks.IS13250","UniqueName","RoleOccurrence");


	if( (e = tm_topicmap_topic_scan_open_with_single_condition(vp->topicmap,
		"org.gooseworks.IS13250","a-sidp","HAS_TYPE",
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

		tm_topicmap_topic_scan_fetch(vp->topicmap,scan,&t);
		tm_topicmap_get_property(vp->topicmap,t,"org.gooseworks.IS13250","a-sidp",&asidp,NULL);

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

		if( (e=tm_topicmap_topic_scan_open_with_single_condition(vp->topicmap,
			"org.gooseworks.IS13250","a-sidp","HAS_TYPE",
			(void*)at_occurring_occurrence_topic,&occ_scan)) != TM_OK)
		{
			return (e);
		}
		while(! tm_topicmap_topic_scan_finished(vp->topicmap,occ_scan) )
		{
			TMTopic y;
			TMTopic o_topic;
			int i=0;
			void *attr[200];
			void *asidp,*data,*address;

			tm_topicmap_topic_scan_fetch(vp->topicmap,occ_scan,&y);
			tm_topicmap_get_property(vp->topicmap,y,"org.gooseworks.IS13250","a-sidp",&asidp,NULL);
			
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

			tm_topicmap_get_property(vp->topicmap,o_topic,"org.gooseworks.SAM","SubjectData",&data,NULL);
			tm_topicmap_get_property(vp->topicmap,o_topic,"org.gooseworks.SAM","SubjectAddress",
										&address,NULL);

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


			if(vp->start)
				vp->start(vp->user_data,"occurrence",attr);
			if(data)
				tm_value_delete(prop_sbjdata->value_type,&data);
			if(address)
				tm_value_delete(prop_sbjadr->value_type,&address);

			if(vp->end)
				vp->end(vp->user_data,"occurrence");

		}

		tm_topicmap_topic_scan_close(vp->topicmap,occ_scan);


		/* ================= END OCCURRENCES ============== */

		if(vp->end)
			vp->end(vp->user_data,"entry");

	}

	tm_topicmap_topic_scan_close(vp->topicmap,scan);
	if(vp->end)
		vp->end(vp->user_data,"index");
	return (TM_OK);
}

/* ---------------------------------------------------------------------------------*/
# if 0
TMError tm_view_sam_namelist(TMTopicMap topicmap, TMParams params)
{
	TMTopicScan scan;
	/*
	TMTopic at_basenamed_basename_topic;
        TMTopic role_basenamed_topic;
        TMTopic role_basename_topic;
	TMTopic at_occurring_occurrence_topic;
        TMTopic role_occurring_topic;
        TMTopic role_occurrence_topic;
	*/
	TMModel m;
	TMProperty prop_asidp;
	TMProperty prop_sbjadr;
	TMProperty prop_sbjdata;
	TMProperty prop_inddata;
	TMError e;
	const char *query;

	tmtk_default_model_lookup_function("org.gooseworks.SAM",&m);
	prop_asidp = tm_model_get_property(m,"org.gooseworks.IS13250","a-sidp");
	prop_sbjadr = tm_model_get_property(m,NULL,"SubjectAddress");
	prop_sbjdata = tm_model_get_property(m,NULL,"SubjectData");
	prop_inddata = tm_model_get_property(m,NULL,"IndicatorData");
	assert(prop_asidp);
	assert(prop_sbjadr);
	assert(prop_sbjdata);
	assert(prop_inddata);

	query = tm_params_get(params,"query");
	if(!query)
	{
		tm_topicmap_set_error(topicmap,"parameter 'query' required");
		return (TM_E_PARAM);
	}
	/*
	fprintf(stderr,"_%s_\n",query);
	*/
	/*
	if( tm_topicmap_get_topic(topicmap,"org.gooseworks.IS13250","UniqueName","AtBaseNamedBaseName",
                                &(at_basenamed_basename_topic)) != TM_OK)
        {
                assert(0);
        }
        if( tm_topicmap_get_topic(topicmap,"org.gooseworks.IS13250","UniqueName","RoleBaseNamed",
                                &(role_basenamed_topic)) != TM_OK )
        {
                assert(0);
        }
        if( tm_topicmap_get_topic(topicmap,"org.gooseworks.IS13250","UniqueName","RoleBaseName",
                                &(role_basename_topic)) != TM_OK )
        {
                assert(0);
        }
	if( tm_topicmap_get_topic(topicmap,"org.gooseworks.IS13250","UniqueName","AtOccurringOccurrence",
                                &(at_occurring_occurrence_topic)) != TM_OK)
        {
                assert(0);
        }
        if( tm_topicmap_get_topic(topicmap,"org.gooseworks.IS13250","UniqueName","RoleOccurring",
                                &(role_occurring_topic)) != TM_OK )
        {
                assert(0);
        }
        if( tm_topicmap_get_topic(topicmap,"org.gooseworks.IS13250","UniqueName","RoleOccurrence",
                                &(role_occurrence_topic)) != TM_OK )
        {
                assert(0);
        }
	assert(at_basenamed_basename_topic);
        assert(role_basenamed_topic);
        assert(role_basename_topic);
	assert(at_occurring_occurrence_topic);
        assert(role_occurring_topic);
        assert(role_occurrence_topic);
	*/



	if( (e = tm_topicmap_topic_scan_open_with_single_condition(topicmap,
			"org.gooseworks.SAM","IndicatorData","LIKE",query,&scan)) != TM_OK)
	{
		fprintf(stderr,"cannot open scan, %s\n", tm_topicmap_get_error(topicmap));
		return e;
	}


	tm_topicmap_view_start_event(topicmap,"namelist",NULL);
	while(! tm_topicmap_topic_scan_finished(topicmap,scan) )
	{
		TMTopic t;
		TMTopic b_topic;
		TMTopic t_topic;
		TMTopicScan occ_scan;
		void *a[10];
		/*
		char buf[1024];
		*/
		void *v = NULL,*v2 = NULL, *v3 = NULL;
		tm_topicmap_topic_scan_fetch(topicmap,scan,&t);
		tm_topicmap_get_property(topicmap,t,"org.gooseworks.SAM","IndicatorData",&v);

		a[0] = "name";
		a[1] = v;
		a[2] = prop_inddata->value_type;
		a[3] = NULL;
		tm_topicmap_view_start_event(topicmap,"hit",a);

#if 0
		b_topic = (TMTopic)tm_value_call(prop_asidp->value_type,v,"getPlayer",(void*)role_basename_topic);
		t_topic = (TMTopic)tm_value_call(prop_asidp->value_type,v,"getPlayer",(void*)role_basenamed_topic);
		tm_value_delete(prop_asidp->value_type,&v);

		tm_topicmap_get_property(topicmap,b_topic,"org.gooseworks.SAM","IndicatorData",&v2);
		tm_topicmap_get_property(topicmap,t_topic,"org.gooseworks.SAM","SubjectAddress",&v3);

		/* we know these are strings!  */
		/*
		tm_value_to_string(p->value_type,v,buf,sizeof(buf));
		*/
		
		fprintf(f,"%s (%d)\n",v2 ? (char*)v2 : "[no-basename]",b_topic);
		if(v2)
		{
			void *attr[20];
			attr[0] = "label";
			attr[1] = prop_sbjdata->value_type;
			attr[2] = v2;
			attr[0] = "href";
			attr[1] = prop_sbjadr->value_type;
			attr[2] = v3;
			tm_topicmap_view_start_event("entry",attr);
			tm_value_delete(prop_inddata->value_type,&v2);
		}
		if( v3 != NULL)
		{
			fprintf(f,"  SubjectAddress: [%s] (%d)\n\n",  (char*)v3 ,t_topic);
			/*
			tm_value_delete(prop_sbjadr->value_type,&v3);
			*/
		}
#endif

		tm_topicmap_view_end_event(topicmap,"hit");


	}

	tm_topicmap_topic_scan_close(topicmap,scan);
	tm_topicmap_view_end_event(topicmap,"namelist");
	return TM_OK;
}

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




#if 0
TMError tm_view_sam_topic(struct view *vp, TMParams params)
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
	TMProperty prop_basenames;
	/*
	const char*attr, *query;
	*/

	int i = 0;
	void *address, *indicators;
	void *attr[100];

	const char*t_str;
	TMTopic topic;

	TMTopic basename_topic;
	struct a_membership pair;
	char pairbuf[100];
	TMTopicScan scan2,occ_scan;
	void *idata;
	void *basenames;

	if( (e=tmtk_default_model_lookup_function("org.gooseworks.SAM",&m)) != TM_OK)
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

	t_str = tm_params_get(params,"topic");
	/*
	query = tm_params_get(params,"query");
	*/
	if(!t_str)
	{
		tm_topicmap_set_error(vp->topicmap,"topic is a required parameter");
		return (TM_E_PARAM);
	}
	topic = atoi(t_str);
	/*
	if(!query)
	{
		tm_topicmap_set_error(vp->topicmap,"query is a required parameter");
		return (TM_E_PARAM);
	}
	*/


	tm_topicmap_get_property(vp->topicmap,topic,"org.gooseworks.SAM","SubjectIndicators",&indicators,NULL);
	tm_topicmap_get_property(vp->topicmap,topic,"org.gooseworks.SAM","SubjectAddress",&address,NULL);
	tm_topicmap_get_property(vp->topicmap,topic,"org.gooseworks.SAM","BaseNames",&basenames,NULL);

	/*
	attr[i++] = "basename";
	attr[i++] = (void*)idata;
	attr[i++] = &Text; 
	*/

	attr[i++] = "topic";
	attr[i++] = (void*)topic;
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
	if(basenames)
	{
		attr[i++] = "basenames";
		attr[i++] = basenames;
		attr[i++] = prop_basenames->value_type;
	}
	attr[i] = NULL;
	if(vp->start) vp->start(vp->user_data,"topic",attr);


	/* get basenames */
	/* -------------------------------------------------------------------- */
#if 0	
	pair.role = role_basenamed_topic;
	pair.player = topic;
	snprintf(pairbuf,sizeof(pairbuf),"(%d,%d)",
		role_basenamed_topic,topic);

	if( (e=tm_topicmap_topic_scan_open_with_double_condition(vp->topicmap,
		"org.gooseworks.IS13250","a-sidp","HAS_TYPE", (void*)at_basenamed_basename_topic,
		"AND",
		"org.gooseworks.IS13250","a-sidp","HAS_PLAYER_PAIR",
		pairbuf,
						&scan2)) != TM_OK)
	{
		return (e);
	}
	if(vp->start) vp->start(vp->user_data,"basenames",NULL);

	while(! tm_topicmap_topic_scan_finished(vp->topicmap,scan2) )
	{
		int i = 0;
		TMTopic a_topic,b_topic;
		void *asidp,*idata;
		void *attr[100];

		tm_topicmap_topic_scan_fetch(vp->topicmap,scan2,&a_topic);
		tm_topicmap_get_property(vp->topicmap,a_topic,"org.gooseworks.IS13250","a-sidp",&asidp,NULL);

		b_topic = (TMTopic)tm_value_call(prop_asidp->value_type,asidp,"getPlayer",(void*)role_basename_topic);
		tm_value_delete(prop_asidp->value_type,&asidp);
		assert(asidp == NULL);

		tm_topicmap_get_property(vp->topicmap,b_topic,"org.gooseworks.SAM","IndicatorData",&idata,NULL);


		attr[i++] = "string";
		attr[i++] = (void*)idata;
		attr[i++] = &Text; 

		attr[i++] = "topic";
		attr[i++] = (void*)b_topic;
		attr[i++] = &Topic; 
		attr[i] = NULL;

		if(vp->start) vp->start(vp->user_data,"basename",attr);
		/*	
		if(address)
			tm_value_delete(prop_sbjadr->value_type,&address);
		if(indicators)
			tm_value_delete(prop_sbjind->value_type,&indicators);
		*/


		if(vp->end) vp->end(vp->user_data,"basename");
	}
	/* FIXME: free name here??? */
	tm_topicmap_topic_scan_close(vp->topicmap,scan2);
	if(vp->end) vp->end(vp->user_data,"basenames");
#endif

	if(vp->start) vp->start(vp->user_data,"occurrence",NULL);

	if( (e=tm_topicmap_topic_scan_open_with_single_condition(vp->topicmap,
		"org.gooseworks.IS13250","a-sidp","HAS_TYPE",
		(void*)at_occurring_occurrence_topic,&occ_scan)) != TM_OK)
	{
		return (e);
	}

	while(! tm_topicmap_topic_scan_finished(vp->topicmap,occ_scan) )
	{
		TMTopic y;
		TMTopic o_topic;
		int i=0;
		void *attr[200];
		void *asidp,*data,*address,*basenames;

		tm_topicmap_topic_scan_fetch(vp->topicmap,occ_scan,&y);
		tm_topicmap_get_property(vp->topicmap,y,"org.gooseworks.IS13250","a-sidp",&asidp,NULL);
		
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

		tm_topicmap_get_property(vp->topicmap,o_topic,"org.gooseworks.SAM","SubjectData",&data,NULL);
		tm_topicmap_get_property(vp->topicmap,o_topic,"org.gooseworks.SAM","SubjectAddress",
									&address,NULL);
		tm_topicmap_get_property(vp->topicmap,o_topic,"org.gooseworks.SAM","BaseNames",
									&basenames,NULL);

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
			{ /* get first basename as title if any */
				attr[i++] = "title";
				attr[i++] = ((TMList)basenames)->content;
				attr[i++] = prop_sbjadr->value_type;
			}
		}
		attr[i] = NULL;


		if(vp->start) vp->start(vp->user_data,"occurrence",attr);
		if(data)
			tm_value_delete(prop_sbjdata->value_type,&data);
		if(address)
			tm_value_delete(prop_sbjadr->value_type,&address);
		if(basenames)
			tm_value_delete(prop_basenames->value_type,&basenames);

		if(vp->end) vp->end(vp->user_data,"occurrence");

	}

	tm_topicmap_topic_scan_close(vp->topicmap,occ_scan);
	if(vp->end) vp->end(vp->user_data,"occurrences");





	if(vp->end) vp->end(vp->user_data,"topic");


	return (TM_OK);
}
#endif



TMError tm_view_sam_tops(struct view *vp, TMParams params)
{
	TMError e;
	TMTopicScan scan;
	TMModel m,is13250;
	TMProperty prop_asidp;
	/*
	const char *model_name;
	*/
	const char *at_name;
	const char *yesrole_name,*norole_name;
	TMTopic t_at, t_yesrole, t_norole;

	TMTable visited;

	visited = tm_table_new(20,NULL,NULL);
	/*
	model_name = tm_params_get(params,"model_name");
	*/
	at_name = tm_params_get(params,"at_name");
	yesrole_name = tm_params_get(params,"yesrole");
	norole_name = tm_params_get(params,"norole");
	/*
	if(! model_name)
	{
		tm_topicmap_set_error(vp->topicmap,"model_name is a required parameter");
		return (TM_E_PARAM);
	}
	*/
	if(! at_name)
	{
		tm_topicmap_set_error(vp->topicmap,"at_name is a required parameter");
		return (TM_E_PARAM);
	}
	if(! yesrole_name)
	{
		tm_topicmap_set_error(vp->topicmap,"yesrole is a required parameter");
		return (TM_E_PARAM);
	}
	if(! norole_name)
	{
		tm_topicmap_set_error(vp->topicmap,"norole is a required parameter");
		return (TM_E_PARAM);
	}
	if( (e=tmtk_default_model_lookup_function(vp->tm,"org.gooseworks.IS13250",&is13250)) != TM_OK)
	{
		tm_topicmap_set_error(vp->topicmap,"unable to load model 'IS13250', %s",
			tmtk_get_error());
		return (e);
	}
	assert(is13250);

	GET_PROP(vp->topicmap,prop_asidp,is13250,"org.gooseworks.IS13250","a-sidp");	


	GET_TOPIC(vp->topicmap,t_at,"org.gooseworks.IS13250","UniqueName",at_name);
	GET_TOPIC(vp->topicmap,t_yesrole,"org.gooseworks.IS13250","UniqueName",yesrole_name);
	GET_TOPIC(vp->topicmap,t_norole,"org.gooseworks.IS13250","UniqueName",norole_name);

	if(vp->start)
		vp->start(vp->user_data,"tops",NULL);

	if( (e = tm_topicmap_topic_scan_open_with_single_condition(vp->topicmap,
		"org.gooseworks.IS13250","a-sidp","HAS_TYPE", (void*)t_at,&scan)) != TM_OK)
	{
		return (e);
	}


	while(! tm_topicmap_topic_scan_finished(vp->topicmap,scan) )
	{
		int i = 0;
		TMTopic t;
		void *attr[100];
		TMTopicScan scan2;
		TMTopic b_topic,t_topic;
		TMTopic t_yesrole_player;
		void *asidp,*idata,*address, *indicators;
		void *basenames;
		TMProperty prop;
		char pairbuf[256];
		int plays_norole = 0;

		tm_topicmap_topic_scan_fetch(vp->topicmap,scan,&t);
		tm_topicmap_get_property(vp->topicmap,t,"org.gooseworks.IS13250","a-sidp",&asidp,NULL);

		t_yesrole_player = (TMTopic)tm_value_call(prop_asidp->value_type,asidp,"getPlayer",(void*)t_yesrole);

		/* go on of we have that already. */
		if( tm_table_get(visited,(void*)t_yesrole_player) != NULL)
			continue;
		tm_table_put(visited,(void*)t_yesrole_player,(void*)1);

		snprintf(pairbuf,sizeof(pairbuf),"(%d,%d)",
			t_norole,t_yesrole_player);

		if( (e=tm_topicmap_topic_scan_open_with_double_condition(vp->topicmap,
			"org.gooseworks.IS13250","a-sidp","HAS_TYPE", (void*)t_at,
			"AND",
			"org.gooseworks.IS13250","a-sidp","HAS_PLAYER_PAIR", pairbuf,
						&scan2)) != TM_OK)
		{
			return (e);
		}

		if( ! tm_topicmap_topic_scan_finished(vp->topicmap,scan2))
			plays_norole = 1;

		tm_topicmap_topic_scan_close(vp->topicmap,scan2);

		if(plays_norole)
			continue;

		attr[i++] = "topic";
		attr[i++] = (void*)t_yesrole_player;
		attr[i++] = &Topic; 

		tm_topicmap_get_property(vp->topicmap,t_yesrole_player,"org.gooseworks.SAM","BaseNames",
									&basenames,&prop);
		attr[i++] = "names";
		attr[i++] = basenames;
		attr[i++] = prop->value_type; 

		attr[i] = NULL;

		if(vp->start)
			vp->start(vp->user_data,"top",attr);
		if(vp->end)
			vp->end(vp->user_data,"top");

	}

	tm_topicmap_topic_scan_close(vp->topicmap,scan);
	if(vp->end)
		vp->end(vp->user_data,"tops");

	tm_table_delete(&visited);
	return (TM_OK);
}
