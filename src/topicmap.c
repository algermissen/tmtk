/*
 * $Id$
 *
 * Copyright (c) 2002 Jan Algermissen
 * See the file "COPYING" for copying permission.
 *
 */

#include "tm.h"
#include "tmstack.h"
#include "tmmem.h"
#include "tmtrace.h"
#include "tmutil.h"
#include "tmparams.h"
#include "tmtable.h"
#include "tmtopicset.h"

#include "tmassertion.h"
#include "tmtopicmap.h"
#include "tmstorage.h"
#include "tmbasictypes.h"
#include "tmassert.h"

#include <ctype.h>

/* for bootsrapping (is loaded in open) */
extern struct TMModel coremodel;

/* conditional expression tree type used for topic scans */
#include "cetree.h"


/* local tracing stuff. Should propably not depend on NDEBUG - it is adding
 * overhead, even if tracing is not on for TM_TOPICMAP_TRACE (FIXME) */

#ifndef NDEBUG
#define TRACE_PROP_VALUE(prop,vtype,value) do { \
	char buf[11024]; \
	tm_value_to_string( (vtype), (value), buf,sizeof(buf)); \
	TMTRACE(TM_GRAPH_TRACE,"prop value: %s%s%s=%s\n" _  \
		(prop)->model _ TM_PROP_SEPERATOR _ (prop)->name _ buf); \
	} while(0) 

#define TRACE_VALUE(vtype,value) do { \
	char buf[11024]; \
	tm_value_to_string( (vtype), (value), buf,sizeof(buf)); \
	TMTRACE(TM_GRAPH_TRACE,"value: %s\n" _ buf); \
	} while(0) 
#else
#define TRACE_VALUE(prop,vtype,value) /* as nothing */
#define TRACE_PROP_VALUE(prop,vtype,value) /* as nothing */
#endif

#define DISPOSE_SIZE 200000   /* FIXME (all of the disposal) */

struct TMTopicMap {
	TM tm;
	TMTopicMapStorageDescriptor storage_descriptor;	
				/* this describes the storage system */
	void *storage_handle; 	/* generic data for storage system */
        TMList models;		/* currently loaded models */
 
        TMMetaStore meta_store; /* the store for meta data */
	TMList value_stores;	/* all value stores */
	TMList property_stores;	/* all property stores */
	TMTable propname_to_vstore_table; /* find store by prop's fullname */
	TMTable propname_to_pstore_table; /* find store by prop's fullname */

	TMStack transactions;	/* stack of transactions */

	TMList merge_lists;

	TMTopic disposed[DISPOSE_SIZE];
};

typedef struct Transaction *Transaction;
struct Transaction
{
	int foo;
};

struct TMTopicScan {
	int foo;
	TMList topics;
};

/* =========================================================================
 *
 *                             STATIC PROTOTYPES
 *                 (definitions are at the end of this file)
 *
 * ========================================================================= */

/* check if all property stores are normalized
 */
static int tm_topicmap_all_stores_normalized(TMTopicMap self);

/* Lookup the model in the list of loaded models.
 */
static TMModel tm_topicmap_get_model_by_name(TMTopicMap self,const char *name);

/* print a topic and all its properties
 */
/*
static void print_topic(TMTopicMap self,TMTopic i, FILE* file);
*/

static void _dispose_topic(TMTopicMap self,TMTopic t)
{
	TMList mp;
	TMValueStoreScan vscan;
	int opcode;
	assert(t < DISPOSE_SIZE);
	assert(self->disposed[t] == 0);
	self->disposed[t] = 1;

	for(mp=self->models;mp;mp=mp->next)
	{
		TMList pp;
		TMError e;
		TMModel m = (TMModel)mp->content;
		for(pp=m->properties; pp; pp=pp->next)
		{
			TMVid vid;
			TMPropertyStore pstore;
			TMValueStore vstore;
			TMProperty prop = (TMProperty)pp->content;

			pstore = tm_table_get(self->propname_to_pstore_table,prop->fullname);
			vstore = tm_table_get(self->propname_to_vstore_table,prop->fullname);
			assert(pstore); assert(vstore);

			e = tm_property_store_get_vid_of_topic(pstore,t,&vid);
			assert(e == TM_OK);
			assert(vid == 0);

			/* make sure topic is not in value store! */
			opcode = tm_valuetype_parse_opstring(prop->value_type,"VALUE_INCLUDES_TOPIC");
			if(opcode)
			{
				if( (e = tm_value_store_scan_open(vstore, opcode, (void*)t, &vscan ))
							 != TM_OK)
				{ assert(0); }

				assert( tm_value_store_scan_finished(vstore,vscan) );
				if( (e = tm_value_store_scan_close(vstore,vscan)) != TM_OK)
				{ assert(0); }
			}

		} 
	} 

}

/* lookup a topic by the value of an SIDP property.
 */
static TMError
_lookup_topic_by_sidp(TMTopicMap self,TMProperty prop,const void *value,TMTopic *np);


/* Lookup a property in the topicmap. Return NULL if not found.
 */
static TMProperty
_lookup_property(TMTopicMap self, const char *modelname, const char* propname);

static TMError _topicmap_create_topic(TMTopicMap self, TMTopic *np);

/* Load a model into the topicmap. */
static TMError _load_model(TMTopicMap self,TMModel model);


/*
static TMError _store_value(TMTopicMap self, TMValueStore vstore, const void *value, TMVid *vidp);
static TMError _add_assertion(TMTopicMap self, TMTopicAssertion astn);
static TMError _get_assertionstore(TMTopicMap self,TMTopic type_topic, TMAssertionStore *p);
static TMError _assertion_to_topic_assertion(TMTopicMap self, TMAssertion A, TMTopicAssertion a);
static TMError tm_topicmap_merge_topics(TMTopicMap self,TMTopic stays, TMTopic disappears);
*/


static TMError _pstore_get(TMTopicMap self,TMProperty prop,
	TMPropertyStore pstore, TMValueStore vstore, TMTopic topic,
	void **valp);

/* note, central internal adding function. Only called in add_property_to_topic() */
static TMError _pstore_put(TMTopicMap self,TMProperty prop, TMPropertyStore pstore, TMValueStore vstore,
			TMTopic topic, const void* value);
static TMError _pstore_vput(TMTopicMap self,TMProperty prop, TMPropertyStore pstore, TMValueStore vstore,
			const void* value, TMVid *pvid);
static void _set_contradicting_values_error(TMTopicMap self,TMProperty prop,
		TMValueStore vstore,TMVid vid1,TMVid vid2);
static void _set_contradicting_values_error_2(TMTopicMap self,TMProperty prop,
		const void*,const void*);

static TMError _pstore_replace_topics(TMTopicMap self,TMProperty prop,TMPropertyStore pstore,TMValueStore vstore,TMTopicSet set);
static TMError _pstore_replace_topics_in_values(TMTopicMap self,TMProperty prop,TMPropertyStore pstore,TMValueStore vstore,TMTopicSet set);

static TMError tm_topicmap_topic_scan_open(TMTopicMap self, TMCETree cetree, TMTopicScan *scanp);
static TMError tm_topicmap_cetree_eval(TMTopicMap self, TMCETree cetree, TMTopicSet set);



TMTopicMap tm_topicmap_new(TM tm)
{
	TMTopicMap self;
	TM_NEW(self);

	self->tm = tm;
	self->storage_descriptor = NULL;
	self->storage_handle     = NULL;
	self->models             = NULL;
	self->meta_store         = NULL;
	self->value_stores       = NULL;
	self->property_stores    = NULL;
	self->propname_to_vstore_table = tm_table_new(100,tm_strcmp_v,tm_strhash_v);
	self->propname_to_pstore_table = tm_table_new(100,tm_strcmp_v,tm_strhash_v);
	self->transactions       = tm_stack_new(0);
	self->merge_lists	 = NULL;

	bzero(self->disposed,sizeof(self->disposed));

	return self;
}

TMError tm_topicmap_set_storage(TMTopicMap self,const char *storage_name)
{
	assert(self);
	self->storage_descriptor = tm_lookup_storage_descriptor(self->tm,storage_name);
	if(! self->storage_descriptor)
		return TM_FAIL;

	return TM_OK;
}

void tm_topicmap_delete(TMTopicMap *pself)
{
	TMTopicMap self;
	assert(pself && *pself);
	self = *pself;

	tm_list_delete(&self->models);
	tm_table_delete(&self->propname_to_vstore_table);
	tm_table_delete(&self->propname_to_pstore_table);
	tm_stack_delete(&self->transactions);
	/* FIXME: valuestores and propertystores lists ? */
	/* fixme: merge lists TMList !! */
	TM_FREE(self);	
}


TMError tm_topicmap_open(TMTopicMap self,void* args)
{
	TMError e;
	TMList model_list,lp;	/* for loading the models */
	assert(self);

	TMTRACE(TM_GRAPH_TRACE,"enter\n");

	/* Handle storage arguments (e.g. path or DB access info). This
	 * might for example involve copying or converting to a table.
	 * Considering DB access, the connection would also be made there.
	 */
	if( (e = self->storage_descriptor->init_store(self->tm,args,&self->storage_handle)) != TM_OK)
	{
		/* error has been set in init_store() */
		return e;
	}

	self->meta_store = tm_meta_store_new(self->storage_descriptor,self->storage_handle);
	assert(self->meta_store);


	if( (e = tm_meta_store_open(self->meta_store)) != TM_OK)
	{
		/* FIXME  cleanup of all the above */
		return e;
	}

	/* load all basic props here */

	if( (e =  _load_model(self,&coremodel)) != TM_OK)
                return e;


	/* make function to load property */


	/* Now we need to load all the models that are known to
	 * be required for this topicmap.
	 */

	tm_meta_store_get_model_list(self->meta_store,&model_list);
	for(lp=model_list;lp;lp=lp->next)
	{
		TMTRACE(TM_GRAPH_TRACE,"from metastore: model: %s\n" _
					(const char*)lp->content);

		if( (e = tm_topicmap_require_model(self,(const char*)lp->content)) != TM_OK)
			return e;
	}
	tm_list_delete(&model_list);

	return TM_OK;
} 

TMError tm_topicmap_close(TMTopicMap self)
{
	TMList lp;
	assert(self);
	/* FIXME !! 
	assert(tm_stack_size(self->transactions) == 0);
	*/

	/* We are closing and deleting stores here, since if topicmap
	 * is closed, we want no store open any more. If no store is
	 * open we don't want them allocated.
	 * FIXME: what about reopening? */

	for(lp=self->value_stores;lp;lp=lp->next)
	{
		/*
		tm_value_store_print(lp->content,stderr);
		*/
		tm_value_store_close((TMValueStore)lp->content);	/* close the value store */
		tm_value_store_delete((TMValueStore)lp->content);
	}
	for(lp=self->property_stores;lp;lp=lp->next)
	{
		/*
		tm_property_store_print(lp->content,stderr);
		*/
		tm_property_store_close((TMPropertyStore)lp->content);	/* close the property store */
		tm_property_store_delete((TMPropertyStore)lp->content);
	}
	/* since topicmaps are deleted, lists are useless */
	tm_list_delete(&self->value_stores);
	tm_list_delete(&self->property_stores);
	/*
	tm_list_delete(&self->assertion_stores);
	*/
	
	tm_meta_store_close(self->meta_store);
	tm_meta_store_delete(self->meta_store);

	/* Do any required cleanup on the storage args, that might
	 * involve deallocation for example.
	 */
	self->storage_descriptor->cleanup_store(self->storage_handle); 

	return TM_OK;
}


TMError tm_topicmap_start_transaction(TMTopicMap self)
{
	Transaction t;
	assert(self);


	TM_NEW(t);

	tm_stack_push(self->transactions,t);


	return TM_OK;
}
TMError tm_topicmap_end_transaction(TMTopicMap self)
{
	Transaction t;
	assert(self);
#if 0
TMError e;
	if( (e = tm_topicmap_fully_merge(self)) != TM_OK)
	{
		/* FIXME: rollback here? */
		return e;
	}
#endif

	assert(tm_stack_size(self->transactions) > 0);

	t = (Transaction)tm_stack_pop(self->transactions);
	TM_FREE(t);

	return TM_OK;
}
TMError tm_topicmap_rollback_transaction(TMTopicMap self)
{
	Transaction t;
	assert(self);

	assert(tm_stack_size(self->transactions) > 0);

	t = (Transaction)tm_stack_pop(self->transactions);
	TM_FREE(t);

	return TM_OK;
}

TMError tm_topicmap_require_model(TMTopicMap  self,const char *name)
{
	TMError e;
	TMModel model;
	TMList rmp;
	assert(self);
	assert(name);

	TMTRACE(TM_GRAPH_TRACE,"enter, model=%s\n" _
		name);
	/* have it already? */
	if( tm_topicmap_get_model_by_name(self,name) != NULL)
		return TM_OK;

	if( (e = tm_topicmap_start_transaction(self)) != TM_OK)
		return e;

	if( (e = tm_disclosureframework_lookup_model(
		tm_get_disclosureframework(self->tm),self->tm,
		name,&model)) != TM_OK)
		return e;

	/* A Model may require other models, so we need to make
	 * sure they are present.
	 */

	for(rmp = model->require; rmp; rmp=rmp->next)
	{ 
		TMModel rm = (TMModel)rmp->content;
		TMTRACE(TM_GRAPH_TRACE,
			"%s requires %s\n" _
			model->name _ rm->name);
		if( (e = tm_topicmap_require_model(self,rm->name)) != TM_OK)
			return e;
	}

	if( (e =  _load_model(self,model)) != TM_OK)
		return e;

	/* make persistent in garph */
	if( (e=tm_meta_store_add_model(self->meta_store,model->name)) != TM_OK)
		return e;

	if( (e = tm_topicmap_end_transaction(self)) != TM_OK)
		return e;

	return TM_OK;
}


TMError tm_topicmap_fully_merge(TMTopicMap self)
{
	TMList mp,lp;
	int count = 0;
	TMError e;
	while( ! tm_topicmap_all_stores_normalized(self))
	{
		assert(self->merge_lists == NULL);

		/* normalize property stores => produce a list of merge_sets */
		for(lp=self->property_stores;lp;lp=lp->next)
		{
			TMPropertyStore pstore = (TMPropertyStore)lp->content;
			/* need to pass ourselves, because store needs to add mergelists to us! */
			e = tm_property_store_normalize(pstore,self);
			assert(e == TM_OK);	/*FIXME */
		}
		/* now we have a bunch of disjoint merge sets that need to be processed */

		/* substitute all topics in property stores */
		for(mp=self->models;mp;mp=mp->next)
		{
			TMList pp;
			TMError e;
			TMModel m = (TMModel)mp->content;
			TMTRACE(TM_GRAPH_TRACE,"replace topics loop for model %s\n" _ m->name);
			for(pp=m->properties; pp; pp=pp->next)
			{
				TMList kp;
				TMPropertyStore pstore;
				TMValueStore vstore;
				TMProperty prop = (TMProperty)pp->content;
	
				pstore = tm_table_get(self->propname_to_pstore_table,prop->fullname);
				vstore = tm_table_get(self->propname_to_vstore_table,prop->fullname);
				assert(pstore); assert(vstore);
				for(kp = self->merge_lists; kp; kp=kp->next)
				{
					if( (e = _pstore_replace_topics(self,prop,pstore,vstore,
							(TMTopicSet)kp->content)) != TM_OK)
						return e;
				} 
			} 
		} 
		/* fails - why?
		assert(tm_topicmap_all_stores_normalized(self));
		*/
			

		/* now al stores are normalized (except for topics in values!) */
		for(mp=self->models;mp;mp=mp->next)
		{
			TMList pp;
			TMError e;
			TMModel m = (TMModel)mp->content;
			TMTRACE(TM_GRAPH_TRACE,"replace topics in values loop for model %s\n" _ m->name);
			for(pp=m->properties; pp; pp=pp->next)
			{
				TMList kp;
				TMPropertyStore pstore;
				TMValueStore vstore;
				TMProperty prop = (TMProperty)pp->content;
	
				pstore = tm_table_get(self->propname_to_pstore_table,prop->fullname);
				vstore = tm_table_get(self->propname_to_vstore_table,prop->fullname);
				assert(pstore); assert(vstore);
				for(kp = self->merge_lists; kp; kp=kp->next)
				{
					if( (e = _pstore_replace_topics_in_values(self,prop,pstore,vstore,
							(TMTopicSet)kp->content)) != TM_OK)
						return e;
				} 
			} 
		} 
	
		/* now cleanup saved_merge_lists */
		for(lp = self->merge_lists; lp; lp=lp->next)
		{
			int len,i;
			TMTopicSet set = (TMTopicSet)lp->content;
			len = tm_topicset_size(set);
			/* first is 'stays', so we start at index 1 */
			for(i=1;i<len;i++)
			{
				TMTopic d = tm_topicset_get(set,i);
				_dispose_topic(self,d);
			}
			
			
			tm_topicset_delete(&set);
		}
		tm_list_delete(&(self->merge_lists));
		self->merge_lists = NULL; /* FIXME: free list and sets (is now done, or?)! */
	
		/* avoid endless loop in any case */
		if(++count > 200)
		{
			assert(0); /* make sure we catch this (impossible?) event during development! */
			break;
		}
	} /* while not all stores normalized */

	return TM_OK;
}



TMError _pstore_replace_topics(TMTopicMap self,TMProperty prop,TMPropertyStore pstore,TMValueStore vstore,TMTopicSet set)
{
	int i,len;
	TMTopic stays;
	TMError e;
	TMVid stays_vid,comb_vid;
	void *stays_val,*disap_val,*comb_val;

	TMTRACE(TM_GRAPH_TRACE,"enter [prop=%s]\n" _ prop->fullname);
	len = tm_topicset_size(set);
	stays = tm_topicset_get(set,0);
	TMTRACE(TM_GRAPH_TRACE,"MergeSet is [ ");
	for(i=0;i<len;i++)
	{
		TMTRACE_NOFUNC(TM_GRAPH_TRACE,"%d, " _ tm_topicset_get(set,i));
	}
	TMTRACE_NOFUNC(TM_GRAPH_TRACE," ]\n");
	/* get stays vid if stays is in store */
	if( (e = tm_property_store_get_vid_of_topic(pstore,stays,&stays_vid)) != TM_OK)
		return e;

	for(i=1;i<len;i++)
	{
		TMTopic disap = tm_topicset_get(set,i);
		TMVid disap_vid;
		if( (e = tm_property_store_get_vid_of_topic(pstore,disap,&disap_vid)) != TM_OK)
			return e;
		if( !disap_vid)
			continue;

		/* if stays not in store, simply store the vid for stays */	
		if( !stays_vid)
		{
			if( (e = tm_property_store_insert(pstore,stays,disap_vid)) != TM_OK)
				return e;
			if( (e = tm_property_store_remove(pstore,disap)) != TM_OK)
				return e;
			stays_vid = disap_vid;
			continue;
		}
		if(stays_vid == disap_vid)
		{
			if( (e = tm_property_store_remove(pstore,disap)) != TM_OK)
				return e;
			/* do nothing else */
			continue;
		}

		TMTRACE(TM_GRAPH_TRACE, "stays has other vid as disap -> combine\n");
		if(! prop->combination_code)
		{
			TMTRACE(TM_GRAPH_TRACE, "no combination code in prop\n");

			if(! tm_valuetype_parse_opstring(prop->value_type,"VALUE_INCLUDES_TOPIC"))
			{
				/*
				_set_contradicting_values_error_2(self,prop,value,exist_val);
				*/
				_set_contradicting_values_error(self,prop,vstore,stays_vid,disap_vid);
				return TM_ECONTRADICTING_PROPERTY_VALUES;
			}
			/* if values may contain topics, we assume error is due to
			 * unknown future equivalence of topics and just proceed.
			 * FIXME: this can be made better by providing an equality check
			 * by the vtype that explcitly checks for topic-inequality
 			 * situations.
			 */
			/*
			print_topic(self,stays,stderr);
			print_topic(self,disap,stderr);
			getchar();
			*/
			/*
			*/
			if( (e = tm_property_store_remove(pstore,disap)) != TM_OK)
				return e;
			/* do nothing else */
			continue;
		}

		if( (e = tm_value_store_get_value(vstore,stays_vid,&stays_val)) != TM_OK)
			return e;
		if( (e = tm_value_store_get_value(vstore,disap_vid,&disap_val)) != TM_OK)
			return e;

		TMTRACE(TM_GRAPH_TRACE, "stays value:\n");
		TMTRACE_VALUE(TM_GRAPH_TRACE,prop->value_type,stays_val);
		TMTRACE(TM_GRAPH_TRACE, "disap value:\n");
		TMTRACE_VALUE(TM_GRAPH_TRACE,prop->value_type,disap_val);
		TMTRACE(TM_GRAPH_TRACE, "now combining values and storing result as prop val\n");
		comb_val = tm_value_call(prop->value_type,stays_val,prop->combination_code,disap_val);
		TMTRACE(TM_GRAPH_TRACE, "comb value:\n");
		TMTRACE_VALUE(TM_GRAPH_TRACE,prop->value_type,comb_val);


		/* remove since we re-store it with combined property */
		if( (e = tm_property_store_remove(pstore,stays)) != TM_OK)
			return e;
		if( (e = tm_property_store_remove(pstore,disap)) != TM_OK)
			return e;

		if( (e = _pstore_vput(self,prop,pstore,vstore,comb_val,&comb_vid)) != TM_OK)
			return e;
		if( (e = tm_property_store_insert(pstore,stays,comb_vid)) != TM_OK)
			return e;
		stays_vid = comb_vid;
		/* FIXME yes or no?
		tm_value_delete(prop->value_type,&exist_val);
		tm_value_delete(prop->value_type,&comb_val);
		*/
	}
	TMTRACE(TM_GRAPH_TRACE,"exit\n");
	return TM_OK;
}

TMError _pstore_replace_topics_in_values(TMTopicMap self,TMProperty prop,TMPropertyStore pstore,TMValueStore vstore,TMTopicSet set)
{
	int i,len;
	TMTopic stays;
	TMError e;
	TMValueStoreScan vscan;
	TMList vp,vids = NULL;
	int opcode;

	TMTRACE(TM_GRAPH_TRACE,"enter for prop=%s" _ prop->fullname);

	opcode = tm_valuetype_parse_opstring(prop->value_type,"VALUE_INCLUDES_TOPIC");
	if(!opcode)
		return TM_OK;

	len = tm_topicset_size(set);
	stays = tm_topicset_get(set,0);
	TMTRACE(TM_GRAPH_TRACE,"MergeSet is [ ");
	for(i=0;i<len;i++)
	{
		TMTRACE_NOFUNC(TM_GRAPH_TRACE,"%d, " _ tm_topicset_get(set,i));
	}
	TMTRACE_NOFUNC(TM_GRAPH_TRACE," ]\n");

	/* loop starts at 1 since we omit stays */	
	for(i=1;i<len;i++)
	{
		TMTopic t;
		t = tm_topicset_get(set,i);
		if( (e = tm_value_store_scan_open(vstore, opcode, (void*)t, &vscan )) != TM_OK)
			return e;
		while(! tm_value_store_scan_finished(vstore,vscan) )
		{
			TMVid vid;
			TMList lp;
			int found = 0;
			e = tm_value_store_scan_fetch(vstore,vscan,&vid);
			assert(e == TM_OK);  /* FIXME */

			/* can we have doubles or is having doubles a programming error? */
			/* FIXME: use if_contains function */
			for(lp=vids;lp;lp=lp->next)
			{
				if( (TMVid)lp->content == vid)
				{
					found = 1;
					break;
				}
			}
			if(!found)
			{
				/* get value, substitute, remove old and vput new one etc. */
				/* later on, see if we can have a merge or if w just have new topic lists (just that store isn;t normalized anymore!...*/
				vids = tm_list_push(vids,(void*)vid);
			}
		} /* while vscan has more */
		if( (e = tm_value_store_scan_close(vstore,vscan)) != TM_OK)
			return e;
	} /* for each topic in merge set */
	for(vp=vids;vp;vp=vp->next)
	{
		TMTopic *topics;
		TMList lp,saved_topics = NULL;  /* we must not alter store while using 'topics' */
		int i,len;
		int changed;
		void *v;
		TMVid new_vid;
		TMVid vid = (TMVid)vp->content;
		TMTRACE(TM_GRAPH_TRACE,"vid=%d has one of mergeset\n" _
			vid);
		if( (e = tm_value_store_get_value(vstore,vid,&v)) != TM_OK)
			return e;
		TMTRACE_VALUE(TM_GRAPH_TRACE,prop->value_type,v);
		changed = (int)tm_value_call(prop->value_type , v , "replaceTopics" ,set);
		assert(changed);
		TMTRACE(TM_GRAPH_TRACE,"New value (with topics replaced)\n");
		TMTRACE_VALUE(TM_GRAPH_TRACE,prop->value_type,v);

		if( (e = _pstore_vput(self,prop,pstore,vstore,v,&new_vid)) != TM_OK)
			return e;	

		TMTRACE(TM_GRAPH_TRACE,"Value with replaced topics has new vid=%d (old was %d)\n" _ new_vid _ vid);

		if( (e = tm_property_store_lookup_topics(pstore,vid,&topics,&len)) != TM_OK)
			return e;
		for(i=0;i<len;i++)
			saved_topics = tm_list_push(saved_topics,(void*)topics[i]);

		for(lp=saved_topics;lp;lp=lp->next)
		{
			if( (e = tm_property_store_remove(pstore,(TMTopic)lp->content)) != TM_OK)
				return e;
		}
		for(lp=saved_topics;lp;lp=lp->next)
		{
			if( (e = tm_property_store_insert(pstore,(TMTopic)lp->content,new_vid)) != TM_OK)
				return e;
		}


			
	}
	tm_list_delete(&vids);
	return TM_OK;
}










/* revised 1. Apr 2003 */
TMError tm_topicmap_xml_export(TMTopicMap self, FILE *f)
{
#if 0
	TMTopic topicmax,topic;
	TMList lp;
	assert(self);
	assert(f);

	fprintf(f,"<?xml version=\"1.0\" encoding=\"UTF-8\">\n");
	fprintf(f,"<TopicMap>\n");
	fprintf(f,"<Models>\n");
	for(lp=self->models;lp;lp=lp->next)
	{
		fprintf(f,"  <Name>%s</Name>\n", ((TMModel)lp->content)->name );
	}
	fprintf(f,"</Models>\n");

	tm_meta_store_get_topicmax(self->meta_store,&topicmax) ;

	for(topic = 1; topic <= topicmax; topic++)
	{
		TMList lp;
		fprintf(f,"<Topic id=\"%d\">\n",topic);
		for(lp=self->property_stores;lp;lp=lp->next)
		{
			const void *value;
			TMError e;
			char buf[132000];
			TMVid vid = 0;
			TMValueStore vstore;
			TMValueType vtype;
			TMProperty prop;
			TMPropertyStore pstore = (TMPropertyStore)lp->content;
			/*
			prop = tm_property_store_get_property(pstore);
			*/
       			vtype = prop->value_type;
			vstore = (TMValueStore)tm_table_get(
				self->propname_to_vstore_table,prop->name);
			assert(vstore); /* if prop is there,store must too */
			
			if( (e = tm_property_store_lookup_vid(pstore,topic,
					&vid)) != TM_OK)
				return e;
			if(!vid)
				continue;

			tm_value_store_get_value(vstore,vid,&value);
			tm_value_to_xml(vtype,value, buf,sizeof(buf));
			fprintf(f,
				"  <property model=\"%s\" name=\"%s\" "
				"valueType=\"%s\" vid=\"%d\">\n%s\n  </property>\n",
				prop->model,prop->name,vtype->name,vid,buf);
			tm_value_delete( vtype, &value);
		}
		fprintf(f,"</Topic>\n");
	}
	/* This must be done with the TMM defined props */		
	/*
	for(lp=self->assertion_stores;lp;lp=lp->next)
		tm_assertion_store_print(lp->content,f);
	*/
	fprintf(f,"</TopicMap>\n");
#endif
	return TM_OK;

}


/* NEW, ok 1.8.2003 */
/* Note: this is the central function for adding stuff to the map,
 * add_subject() and add_topic_assertion() both call it internally */
TMError tm_topicmap_add_property_to_topic_from_string(TMTopicMap self, TMTopic topic,
	const char *modelname, const char *propname, const char *value_string)
{
	TMError e;
	TMModel model;
	TMProperty prop;
	TMPropertyStore pstore;
	TMValueStore vstore;
	void *value;
	/*
	TMVid vid;
	*/

	assert(self);
	assert(topic);
	assert(modelname);
	assert(propname);
	assert(value_string);

	TMTRACE(TM_GRAPH_TRACE, "enter model=%s prop=%s topic=%d\n" _ modelname _ propname _ topic);
	if( (model = tm_topicmap_get_model_by_name(self,modelname)) == NULL) 
	{
		tm_set_error(self->tm,
		"unable to process property (model not loaded) %s %s", modelname,propname);
       		return TM_EMODEL_NOT_LOADED;
	}
	
       	if( (prop = tm_model_get_property(model,NULL,propname)) == NULL)
	{
		tm_set_error(self->tm,
		"property %s not found in model %s", propname,modelname);
		return TM_E_PROP_NOT_FOUND;
	}


	TMTRACE(TM_GRAPH_TRACE, "value string: _%s_" _ value_string);
	value = tm_value_new_from_string(prop->value_type,value_string);
	assert(value);


	TMTRACE(TM_GRAPH_TRACE, "value: ");
	TMTRACE_VALUE(TM_GRAPH_TRACE,prop->value_type,value);

	pstore = tm_table_get(self->propname_to_pstore_table,prop->fullname);
	vstore = tm_table_get(self->propname_to_vstore_table,prop->fullname);
	assert(pstore); /* if property is there, stores must be there too */
	assert(vstore);

	/* this is the only place where pstore_put is called !! */
	if( (e = _pstore_put(self,prop, pstore,vstore,topic,value)) != TM_OK)
	{
		return e;
	}

/*




*/


	tm_value_delete(prop->value_type,&value);


	TMTRACE(TM_GRAPH_TRACE, "exit\n");
	return TM_OK;
}


/* NEW, ok 1.8.2003 */
/* Note: this is the central function for adding stuff to the map,
 * add_subject() and add_topic_assertion() both call it internally */
TMError tm_topicmap_add_property_to_topic(TMTopicMap self, TMTopic topic,
	const char *modelname, const char *propname, const void *value)
{
	TMError e;
	TMModel model;
	TMProperty prop;
	TMPropertyStore pstore;
	TMValueStore vstore;
	/*
	TMVid vid;
	*/

	assert(self);
	assert(topic);
	assert(modelname);
	assert(propname);
	assert(value);

	TMTRACE(TM_GRAPH_TRACE, "enter model=%s prop=%s topic=%d\n" _ modelname _ propname _ topic);
	if( (model = tm_topicmap_get_model_by_name(self,modelname)) == NULL) 
	{
		tm_set_error(self->tm,
		"unable to process property (model not loaded) %s %s", modelname,propname);
       		return TM_EMODEL_NOT_LOADED;
	}
	
       	if( (prop = tm_model_get_property(model,NULL,propname)) == NULL)
	{
		tm_set_error(self->tm,
		"property %s not found in model %s", propname,modelname);
		return TM_E_PROP_NOT_FOUND;
	}
	TMTRACE(TM_GRAPH_TRACE, "value: ");
	TMTRACE_VALUE(TM_GRAPH_TRACE,prop->value_type,value);

	pstore = tm_table_get(self->propname_to_pstore_table,prop->fullname);
	vstore = tm_table_get(self->propname_to_vstore_table,prop->fullname);
	assert(pstore); /* if property is there, stores must be there too */
	assert(vstore);

	/* this is the only place where pstore_put is called !! */
	if( (e = _pstore_put(self,prop, pstore,vstore,topic,value)) != TM_OK)
	{
		return e;
	}

/*




*/





	TMTRACE(TM_GRAPH_TRACE, "exit\n");
	return TM_OK;
}
/* NEW */
TMError tm_topicmap_get_property(TMTopicMap self,
			      TMTopic topic,
			      const char *modelname,
			      const char *propname,
			      void **valuep,
			      TMProperty *pp)
{
	TMError e;
	TMModel model;
	TMProperty prop;
	TMPropertyStore pstore;
	TMValueStore vstore;
	const char corename[] = "RM-Core";

	assert(self);
	assert(topic);
	assert(propname);
	assert(valuep);

	if(!modelname || !*modelname)
	{
		modelname = corename;
	}

	if(pp)
		*pp = NULL;

	TMTRACE(TM_GRAPH_TRACE, "enter model=%s prop=%s\n" _ modelname _ propname);
	if( (model = tm_topicmap_get_model_by_name(self,modelname)) == NULL) 
	{
		tm_set_error(self->tm,
		"unable to process property %s %s", modelname,propname);
       		return TM_EMODEL_NOT_LOADED;
	}
	
       	if( (prop = tm_model_get_property(model,NULL,propname)) == NULL)
	{
		assert(0); /* (FIXME: take out) assert assures we stop! */
		tm_set_error(self->tm,
		"property %s not found in model %s", propname,modelname);
		return TM_E_PROP_NOT_FOUND;
	}
	if(pp)
		*pp = prop;

	pstore = tm_table_get(self->propname_to_pstore_table,prop->fullname);
	vstore = tm_table_get(self->propname_to_vstore_table,prop->fullname);
	assert(pstore); /* if property is there, stores must be there too */
	assert(vstore);

	if( (e = _pstore_get(self,prop, pstore,vstore,topic,valuep)) != TM_OK)
	{
		return e;
	}

	TMTRACE(TM_GRAPH_TRACE, "exit\n");
	return TM_OK;
}


/*-----------------------------------------------------------------------
 *
 *
 *----------------------------------------------------------------------- */

/* NEW, ok 02 Sep 2003 */

TMError _pstore_get(TMTopicMap self,TMProperty prop, TMPropertyStore pstore, TMValueStore vstore,
			TMTopic topic, void **valp)
{
	TMVid vid;
	TMError e;
	*valp = NULL;
	if( (e = tm_property_store_get_vid_of_topic(pstore,topic,&vid)) != TM_OK)
		return e;
	if(!vid)
		return TM_OK;

	if( (e = tm_value_store_get_value(vstore,vid,valp)) != TM_OK)
		return e;

	return TM_OK;
}

/* NEW */
TMError _pstore_put(TMTopicMap self,TMProperty prop, TMPropertyStore pstore, TMValueStore vstore,
			TMTopic topic, const void* value)
{
	TMVid vid;
	TMVid exist_vid;
	TMError e;
	void *exist_val;
	void *comb_val;
	TMVid comb_vid;
	TMTRACE(TM_GRAPH_TRACE, "enter\n");

	/* we used to vput here, but that does not work,because in case exist_vid,
	 * vputted value is hard to remove again! */

	if( (e = tm_property_store_get_vid_of_topic(pstore,topic,&exist_vid)) != TM_OK)
		return e;
	if(! exist_vid)
	{
		/* topic not yet in store */
		if( (e = _pstore_vput(self,prop,pstore,vstore,value,&vid)) != TM_OK)
			return e;
		if( (e = tm_property_store_insert(pstore,topic,vid)) != TM_OK)
			return e;
	
		TMTRACE(TM_GRAPH_TRACE, "exit\n");
		return TM_OK;	
	}

	/* topic is already in store */
	TMTRACE(TM_GRAPH_TRACE, "topic has already a value for this property\n");

	if( (e = tm_value_store_get_value(vstore,exist_vid,&exist_val)) != TM_OK)
		return e;
	TMTRACE(TM_GRAPH_TRACE, "existing value:\n");
	TMTRACE_VALUE(TM_GRAPH_TRACE,prop->value_type,exist_val);


	/* same prop value assigned twice? */
	/* FIXME: maybe a vstore can later on implement equality check between value and
	 * a vid to avoid fetching the exist_val?
	 */
	if(tm_value_equal(prop->value_type,exist_val,value))
	{
		TMTRACE(TM_GRAPH_TRACE, "same value assigned twice (that is ok)\n");
		tm_value_delete(prop->value_type,&exist_val);
		return TM_OK;
	}
	TMTRACE(TM_GRAPH_TRACE, "values not equal, need to combine them\n");
	if(! prop->combination_code)
	{
		TMTRACE(TM_GRAPH_TRACE, "no combination code in prop -> error\n");
		_set_contradicting_values_error_2(self,prop,value,exist_val);
		/*
		_set_contradicting_values_error(self,prop,vstore,vid,exist_vid);
		*/
		return TM_ECONTRADICTING_PROPERTY_VALUES;
	}
	TMTRACE(TM_GRAPH_TRACE, "now combining values and storing result as prop val\n");
	/* FIXME (here could/should? be real code execution! */
	comb_val = tm_value_call(prop->value_type,exist_val,prop->combination_code,value);


	/*****************************************************
	 * FIXME: just some thoughts to improve speed!
	 * -- this is how execution in value store might look like
	 if( (e = tm_value_store_execute_combination_code(vstore,prop->combination_code,
	 	vid,existing_vid,&comb_val)) != TM_OK)
		return e;
	if(! comb_val)
	{
		_set_contradicting_values_error(self,prop,vstore,vid,exist_vid);
		return TM_ECONTRADICTING_PROPERTY_VALUES;
	}

	HAUPTFRAGE: was ist mit dem loesechen der anderen values....und so weiter
	--> ahh, the follwoing vput will handle that (partly)

	WICHTIG sind alle delets die ich machen muss!!! siehe Pyton code!!
	 ***************************************************************/

	/* remove since we re-store it with combined property */
	if( (e = tm_property_store_remove(pstore,topic)) != TM_OK)
		return e;

	if( (e = _pstore_vput(self,prop,pstore,vstore,comb_val,&comb_vid)) != TM_OK)
		return e;
	if( (e = tm_property_store_insert(pstore,topic,comb_vid)) != TM_OK)
		return e;
	/* FIXME yes or no?
	tm_value_delete(prop->value_type,&exist_val);
	tm_value_delete(prop->value_type,&comb_val);
	*/

	TMTRACE(TM_GRAPH_TRACE, "exit\n");
	return TM_OK;
}
/*
 * This function stores a value in the value store that 'belongs' to a
 * certain property store. The insertion process also deals with a
 * property defined equality definition if present.
*/
TMError _pstore_vput(TMTopicMap self,TMProperty prop, TMPropertyStore pstore, TMValueStore vstore,
			const void* value, TMVid *pvid)
{
	TMError e;
	TMList collected_topics = NULL, lp;
	TMList vids = NULL;
	int opcode;
	void *combined_value, *tmp;
	TMValueStoreScan vscan;

	TMTRACE(TM_GRAPH_TRACE, "enter\n");
	TMTRACE_VALUE(TM_GRAPH_TRACE,prop->value_type,value);

	if( ! prop->equality_code ) 
	{
		/* property does not contrain its range (it does not define
		 * an equality function beyond value type equality). Thus
		 * we can simply store the value.
		 */
		TMTRACE(TM_GRAPH_TRACE,"prop does not define equality code\n");
		if( (e = tm_value_store_lookup_vid(vstore,value,
				TM_VALUE_LOOKUP_OR_CREATE,pvid)) != TM_OK)
		{
			assert(0);
			return e;
		}
#ifndef NDEBUG
		{ /* FIXME: debug code (adds one second to a 10 second parse of opera) */
		TMVid i;
		if( (e = tm_value_store_lookup_vid(vstore,value,
				TM_VALUE_LOOKUP,&i)) != TM_OK)
		assert(i == *pvid);
		}
#endif
		TMTRACE(TM_GRAPH_TRACE, "exit\n");
		return TM_OK;
	}
	TMTRACE(TM_GRAPH_TRACE, "property defines equality code\n");
	TMTRACE(TM_GRAPH_TRACE, "getting 'all values that the prop considers "
				"to be equal to vputted value'\n");


	/* get opcode from operator string in TMA definition */	
	opcode = tm_valuetype_parse_opstring(prop->value_type, prop->equality_code);
	if(!opcode)
	{
		tm_set_error(self->tm,
			"operator '%s' not defined for valuetype '%s'",
			prop->equality_code, prop->value_type->name);
		return TM_E_OP_UNDEFINED;
	}
	/* no argstring parsing needed, arg is value (Vq) */



	if( (e = tm_value_store_scan_open(vstore, opcode, value , &vscan )) != TM_OK)
		return e;
	while(! tm_value_store_scan_finished(vstore,vscan) )
	{
		TMVid vid;
		TMList lp;
		int found = 0;
		tm_value_store_scan_fetch(vstore,vscan,&vid);

		/* FIXME: can we have doubles or is having doubles a programming error? */
		/* FIXME: use if_contains function */
		for(lp=vids;lp;lp=lp->next)
		{
			if( (TMVid)lp->content == vid)
			{
				found = 1;
				break;
			}
		}
		if(!found)
		{
			vids = tm_list_push(vids,(void*)vid);
		}
	}
	if( (e = tm_value_store_scan_close(vstore,vscan)) != TM_OK)
		return e;
	
	combined_value = (void*)value;  /* FIXME: const -> nonconst cast! */

	TMTRACE(TM_GRAPH_TRACE, "start value (before combining things):\n" );
	TMTRACE_VALUE(TM_GRAPH_TRACE,prop->value_type,combined_value);
	TMTRACE(TM_GRAPH_TRACE, "iterating over vids and merging values\n" );

	/* special code for handling common case that only on equal value is
	 * found in scan and that thus we can handle this exactly like the
	 * ordinary insert!   (added 16 Nov 2003)
	 */
	if(vids && vids->next == NULL)
	{
		void *v;
		TMVid vid;
		TMTRACE(TM_GRAPH_TRACE,"special case (only one vid found in vscan)\n");
		vid = (TMVid)vids->content;
		if( (e = tm_value_store_get_value(vstore,vid,&v)) != TM_OK)
			return e;
		TMTRACE(TM_GRAPH_TRACE, "got value\n");
		TMTRACE_VALUE(TM_GRAPH_TRACE,prop->value_type,v);
		if(tm_value_equal(prop->value_type,value,v))
		{
			/* value equal => handle like simple insert. */
			tm_value_delete(prop->value_type,&v);
			*pvid = vid;
			return TM_OK;
		}
		tm_value_delete(prop->value_type,&v);
		/* values are not equal, so we need to do all the combination stuff
		 * anyway now. Simply do now, what we would do in the nen-special case
		 * anyway.
		 */
	}
	/* end special code */

	for(lp=vids;lp;lp=lp->next)
	{
		int i,len;
		TMTopic *topics;
		TMList kp;
		TMList my_collected_topics = NULL;
		void *v;
		TMVid vid;
		TMTRACE(TM_GRAPH_TRACE,"loop vids\n");
		vid = (TMVid)lp->content;
		TMTRACE(TM_GRAPH_TRACE, "in loop over vids to combine vid=%d\n" _ vid);
		if( (e = tm_value_store_get_value(vstore,vid,&v)) != TM_OK)
			return e;
		TMTRACE(TM_GRAPH_TRACE, "got value\n");
		TMTRACE_VALUE(TM_GRAPH_TRACE,prop->value_type,v);

		if( (e = tm_property_store_lookup_topics(pstore,vid,&topics,&len)) != TM_OK)
			return e;
		TMTRACE(TM_GRAPH_TRACE, "got topics\n");
		for(i=0;i<len;i++)
		{
			TMList kp;
			int found = 0;
			TMTRACE(TM_GRAPH_TRACE, "   .... %d (i=%d)\n" _ topics[i] _ i);
			/* FIXME! shouldn't collected_topics be disjoint by nature?! */
			for(kp=collected_topics;kp;kp=kp->next)
			{
				if( (TMTopic)kp->content == topics[i])
				{
					found = 1;
					break;
				}
			}
			if(!found)
			{
				collected_topics = tm_list_push(collected_topics,(void*)topics[i]);
			}

			for(kp=my_collected_topics;kp;kp=kp->next)
			{
				if( (TMTopic)kp->content == topics[i])
				{
					found = 1;
					break;
				}
			}
			if(!found)
			{
				my_collected_topics = tm_list_push(my_collected_topics,(void*)topics[i]);
			}
		}
		
		TMTRACE(TM_GRAPH_TRACE, "now removing all the topics\n");
		for(kp=my_collected_topics;kp;kp=kp->next)
		{
			TMTRACE(TM_GRAPH_TRACE, " .... %d\n" _ (TMTopic)kp->content );
			if( (e = tm_property_store_remove(pstore,(TMTopic)kp->content) ) != TM_OK)
				return e;
		}
		TMTRACE(TM_GRAPH_TRACE, "removed all the topics\n");
		tm_list_delete(&my_collected_topics);



		tmp = tm_value_call(prop->value_type,combined_value,prop->combination_code,v);
		/* ---- FIXME: does that break?
		tm_value_delete(prop->value_type,&combined_value);
		tm_value_delete(prop->value_type,&v);
		*/
		combined_value = tmp;
		TMTRACE(TM_GRAPH_TRACE, "combined value now:\n");
		TMTRACE_VALUE(TM_GRAPH_TRACE,prop->value_type,combined_value);
		TMTRACE(TM_GRAPH_TRACE, "MARK 1\n");
	}

	TMTRACE(TM_GRAPH_TRACE, "storing combined value\n" );
#ifndef NDEBUG
	{  /* check that value is really not in store (so use of TM_VALUE_CREATE is ok) */
	TMVid i;
	if( (e = tm_value_store_lookup_vid(vstore,combined_value,
			TM_VALUE_LOOKUP,&i)) != TM_OK)
	assert(i == 0);   /* value MUST not be inm store */

	}
#endif
	/* here we use explicit create-only (FIXME: is that correct? - see tmm_pro) */
	if( (e = tm_value_store_lookup_vid(vstore,combined_value,TM_VALUE_CREATE,pvid)) != TM_OK)
		return e;
#ifndef NDEBUG
	{ /* FIXME: debug code */
	TMVid i;
	if( (e = tm_value_store_lookup_vid(vstore,combined_value,
			TM_VALUE_LOOKUP,&i)) != TM_OK)
	assert(i == *pvid);
	}
#endif
	TMTRACE(TM_GRAPH_TRACE, "stored combined value as vid %d\n" _ *pvid);
	for(lp=collected_topics;lp;lp=lp->next)
	{
		if( (e = tm_property_store_insert(pstore,(TMTopic)lp->content,*pvid)) != TM_OK)
			return e;
		TMTRACE(TM_GRAPH_TRACE, "re-stored topic %d with this vid\n" _ (TMTopic)lp->content);
	}

	tm_list_delete(&collected_topics);
	tm_list_delete(&vids);


	TMTRACE(TM_GRAPH_TRACE, "exit\n");
	return TM_OK;
}
/* what if scan runs more props?
 *
 SELECT
  WHERE SAM::SubectData MATCHES "water AND purification"
    AND DC::Author = "Sam Hunting"



 *
 *
 *
 *
 *
 *
 */

/* NEW */
/* FIXME: is not used... why? */
TMError _pstore_normalize(TMTopicMap self,TMProperty prop, TMPropertyStore pstore, TMValueStore vstore,
			TMTopic topic, const void* value)
{
	TMError e;

	if(prop->type != TM_SIDP)
		return TM_OK;

	/* redundant */
	if(tm_property_store_is_normalized(pstore))
		return TM_OK;

	if( (e = tm_property_store_normalize(pstore,self)) != TM_OK)
		return e;

	return TM_OK;
	
}


/* --------------------- END pstore local methods ----------------------*/



/* NEW */
TMError _topicmap_create_topic(TMTopicMap self, TMTopic *np)
{
	TMError e;
	assert(self);
	assert(np);
	
	TMTRACE(TM_GRAPH_TRACE, "enter\n");

	if( (e = tm_meta_store_get_new_topic(self->meta_store,np)) != TM_OK)
		return e;
	TMTRACE(TM_GRAPH_TRACE,
		"got new topic ID %d\n" _ *np);

	TM_LOG(self->tm,TM_LOG_DEBUG,"CREATED topic (%d)" _ *np);

	TMTRACE(TM_GRAPH_TRACE, "exit\n");
	return TM_OK;
}

/*
 * Find the topic that has the same subject as specified by 'subject'.
 * OPs will be ignored, no storage will be made of any propertes in
 * subject. so, what about subset etc..[FIXME]?
 *
 */
TMError tm_topicmap_lookup_topic_by_subject(TMTopicMap self, TMSubject subject,
	TMTopic *np)
{
	TMError e;
	int i;
	assert(self);
	assert(subject);
	assert(np);
	assert(0);

	TMTRACE(TM_GRAPH_TRACE, "enter\n");
	*np = 0;

        for(i=0; i<subject->N;i++)
        {
		TMProperty prop;

		prop = _lookup_property(self,subject->props[i].model,
				subject->props[i].name);
		if (!prop)
		{
			tm_set_error(self->tm,
			"property %s%s%s not found",
			subject->props[i].model,
			TM_PROP_SEPERATOR,
			subject->props[i].name);
			/* FIXME: we might just continue here */
			return TM_OK;
		}

		/* lookup can only be done by SIDPs*/
		/* FIXME: is that so ? ... NO, there can be multiple topics !!! so we need two func...*/
		if(prop->type != TM_SIDP)
			continue;

		/* FIXME: also, there could be several topics for this operation if we
		 * process all SIDPs...? Does that matter?
		 */

		assert(subject->props[i].value);

		TMTRACE(TM_GRAPH_TRACE, "now looking up by this SIDP (%s)\n" _ prop->name );
		if( (e = _lookup_topic_by_sidp(self,prop,
			subject->props[i].value,np)) != TM_OK)
			return e;

		if(*np)
		{
			/* one SIDP match is enough for the lookup !*/
			break;
		}
	}


	TMTRACE(TM_GRAPH_TRACE, "exit\n");
	return TM_OK;
}

/* revised 07 Mar 2003 */
TMError tm_topicmap_lookup_topic_by_property(TMTopicMap self, const char *modelname,
			const char *propname, const void *value, TMTopic *np)
{
	TMError e;
	TMProperty prop;
	assert(self);
	assert(modelname);
	assert(propname);
	assert(value);
	assert(np);
	assert(0);

	*np = TM_NO_NODE;
	
	TMTRACE(TM_GRAPH_TRACE, "enter\n");

	prop = _lookup_property(self,modelname,propname );
	if (!prop)
		return TM_OK;

	if(prop->type != TM_SIDP)
		return TM_E_SIDP_EXPECTED;

	if( (e = _lookup_topic_by_sidp(self,prop,value,np)) != TM_OK)
		return e;

	TMTRACE(TM_GRAPH_TRACE, "exit\n");
	return TM_OK;
}
/* NEW, ok 3.8.2003 FIXME: ambigos lookup !*/
TMError tm_topicmap_get_topic(TMTopicMap self, const char *modelname,
			const char *propname, const void *value, TMTopic *np, TMProperty *pp)
{
	TMError e;
	TMModel model;
	TMProperty prop;
	TMPropertyStore pstore;
	TMValueStore vstore;
	TMVid vid;
	const char corename[] = "RM-Core";

	assert(self);
	assert(propname);
	assert(value);
	assert(np);
 
        if(!modelname || !*modelname)
        {
                modelname = corename;
        }


	*np = 0;

	/* did caller supply TMProperty pointer? => initialize */
	if(pp)
		*pp = NULL;



	TMTRACE(TM_GRAPH_TRACE, "enter model=%s prop=%s\n" _ modelname _ propname);
	if( (model = tm_topicmap_get_model_by_name(self,modelname)) == NULL) 
		return TM_OK;  /* model absence is ok on get_topic()  */

	/* FIXME propably report error if asked for non-loaded prop model */
	
       	if( (prop = tm_model_get_property(model,NULL,propname)) == NULL)
		return TM_OK;
	if(pp)
		*pp = prop;

	if(prop->type != TM_SIDP)
	{
		tm_set_error(self->tm,"%s is not an SIDP, lookup is impossible",
				prop->fullname);
		return TM_E_SIDP_EXPECTED;
	}

	TMTRACE(TM_GRAPH_TRACE, "value: ");
	TMTRACE_VALUE(TM_GRAPH_TRACE,prop->value_type,value);

	pstore = tm_table_get(self->propname_to_pstore_table,prop->fullname);
	vstore = tm_table_get(self->propname_to_vstore_table,prop->fullname);
	assert(pstore); /* if property is there, stores must be there too */
	assert(vstore);

	if(! tm_property_store_is_normalized(pstore))
	{
		tm_set_error(self->tm,"%s not normalized in call to get_topic()",
				prop->fullname);
		return TM_E_PSTORE_NOT_NORMALIZED;
	}

	if( ! prop->equality_code ) /* prop NOT defines own equality */
	{
		if( (e = tm_value_store_lookup_vid(vstore,value,TM_VALUE_LOOKUP,&vid)) != TM_OK)
			return e;		
	}
	else
	{
		TMError e;
        	int opcode;
		TMValueStoreScan vscan;


		/* get opcode from operator string in TMA definition */	
		opcode = tm_valuetype_parse_opstring(prop->value_type, prop->equality_code);
		if(!opcode)
		{
			tm_set_error(self->tm,
				"operator '%s' not defined for valuetype '%s'",
				prop->equality_code, prop->value_type->name);
			return TM_E_OP_UNDEFINED;
		}
		/* no argstring parsing needed, arg is value (Vq) */

		if( (e = tm_value_store_scan_open(vstore, opcode, value , &vscan )) != TM_OK)
			return e;
		
		if( tm_value_store_scan_finished(vstore,vscan) )
		{
			if( (e = tm_value_store_scan_close(vstore,vscan)) != TM_OK)
				return e;
			/* not found (topic already set to 0, simply return) */
			return TM_OK;
		}
		tm_value_store_scan_fetch(vstore,vscan,&vid);
		if(! tm_value_store_scan_finished(vstore,vscan) )
		{
			if( (e = tm_value_store_scan_close(vstore,vscan)) != TM_OK)
				return e;
			tm_set_error(self->tm,
				"ambigous lookup for property value"); /* FIXME: message! */
			return TM_E_AMBIGUOUS_LOOKUP;
		}
		if( (e = tm_value_store_scan_close(vstore,vscan)) != TM_OK)
			return e;
	}

	if( ! vid)
		return TM_OK;



	if( (e = tm_property_store_lookup_topic(pstore,vid,np)) != TM_OK)
	{
		return e;
	}
	return TM_OK;
}

/* revised 7 Mar 2003 */
TMError tm_topicmap_lookup_topics_by_property(TMTopicMap self,
					  const char *modelname,
					  const char *propname,
					  const void *value,
					  int flag,
					  TMTopicSet topicset)
{
	TMError e;
	TMProperty prop;
	TMValueType vtype;
	TMValueStore vstore;
	TMPropertyStore pstore;

	assert(self);
	assert(modelname);
	assert(propname);
	assert(value);
	assert(topicset);
	assert(0);

	TMTRACE(TM_GRAPH_TRACE, "enter\n");

	prop = _lookup_property(self,modelname,propname);
	if (!prop)
		return TM_OK;
	/* --FIXME--
	pstore = _get_propertystore(self,prop->model,prop->name);
	*/
	assert(pstore); /* if prop is in topicmap, store must be too */

	vtype = tm_property_get_valuetype(prop);

	vstore = (TMValueStore)tm_table_get(self->propname_to_vstore_table,prop->name);
	assert(vstore); /* prop in topicmap requires vstore to be there too */

	
	if(flag == TM_LOOKUP_EQUAL)
	{
		TMVid vid;
		/*
		if( (e = tm_value_store_get_vid(vstore,value,&vid)) != TM_OK)
			return e;
		*/
		
		if(prop->type == TM_SIDP)
		{
			TMTopic topic;

			if( (e = tm_property_store_lookup_topic(pstore,vid,
					&topic)) != TM_OK)
				return e;
			if(topic)
				tm_topicset_add(topicset,topic);
		}
		else
		{
			TMTopic *topics;
			int i,len;

			if( (e = tm_property_store_lookup_topics(pstore,vid,
				&topics,&len)) != TM_OK)
				return e;
			for(i=0; i<len; i++)
				tm_topicset_add(topicset,topics[i]);
			/* no cleanup required! */
			/* FIXME: propably pass topicset */
		}
	}
	else
	{
		
		TMTopicSet set;
		int length;
		assert(flag == TM_LOOKUP_LIKE);

		set = tm_topicset_new(0);
	/* --- FIXME
		if( (e = tm_value_store_get_vids_like(vstore,
						value,set)) != TM_OK)
		{
			tm_topicset_delete(&set);
			return e;
		}
	*/
		length = tm_topicset_size(set);

		if(prop->type == TM_SIDP)
		{
			int k;
			for(k=0; k<length; k++)
			{
				TMVid vid;
				TMTopic topic;
				vid = tm_topicset_get(set,k);

				if( (e = tm_property_store_lookup_topic(pstore,
					vid,&topic)) != TM_OK)
				{
					tm_topicset_delete(&set);
					return e;
				}
				if(topic)
					tm_topicset_add(topicset,topic);
			}
		}
		else
		{
			int k;
			for(k=0; k<length; k++)
			{
				TMVid vid;
				TMTopic *topics;
				int i,len;
				vid = tm_topicset_get(set,k);

				if( (e = tm_property_store_lookup_topics(pstore,
					vid,&topics,&len)) != TM_OK)
				{
					tm_topicset_delete(&set);
					return e;
				}
				for(i=0; i<len; i++)
					tm_topicset_add(topicset,topics[i]);
			}

		}
		tm_topicset_delete(&set);
	}
	return TM_OK;
}
/* NEW */
TMError tm_topicmap_add_subject(TMTopicMap self, TMSubject subject, TMTopic *np)
{
	TMError e;
	int i;
	TMTopic topic;
	assert(self);

	TMTRACE(TM_GRAPH_TRACE, "enter\n");

	/* we allways create a new topic */
	if( (e = _topicmap_create_topic(self,&topic)) != TM_OK)
			return e;
	TMTRACE(TM_GRAPH_TRACE, "new topic created: %d\n" _ topic);

	/* Allow anonymous topic! Propably we do not want to allow this!! (FIXME) */
	if(subject == NULL)
	{
		assert(np);	/* FIXME: error code for this? */
		*np = topic;
		return TM_OK;
	}

	/* now add each property to the topic */
        for(i=0;i<subject->N;i++)
        {
		if( (e = tm_topicmap_add_property_to_topic(self,topic,
					subject->props[i].model,
					subject->props[i].name,
					subject->props[i].value)) != TM_OK)
		{
			return e; 
		}

	}
	/*
	LOG(self,"BUILT-IN-NODE topic %d" _ topic);
	*/
	/* if caller supplied topic pointer, we pass back topic */
	if(np)
		*np = topic;
	TMTRACE(TM_GRAPH_TRACE, "exit\n");
	return TM_OK;
}
TMError tm_topicmap_add_built_in_topic(TMTopicMap self, TMSubject subject, TMTopic *np)
{
	TMError e;
	int i;
	TMTopic topic,new_topic;
	assert(self);
	assert(subject);
	assert(0);

	TMTRACE(TM_GRAPH_TRACE, "enter\n");

	/* FIXME: is that ok or not? what behaviour do we want ? */
	/* FIXME: make sure that subjects are equal, or at least the provided
	 * one is a supset of the existing one. !!! */
	if( (e = tm_topicmap_lookup_topic_by_subject(self,subject,&topic)) != TM_OK)
		return e;

	

	/* topic not found, make new one */

	if(! topic)
	{
		if( (e = _topicmap_create_topic(self,&topic)) != TM_OK)
			return e;
	}


	/* now add each property to the topic or at least make sure that it is there */
        for(i=0;i<subject->N;i++)
        {
		assert(0);
		/* --FIXME--
		if( (e = _add_property(self,topic,
					subject->props[i].model,
					subject->props[i].name,
					subject->props[i].value,
					&new_topic)) != TM_OK)
		{
			return e; 
		}
		*/
		assert(!new_topic);

	}
	/*
	LOG(self,"BUILT-IN-NODE topic %d" _ topic);
	*/
	if(np)
		*np = topic;
	TMTRACE(TM_GRAPH_TRACE, "exit\n");
	return TM_OK;
}
/* NEW, ok 1.8.2003 */
TMError tm_topicmap_add_topic_assertion(TMTopicMap self, TMTopicAssertion astn)
{
	void *av;
	void *tv;
	int i;
	TMError e;
	TMTRACE(TM_GRAPH_TRACE,"enter\n");
	/* create A-topic and insert into assertion structure so it is
	 * accessable by caller
	 */
	if(astn->N < 2) 
	{
		tm_set_error(self->tm,
		"assertion must contain at least 2 memberships (found only %d)"
		, astn->N);
		return TM_E_INVALID_ASSERTION;
	}
	if(! astn->T ) 
	{
		tm_set_error(self->tm,
		"assertion must be typed");
		return TM_E_INVALID_ASSERTION;
	}
	if(!astn->A)
	{
		if( (e = _topicmap_create_topic(self,&(astn->A) )) != TM_OK)
				return e;
	}
	/* add property to A-topic */
	/* create and construct asidp value according to assertion
	 * structure.
	 */
	tv = tm_value_new( &TopicSet );
	av = tm_value_new( &ASIDP );
	tm_value_call( &ASIDP , av , "setType" , astn->T );
	for(i=0; i<astn->N; i++)
	{
		tm_value_call( &ASIDP , av , "addMember" ,
			       	astn->memberships[i].R, astn->memberships[i].x);
		tm_value_call( &TopicSet , tv , "addMember" , astn->memberships[i].R);
	}
	/* add property to A-topic */
	if( (e = tm_topicmap_add_property_to_topic(self,astn->A,"RM-Core","a-sidp",av)) != TM_OK)
		return e;
	if( (e = tm_topicmap_add_property_to_topic(self,astn->T,"RM-Core","t-sidp",tv)) != TM_OK)
		return e;
	tm_value_delete(&ASIDP,&av);
	tm_value_delete(&TopicSet,&tv);
	TMTRACE(TM_GRAPH_TRACE,"exit\n");
	return TM_OK;
}

TMError tm_topicmap_get_models(TMTopicMap self,TMList *listp)
{
	TMList list = NULL,lp;
	assert(self);

	for(lp = self->models; lp; lp=lp->next)
		list = tm_list_push(list,(TMModel)lp->content);

	*listp = list;
	return TM_OK;
}

/* ========================================================================= */
/* ========================================================================= */
/* ========================================================================= */
/* ========================================================================= */
/* ========================================================================= */
/* ========================================================================= */





#if 0
TMError _assertion_to_topic_assertion(TMTopicMap self, TMAssertion a, TMTopicAssertion na)
{
	TMError e;
	int i;
	assert(self);
	assert(a);
	assert(na);

	TMTRACE(TM_GRAPH_TRACE,"enter\n");

	if( (e = tm_topicmap_lookup_topic_by_subject(self,a->type, &na->T )) != TM_OK)
		return e;

	if(! na->T)
	{
		tm_set_error(self->tm,"required assertion type topic not in the topicmap");
		return TM_ENODE_NOT_FOUND;
	}
	TMTRACE(TM_GRAPH_TRACE,"T handled\n");

	if(a->self)
	{
		/*
		tm_topicmap_lookup_topic_by_subject(self,a->self,&na->A);	
		*/
		/* FIXME:
		 * this topic cannot be built in by client, since if atopic has
		 * no SIDPs, it is not 'keepable', there is no ID for it
		 * outside the topicmap. OTH, here we can (I THINK!!) build it in.
		 */
		tm_topicmap_add_built_in_topic(self,a->self,&na->A);
		assert(na->A);
	}
	else
	{
			_topicmap_create_topic(self,&na->A);
	}
	TMTRACE(TM_GRAPH_TRACE,"A handled\n");


	for(i=0; i<a->N; i++)
	{
		TMTopic topic;
		TMTRACE(TM_GRAPH_TRACE,"Membership %d\n" _ i);

		assert(a->memberships[i].role);
		
		tm_topicmap_lookup_topic_by_subject(self,a->memberships[i].role,&topic);
		if(! topic)
		{
			/* FIXME: any cleanup ?? */
			return TM_ENODE_NOT_FOUND;
		}
		na->memberships[i].R = topic;
		TMTRACE(TM_GRAPH_TRACE,"R handled\n");


		if(a->memberships[i].casting)
		{
			tm_topicmap_lookup_topic_by_subject(self,a->memberships[i].casting,&topic);
			/* we don;t have a casting topic thing yet ! */
			/* but should work as soon as assert(0) is deleted */
			assert(topic);
			assert(0);
		}
		else
		{
			_topicmap_create_topic(self,&topic);
		}
		na->memberships[i].C = topic;
		TMTRACE(TM_GRAPH_TRACE,"C handled\n");


		if(a->memberships[i].player)
		{
			/* FIXME: this should not be allowed to be built in... */
			/* it is just a workaround the absense of SIDPs when processing
			 * RDF. (need to handle both cases or to change lookup function! */
			
			tm_topicmap_lookup_topic_by_subject(self,a->memberships[i].player,&topic);
			assert(topic);
			/*
			tm_topicmap_add_built_in_topic(self,a->memberships[i].player,&topic);
			assert(topic);
			*/
		}
		else
		{
			_topicmap_create_topic(self,&topic);
		}

		assert(topic);
		na->memberships[i].x = topic;
		TMTRACE(TM_GRAPH_TRACE,"x handled\n");
	}

	na->N = a->N;	

	return TM_OK;
}
#endif




/* ------------------------------------------------------------------------- 
 *
 *                              STATIC ROUTINES
 *
 * ------------------------------------------------------------------------- */
TMError _load_model(TMTopicMap self,TMModel model)
{
	TMError e;
	TMList pp;
	/*
	TMSubject *sp;
	TMAssertion *ap;
	*/
	TMList ap;


	assert(self);
	assert(model);
	TMTRACE(TM_GRAPH_TRACE,"enter [model=%s]\n" _ model->name);

	for(pp = model->properties; pp; pp = pp->next)
	{
		TMValueStore vstore;
		TMPropertyStore pstore;
		TMProperty prop = (TMProperty)pp->content;
		const char *vtype_name = prop->value_type->name;
		TMTRACE(TM_GRAPH_TRACE,
			"loading property %s (%s)\n" _
			prop->name _ vtype_name );

		vstore = tm_value_store_new(self->storage_descriptor,self->storage_handle,prop);
		if(! vstore)
		{
			TMTRACE(TM_GRAPH_TRACE,"no value store found for value type %s\n" _
					prop->value_type->name);
			tm_set_error(self->tm,"no value store found for value type %s\n",
					prop->value_type->name);
			return TM_E_VALUE_STORE_NOT_FOUND; /* FIXME! */

		}

		if( (e = tm_value_store_open(vstore)) != TM_OK)
		{
			/* FIXME  cleanup of all the above */
			assert(0);
			return e;

		}

		pstore = tm_property_store_new(self->storage_descriptor,
				self->storage_handle, prop, vstore );
		if( (e = tm_property_store_open(pstore)) != TM_OK)
		{
			/* FIXME: cleanup */
			return e;
		}
		self->property_stores = tm_list_push(self->property_stores,pstore);
		self->value_stores = tm_list_push(self->value_stores,vstore);
		tm_table_put(self->propname_to_pstore_table,prop->fullname,pstore);
		tm_table_put(self->propname_to_vstore_table,prop->fullname,vstore);

	}
	/* FIXME: in order to allow TMA defs to use props of the TMA itself, we need to
	 * mark TMA as loaded now. FIXME: there is more to this - or?
	 */
	self->models = tm_list_append(self->models,model);

	for(ap = model->atypes; ap; ap=ap->next)
	{
		TMList rp;
		TMAssertionType at = (TMAssertionType)ap->content;
		TMTRACE(TM_GRAPH_TRACE,"Loading assertion type %s\n" _
			       at->name);

		if( (e = tm_topicmap_add_subject(self, at->subject, NULL)) != TM_OK)
			return e;
		for(rp = at->roles; rp; rp=rp->next)
		{
			TMRoleType r = (TMRoleType)rp->content;
			TMTRACE(TM_GRAPH_TRACE,"loading role type %s\n" _
			       r->name);
			if( (e = tm_topicmap_add_subject(self, r->subject, NULL)) != TM_OK)
				return e;

		}

	}	
	for(ap = model->subjects; ap; ap=ap->next)
	{
		if( (e = tm_topicmap_add_subject(self, (TMSubject)ap->content, NULL)) != TM_OK)
			return e;
	}	
#if 0
	for(sp = model->built_in_topics; sp && *sp; sp++)
	{
		TMTopic topic;
		if( (e = tm_topicmap_add_built_in_topic(self,  *sp , &topic)) != TM_OK)
		{
			/* FIXME: cleanup */
			return e;
		}
		TMTRACE(TM_GRAPH_TRACE,"added built-in-topic %d\n" _ topic);

	}

	for(ap = model->built_in_assertions; ap && *ap; ap++)
	{
		if( (e = tm_topicmap_add_assertion(self,  *ap )) != TM_OK)
		{
			/* FIXME: cleanup */
			return e;
		}

	}
#endif
	tm_topicmap_fully_merge(self);
	TMTRACE(TM_GRAPH_TRACE,"exit [model=%s]\n" _ model->name);
	return TM_OK;
}

TMProperty
_lookup_property(TMTopicMap self, const char *modelname, const char* propname)
{
	TMModel m;
	
	assert(self);
	assert(modelname);
	assert(propname);

	if( (m = tm_topicmap_get_model_by_name(self,modelname))  == NULL) 
               	return NULL;

	return (tm_model_get_property(m,NULL,propname));
}
TMModel tm_topicmap_get_model_by_name(TMTopicMap self,const char *name)
{
	TMList lp;
	assert(self);
	assert(name);

	TMTRACE(TM_GRAPH_TRACE, "enter, name=%s\n" _ name);
	for(lp = self->models; lp; lp = lp->next )
	{
		if(strcmp( ((TMModel)lp->content)->name,name) == 0)
		{
			TMTRACE(TM_GRAPH_TRACE, "exit (found)\n");
			return (TMModel)lp->content;
		}
	}
	TMTRACE(TM_GRAPH_TRACE, "exit (not found)\n");
	return NULL;
}

TMError _lookup_topic_by_sidp(TMTopicMap self,
			TMProperty prop, const void *value , TMTopic *np)
{
	/*
	TMError e;
	*/
	TMValueType vtype;
	TMValueStore vstore;
	TMPropertyStore pstore;
	/*
	TMVid vid;
	*/

	TMTRACE(TM_GRAPH_TRACE,"enter\n");
	
	assert(self);
	assert(prop);
	assert(value);
	assert(np);

	/* caller is required to make sure prop is SIDP and that it
	 * exists in the topicmap.
	 * FIXME: maybe remove the second assertion, since if
	 * prop is passed, it actually has to be in topicmap.*/
	assert(prop->type == TM_SIDP);
	assert( _lookup_property(self,prop->model,prop->name));
	/*--FIXME--
	pstore = _get_propertystore(self,prop->model,prop->name);
	*/
	assert(pstore);	/* if prop is in topicmap, pstore MUST be there too */

	vtype = tm_property_get_valuetype(prop);

	TRACE_PROP_VALUE(prop,vtype,value);
	vstore = (TMValueStore)tm_table_get(self->propname_to_vstore_table,prop->name);
	assert(vstore);	/* if prop in topicmap, vstore MUST be there too */
	/* --FIXME--
	if( (e = tm_value_store_get_vid(vstore,value,&vid)) != TM_OK)
		return e;

	if( (e = tm_property_store_lookup_topic(pstore,vid,np)) != TM_OK)
		return e;
	*/

	return TM_OK;
}


/* NEW */
void tm_topicmap_dump(TMTopicMap self, FILE *file)
{
	TMList lp;
	TMTopic i,max;
	assert(self);
	assert(file);

	fprintf(file,"[Topic Map Dump]\n\n\n");
	assert(self);

	fprintf(file,"[Models]\n");
	for(lp = self->models; lp; lp=lp->next)
	{
		TMModel model = (TMModel)lp->content;
		fprintf(file,"%s\n",model->name );

	}

/*--------*/
	fprintf(file,"Special Section: Value Stores\n");
	fprintf(file,"=======================================================\n");
	for(lp = self->value_stores; lp; lp=lp->next)
	{
		tm_value_store_print((TMValueStore)lp->content,file);

	}
	fprintf(file,"=======================================================\n");
/*--------*/

	for(lp = self->property_stores; lp; lp=lp->next)
	{
		tm_property_store_print((TMPropertyStore)lp->content,file);

	}
	fprintf(file,"\n[Unprocessed Merge List]\n");
	for(lp = self->merge_lists; lp; lp=lp->next)
	{
		TMTopicSet set = (TMTopicSet)lp->content;
		tm_topicset_print(set,file);
		fprintf(file,"\n");
	}
	fprintf(file,"\n[Topic with properties]\n");
	
	tm_meta_store_get_topicmax(self->meta_store,&max);	
	for(i=1;i<=max;i++)
	{
		TMList mp;
		int topic_printed = 0;
		for(mp=self->models;mp;mp=mp->next)
		{
			TMList pp;
			TMError e;
			TMModel m = (TMModel)mp->content;
			for(pp=m->properties; pp; pp=pp->next)
			{
				void *value;
				char buf[4096];
				TMPropertyStore pstore;
				TMValueStore vstore;
				TMProperty prop = (TMProperty)pp->content;

				pstore = tm_table_get(self->propname_to_pstore_table,prop->fullname);
				vstore = tm_table_get(self->propname_to_vstore_table,prop->fullname);
				assert(pstore); assert(vstore);
				if( (e = _pstore_get(self,prop,pstore,vstore,i,&value))
									!= TM_OK)
					return; /* FIXME */
				if(!value)
					continue;
				tm_value_to_string(prop->value_type,value,buf,sizeof(buf));
				if(!topic_printed)
				{
					fprintf(file,"<%06d> %-20s : %s\n",i,prop->fullname,buf);
					topic_printed = 1;
				}
				else
				{
					fprintf(file,"         %-20s : %s\n", prop->fullname,buf);
				}
				tm_value_delete(prop->value_type,&value);
		
			} 
		} 
		if(topic_printed)
		{
			assert(self->disposed[i] == 0);
			fprintf(file,"\n");
		}
		else
		{
			assert(self->disposed[i] == 1);
			fprintf(file,"<%06d> *** DISPOSED ***\n",i);
		}


	}


}
void _set_contradicting_values_error(TMTopicMap self,TMProperty prop,
		TMValueStore vstore,TMVid vid1,TMVid vid2)
{
	void *v;
	char buf1[1024],buf2[1024];
	/* FIXME: what about the ret codes of these functins.....?? */
	tm_value_store_get_value(vstore,vid1,&v);
	tm_value_to_string(prop->value_type,v,buf1,sizeof(buf1));
	tm_value_delete(prop->value_type,&v);
	tm_value_store_get_value(vstore,vid2,&v);
	tm_value_to_string(prop->value_type,v,buf2,sizeof(buf2));
	tm_value_delete(prop->value_type,&v);

	tm_set_error(self->tm,"contradicting values for %s: %s contradicts %s",
			prop->fullname, buf1,buf2);
}
void _set_contradicting_values_error_2(TMTopicMap self,TMProperty prop,
		const void *v1,const void *v2)
{
	char buf1[1024],buf2[1024];
	/* FIXME: what about the ret codes of these functins.....?? */
	tm_value_to_string(prop->value_type,v1,buf1,sizeof(buf1));
	tm_value_to_string(prop->value_type,v2,buf2,sizeof(buf2));

	tm_set_error(self->tm,"contradicting values for %s: %s contradicts %s",
			prop->fullname, buf1,buf2);
}

void tm_topicmap_add_mergelist(TMTopicMap self, TMTopic *array, int len)
{
	/*
	TMList tmp = NULL;
	TMTopic t;
	*/
	int i;
	TMTopicSet set;
	TMList lp,prev = NULL;

	/* make set from passed params */
	set = tm_topicset_new(len);
	for(i=0;i<len;i++)
		tm_topicset_add(set,array[i]);

	TMTRACE(TM_GRAPH_TRACE,"starting loop over all merge lists\n");
	/* make sure that merge_sets stay disjoint */
	for(lp = self->merge_lists; lp; lp=lp->next)
	{
		TMTopicSet other;
		TMTRACE(TM_GRAPH_TRACE,"----- LOOP -----\n");
		other = (TMTopicSet)lp->content;
		TMTRACE(TM_GRAPH_TRACE,"checking if current disjoint with new one\n");
		if( ! tm_topicset_disjoint( other , set ))
		{
			TMTRACE(TM_GRAPH_TRACE,"not disjoint -> 'unite'\n");
			for(i=0;i<len;i++)
				tm_topicset_add(other,tm_topicset_get(set,i) );
			tm_topicset_delete(&set);
			set = other;
			/* take other out of list since contents is now in 'set' */
			TMTRACE(TM_GRAPH_TRACE,"prev is: \n" _ prev ? "non-null" : "NULL");
			if(!prev)
			{
				assert(lp);
				assert(lp == self->merge_lists);
				/* special case: first element in list */
				TMTRACE(TM_GRAPH_TRACE,"prev is NULL\n");
				self->merge_lists = lp->next;
				TMTRACE(TM_GRAPH_TRACE,"reasigned self->merge_lists\n");
				tm_list_delete(&lp);
				TMTRACE(TM_GRAPH_TRACE,"deleted lp\n");

				lp = self->merge_lists;	
				TMTRACE(TM_GRAPH_TRACE,"lp now points to merge_lists\n");
			}
			else
			{
				assert(prev->next == lp);
				prev->next=lp->next;
				lp->next = NULL;
				tm_list_delete(&lp);
				lp = prev;
			}
			/* FIXME: do we need to delete topic set when taling out? */

		}
		prev = lp;
		if(!lp)
			break;
		TMTRACE(TM_GRAPH_TRACE,"set prev to lp\n");
	}
	TMTRACE(TM_GRAPH_TRACE,"loop exited\n");
	
	TMTRACE(TM_GRAPH_TRACE,"pushing set onto merge lists\n");
	self->merge_lists = tm_list_push(self->merge_lists,set);

}




TMError tm_topicmap_topic_scan_open_with_single_condition(TMTopicMap self, const char *modelname, const char *propname, const char *opstring, const char *argstring, TMTopicScan *scanp)
{
	TMCETree cetree;

	TMModel model;
	TMProperty prop;
	int opcode;
	void *arg;
	const char corename[] = "RM-Core";
 
        if(!modelname || !*modelname)
        {
                modelname = corename;
        }

	
	model = tm_topicmap_get_model_by_name(self,modelname);
	assert(model);
       	prop = tm_model_get_property(model,NULL,propname);
	assert(prop);

	opcode = tm_valuetype_parse_opstring(prop->value_type, opstring);
	if(!opcode)
	{
		tm_set_error(self->tm,
		"operator '%s' not defined for valuetype '%s'", opstring, prop->value_type->name);
       		return TM_E_OP_UNDEFINED;
	}
	arg = tm_valuetype_parse_argstring(prop->value_type, opcode, argstring);
	assert(arg);  /* FIXME: how to report arg parse error cause ?!!! */

	TM_NEW(cetree);
	cetree->tree_op = TM_CETREE_OP_SIMPLE_CONDITION;
	cetree->prop = prop;
	cetree->op = opcode;
	cetree->arg = arg;

	tm_topicmap_topic_scan_open(self, cetree, scanp);
	return TM_OK;
}
TMError tm_topicmap_topic_scan_open_with_double_condition(TMTopicMap self,
        const char *modelname1, const char *propname1, const char *opstring1, const char *argstring1,        const char *operator,
        const char *modelname2, const char *propname2, const char *opstring2, const char *argstring2, 
        TMTopicScan *scanp)
{
	TMCETree root,left,right;

	TMModel model1,model2;
	TMProperty prop1,prop2;
	int opcode1,opcode2;
	void *arg1,*arg2;
	const char corename[] = "RM-Core";
 
        if(!modelname1 || !*modelname1)
        {
                modelname1 = corename;
        }
        if(!modelname2 || !*modelname2)
        {
                modelname2 = corename;
        }
	
	model1 = tm_topicmap_get_model_by_name(self,modelname1);
	assert(model1);
       	prop1 = tm_model_get_property(model1,NULL,propname1);
	assert(prop1);

	opcode1 = tm_valuetype_parse_opstring(prop1->value_type, opstring1);
	if(!opcode1)
	{
		tm_set_error(self->tm,
		"operator '%s' not defined for valuetype '%s'", opstring1, prop1->value_type->name);
       		return TM_E_OP_UNDEFINED;
	}
	arg1 = tm_valuetype_parse_argstring(prop1->value_type, opcode1, argstring1);
	assert(arg1);  /* FIXME: how to report arg parse error cause ?!!! */

	model2 = tm_topicmap_get_model_by_name(self,modelname2);
	assert(model2);
       	prop2 = tm_model_get_property(model2,NULL,propname2);
	assert(prop2);

	opcode2 = tm_valuetype_parse_opstring(prop2->value_type, opstring2);
	if(!opcode2)
	{
		tm_set_error(self->tm,
		"operator '%s' not defined for valuetype '%s'", opstring2, prop2->value_type->name);
       		return TM_E_OP_UNDEFINED;
	}
	arg2 = tm_valuetype_parse_argstring(prop2->value_type, opcode2, argstring2);
	assert(arg2);  /* FIXME: how to report arg parse error cause ?!!! */






	TM_NEW(root);
	TM_NEW(left);
	TM_NEW(right);

	left->tree_op = TM_CETREE_OP_SIMPLE_CONDITION;
	left->prop = prop1;
	left->op = opcode1;
	left->arg = arg1;

	right->tree_op = TM_CETREE_OP_SIMPLE_CONDITION;
	right->prop = prop2;
	right->op = opcode2;
	right->arg = arg2;
	
	if( strcmp(operator,"AND") == 0)
		root->tree_op = TM_CETREE_OP_AND;
	else if( strcmp(operator,"OR") == 0)
		root->tree_op = TM_CETREE_OP_OR;
	else { assert(0); exit(1); }
	root->left = left;
	root->right = right;

	tm_topicmap_topic_scan_open(self, root, scanp);

	/* FIXME: when to free tree ??? */

	return TM_OK;
}

TMError tm_topicmap_topic_scan_open(TMTopicMap self, TMCETree cetree, TMTopicScan *scanp)
{
	TMTopicScan sp;
	TMTopicSet set;
	int i,len;

	TM_NEW(sp);
	sp->foo = 10;
	sp->topics = NULL;
	set = tm_topicset_new(100); /* FIXME, hint works or not?? */

	TMTRACE(TM_GRAPH_TRACE,"before evaluating cetree\n");
	tm_topicmap_cetree_eval(self,cetree, set);
	/* FIXME: error check */
	TMTRACE(TM_GRAPH_TRACE,"evaluated cetree\n");

	len = tm_topicset_size(set);
	for(i=0;i<len;i++)
		sp->topics = tm_list_push(sp->topics,(void*)tm_topicset_get(set,i) );

	*scanp = sp;
	TMTRACE(TM_GRAPH_TRACE,"exit\n");
	return TM_OK;
}

TMError tm_topicmap_topic_scan_close(TMTopicMap self, TMTopicScan scan)
{
	TM_FREE(scan);
	return TM_OK;
}

int tm_topicmap_topic_scan_finished(TMTopicMap self, TMTopicScan scan)
{
	return (scan->topics == NULL);
}

TMError tm_topicmap_topic_scan_fetch(TMTopicMap self, TMTopicScan scan, TMTopic *tp)
{
	void *v;
	TMTRACE(TM_GRAPH_TRACE,"1\n");
	assert(scan->topics);
	TMTRACE(TM_GRAPH_TRACE,"2\n");
	scan->topics = tm_list_pop(scan->topics,&v);
	TMTRACE(TM_GRAPH_TRACE,"3\n");
	*tp = (TMTopic)v;
	return TM_OK;
}


TMError tm_topicmap_cetree_eval(TMTopicMap self, TMCETree cetree, TMTopicSet set)
{
	TMValueStoreScan vscan;
	TMValueStore vstore;
	TMPropertyStore pstore;
	/*
	TMError e;
	*/
	assert(self);
	assert(cetree);
	assert(set);
	assert(tm_topicset_size(set) == 0);

	TMTRACE(TM_GRAPH_TRACE,"enter\n");
	switch(cetree->tree_op) {
	case TM_CETREE_OP_AND: {
		TMTopicSet leftset,rightset;
		int i,len;
		leftset = tm_topicset_new(100);
		rightset = tm_topicset_new(100);
		tm_topicmap_cetree_eval(self,cetree->left,leftset);
		tm_topicmap_cetree_eval(self,cetree->right,rightset);
		len = tm_topicset_size(leftset);
		for(i =0; i<len; i++)
		{
			TMTopic t = tm_topicset_get(leftset,i);
			if(tm_topicset_contains(rightset,t) )
				tm_topicset_add(set,t);
		}
		tm_topicset_delete(&leftset);
		tm_topicset_delete(&rightset);
		break; }
	case TM_CETREE_OP_OR: {
		TMTopicSet rightset;
		int i,len;
		rightset = tm_topicset_new(100);
		tm_topicmap_cetree_eval(self,cetree->left,set);
		tm_topicmap_cetree_eval(self,cetree->right,rightset);
		len = tm_topicset_size(rightset);
		for(i =0; i<len; i++)
		{
			TMTopic t = tm_topicset_get(rightset,i);
				tm_topicset_add(set,t);
		}
		tm_topicset_delete(&rightset);
		break; }
	case TM_CETREE_OP_SIMPLE_CONDITION:
		pstore = tm_table_get(self->propname_to_pstore_table,cetree->prop->fullname);
		vstore = tm_table_get(self->propname_to_vstore_table,cetree->prop->fullname);
		assert(pstore); /* if property is there, stores must be there too */
		assert(vstore);

		tm_value_store_scan_open(vstore, cetree->op, cetree->arg , &vscan );
		while(! tm_value_store_scan_finished(vstore,vscan) )
		{
			TMVid vid;
			int i,len;
			TMTopic *topics;
			tm_value_store_scan_fetch(vstore,vscan,&vid);
			/*
			fprintf(stderr,"VID:====== %d\n\n", vid);
			*/
			tm_property_store_lookup_topics(pstore,vid,&topics,&len);
			for(i=0;i<len;i++)
				tm_topicset_add(set,topics[i]);
		}
		tm_value_store_scan_close(vstore,vscan);

		break;

	default:
		assert(0);
	}

	return TM_OK;
}
/* everywhere!!! FIXME FREE args that have been parsed!!!! ! */



TMError tm_topicmap_query(TMTopicMap self, void *user_data, TMViewStartEventHandler start, TMViewEndEventHandler end, const char* QUERY)
{
	TMError e;
	char *q,query[1024]; 
	char *view_name;
	char *param_string;
	struct view view;
	TMParams params;
	TMError(*handler)(struct view*,TMParams) = NULL;
	/*TMViewExtractor view_extractor = NULL;
	TMList lp;
	*/

	assert(self);
	view.tm = self->tm;
	view.topicmap = self;
	view.user_data = user_data;
	view.start = start;
	view.end = end;
	
	assert(query);
	TMTRACE(TM_GRAPH_TRACE,"enter: query=%s\n" _ query);

	if(strncmp(QUERY,"VIEW ",5) != 0)
	{
		tm_set_error(self->tm,"only VIEW queries are supported");
		return TM_E_SYNTAX;
	}
	assert(strlen(QUERY) < sizeof(query));
	strcpy(query,QUERY);
	q = query;
	q += 5;	/* skip "VIEW " */
	while(*q && isspace(*q)) q++;
	if( ! *q)
	{
		tm_set_error(self->tm,"syntax error, view name expected");
		return TM_E_SYNTAX;
	}
	view_name = q;
	while(*q && *q != '(')
	{
		if( ! isalnum(*q) && ! *q == '_')
		{
			tm_set_error(self->tm,"syntax error, view name must consist of alphanumeric characters");
			return TM_E_SYNTAX;

		}
		q++;
	}
	if( ! *q)
	{
		tm_set_error(self->tm,"syntax error, '(' expected");
		return TM_E_SYNTAX;
	}
	*q = '\0'; /* terminate view name */
	TMTRACE(TM_GRAPH_TRACE,"view = [%s]\n" _ view_name);

	q++;
	param_string = q;
	while(*q && *q != ')') q++;

	if(!*q)
	{
		tm_set_error(self->tm,"syntax error, ')' exepcted");
		return TM_E_SYNTAX;
	}
	*q = '\0'; /* terminate param_string */

	params = tm_params_new(param_string,TM_PARAMS_SIMPLE);


	if( (e=tm_disclosureframework_lookup_viewhandler(
			tm_get_disclosureframework(self->tm),
			self->tm,
			view_name, &handler)) != TM_OK)
	{
		return e;
	}

	e = handler(&view,params);
	if(e != TM_OK)
		return e;
	tm_params_delete(&params);	

	TMTRACE(TM_GRAPH_TRACE,"exit\n");
	return TM_OK;
}
#if 0
void print_topic(TMTopicMap self,TMTopic i, FILE* file)
{
	TMList mp;
	int topic_printed = 0;
	for(mp=self->models;mp;mp=mp->next)
	{
		TMList pp;
		TMError e;
		TMModel m = (TMModel)mp->content;
		for(pp=m->properties; pp; pp=pp->next)
		{
			void *value;
			char buf[4096];
			TMPropertyStore pstore;
			TMValueStore vstore;
			TMProperty prop = (TMProperty)pp->content;

			pstore = tm_table_get(self->propname_to_pstore_table,prop->fullname);
			vstore = tm_table_get(self->propname_to_vstore_table,prop->fullname);
			assert(pstore); assert(vstore);
			if( (e = _pstore_get(self,prop,pstore,vstore,i,&value))
								!= TM_OK)
				return; /* FIXME */
			if(!value)
				continue;
			tm_value_to_string(prop->value_type,value,buf,sizeof(buf));
			if(!topic_printed)
			{
				fprintf(file,"<%06d> %-20s : %s\n",i,prop->fullname,buf);
				topic_printed = 1;
			}
			else
			{
				fprintf(file,"         %-20s : %s\n", prop->fullname,buf);
			}
			tm_value_delete(prop->value_type,&value);
	
		} 
	} 
	if(topic_printed)
	{
		fprintf(file,"\n");
	}
}
#endif

int tm_topicmap_all_stores_normalized(TMTopicMap self)
{
	TMList lp;
	for(lp=self->property_stores;lp;lp=lp->next)
	{
		TMPropertyStore pstore = (TMPropertyStore)lp->content;
		if( ! tm_property_store_is_normalized(pstore))
		{
			return 0;
		}
	}
	return (1);
}

#if 0
=========================================================================

  OLD CODE RELATED TO UPDATES BASED ON SUBJECTS RATHER THAN TOPICS
  (from the time I did not allow topics outside the topic map. This
  has become a problem due to performance reasons)

=========================================================================

TMError tm_topicmap_add_property(TMTopicMap self, TMSubject subject,
    const char *modelname, const char *propname, const void *value, TMTopic *np)
{
	TMError e;
	TMTopic topic,new_topic;

	assert(0); /* not new */
	if( (e = tm_topicmap_lookup_topic_by_subject(self,subject,&topic))
					 != TM_OK)
		return e;

	/* caller is responsible for asssuring that topic is there */
	/* FIXME: propably that is not an error. Also, check error
	 * of funct call */
	/* FIXME: thios breaks with tmworld !!!! WHY ?!! */
	assert(topic);

	assert(0);
	/* --FIXME--
	if( (e = _add_property(self,topic,modelname,propname,value,&new_topic))
					!= TM_OK)
		return e;
	*/
	assert(!new_topic);	/* stop on merge, check what is affected! */

	/* if caller supplied a pointer, we return the topic */
	if(np)
		*np = topic;
	return TM_OK;
}
TMError tm_topicmap_add_assertion(TMTopicMap self, TMAssertion astn)
{
	TMError e;
	/*
	TMTopic atopic;
	int i;
	*/
	struct TMTopicAssertion _astn;
	assert(0);
	TMTRACE(TM_GRAPH_TRACE, "enter\n");
	/*
	LOG(self,TM_LOG_ADD_ASSERTION, astn );
	*/

	if( (e = _assertion_to_topic_assertion(self, astn, &_astn )) != TM_OK)
	{
		/* FIXME: the 2 lines below are just a workaround! */
		if(e == TM_ENODE_NOT_FOUND)
			return TM_OK;
		return e;
	}


	if( (e = _add_assertion(self,&_astn)) != TM_OK)
		return e;


	/* we can have untyped ones later on, but now this is clearer.
	* anyway, we would not store untyped assertions in an assertion store */


	return TM_OK;

}
#endif

#if 0
============================================================================
/* I keep some old code so I remember it....*/
============================================================================

GSError_T gs_topicmap_flush_all(TM_GRAPH_T g)
{
	int i;
	assert(g);
	gs_storage_flush_cache(g->meta_file);
	gs_storage_flush_cache(g->lpt_file);
	gs_storage_flush_cache(g->word_file);
	gs_storage_flush_cache(g->wordindex_file);
	gs_storage_flush_cache(g->topics_file);
	gs_storage_flush_cache(g->data_file);
	for(i= 0; i<g->nafiles;i++)
	{
		int j;
		gs_storage_flush_cache(g->afiles[i].file);
		for(j=0; j<g->afiles[i].nmembers;j++)
		{
			gs_storage_flush_cache(
				g->afiles[i].player_indexes[j].file);
		}
	}	
	return TM_OK;
}
GSError_T _index_data(TM_GRAPH_T topicmap,gs_topic_t topic, const char *data)
{
	/*
	GS_PAGE_T topic_headpage,dpage,npage,data_headpage;
	GS_PAGE_T topic_headpage;
	*/
	/*
	int data_pnum = 0;
	int data_offset = 0;
	*/
	char *word;
	char wordbuf[256];
	assert(topicmap);
	TMTRACE(TM_GRAPH_TRACE,"Data [%s]\n" _ data);	
	fflush(stdout);
	

	while( gs_strtok(&data," :.~,!()[]{}%\"@#$^&*+=|?-'<>	\t\r\n",wordbuf,sizeof(wordbuf)) != NULL) {
		char lower_word[256];
		char *p;
		word = wordbuf;
		gs_tolowercase(word,lower_word,sizeof(lower_word) );
		if(!*lower_word)
			continue;
		/*
		fprintf(stderr,"[_%s_]\n",lower_word);
		*/
		if(strstr(lower_word," ") != NULL) {
			TMTRACE(TM_GRAPH_TRACE,"[_%s_]\n" _ lower_word);
			continue;
		}
		p = lower_word;	
		/*
		printf("   %s ",p);	
		fflush(stdout);
		*/
		/*
		while( *p && *(p+1) && *(p+2) ) {
		while( strlen(p) > 2 ) {
		*/
		gs_lookup_topic_by_data(topicmap,p,GS_SCR,GS_CREATE,topic,NULL);
		while(0 && *p ) {

		TMTRACE(TM_GRAPH_TRACE,"Indexing [_%s_]\n" _ p);
		gs_lookup_topic_by_data(topicmap,p,GS_SCR,GS_CREATE,topic,NULL);
		p++;
		/*
		printf(".");	
		fflush(stdout);
		*/
		}

		/*
		printf("\n");	
		fflush(stdout);
		*/

	}

	return TM_OK;
}

GSError_T gs_topicmap_build_word_index(TM_GRAPH_T g)
{
	/*
	GS_PAGE_T topic_headpage,dpage,npage,data_headpage;
	*/
	GS_PAGE_T topic_headpage;
	gs_topic_t max,topic;
	/*
	int data_pnum = 0;
	int data_offset = 0;
	*/
	assert(g);
	
	

	g->word_index = (struct word_bucket**)GS_ALLOC(100 * sizeof(struct word_bucket*));
	g->word_index_len = 0;
	g->word_index_size = 100;

	gs_storage_fetch_page(g->topics_file,&topic_headpage,1);
	max = gs_topicsheadpage_get_topicmax(topic_headpage);
	/*
	pagecount = gs_topicsheadpage_get_pagecount(topic_headpage);
	*/
	gs_storage_drop_page(g->topics_file,topic_headpage);


	for(topic = 1; topic<=max;topic++)
	{
		char data[4096];
		char *word;
		char wordbuf[256];
		const char *dp = data;

		/* TBD: check freelist once we implemented merging  */
		gs_lookup_scr_data(g,topic,data,sizeof(data));
		if(! *data)
			continue;

		/*
		fprintf(stderr,"Node: %d, data: %s\n",topic,data);
		word = strtok_r(data," ",&lasts);
		*/

		while( gs_strtok(&dp," ",wordbuf,sizeof(wordbuf)) != NULL) {
			int k;
			int found = 0;
			char lower_word[256];
			word = wordbuf;
			gs_tolowercase(word,lower_word,sizeof(lower_word) );
			/*
			fprintf(stderr,"    %s\n",word);
			*/
			for(k=0; k<g->word_index_len;k++)
			{
				int c;
				c = strcmp(g->word_index[k]->word,lower_word);
				if(c == 0)
					found = 1;
				if(c >= 0)
					break;
			}
			if(found) {
				g->word_index[k]->array_len++;
				GS_RESIZE(g->word_index[k]->array,g->word_index[k]->array_len * 4);
				assert(topic);
				g->word_index[k]->array[g->word_index[k]->array_len-1] = topic;
			} else {
				struct word_bucket *bp;
				if(g->word_index_len >= g->word_index_size)
				{
					g->word_index_size *= 2;
					GS_RESIZE(g->word_index,g->word_index_size * sizeof(struct word_bucket) );
				}
				bp = (struct word_bucket *)
					GS_ALLOC(sizeof(struct word_bucket));
				assert(bp);
				bp->array_len = 1;
				bp->array = (int*)GS_ALLOC(1 * sizeof(gs_topic_t));
				assert(bp->array);
				assert(topic);
				bp->array[0] = topic;

				bp->word = (char*)GS_ALLOC(strlen(lower_word)+1);
				assert(bp->word);
				strcpy(bp->word,lower_word);
				

				/* if need to insert in between */
				if(k < g->word_index_len)
				{
					int j;
					for(j = g->word_index_len; j > k; j--)
						g->word_index[j] = g->word_index[j-1];


				}
				/*
				fprintf(stderr,"insert %s at pos %d\n",bp->word,k);
				*/
				g->word_index[k] = bp;	
				g->word_index_len++;

				if(0) {
				int l;
				fprintf(stderr,"------------------------------------\n");
				for(l = 0; l < g->word_index_len; l++)
				{
					fprintf(stderr,"          %5d: %s\n",l,g->word_index[l]->word);
				}
				fprintf(stderr,"------------------------------------\n");
				}
				}
					
		}

	}


}




TMError tm_topicmap_merge(TMTopicMap self, TMTopicMap other)
{
	assert(self);
	assert(other);

	/*

	- require models of other
	- walk all topics and test if merge with self or
	  add

	keep mergelist

 	assertions ??	

	*/

	return TM_OK;
}


#endif



TM tm_topicmap_get_tm(TMTopicMap self)
{
	assert(self);
	return self->tm;
}
