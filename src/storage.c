#include "tmstorage.h"
#include "tmassert.h"


/* =========================================================================
 
                           TMMetaStore 
 
   ========================================================================= */

TMMetaStore tm_meta_store_new(TMTopicMapStorageDescriptor desc, void *handle)
{
	TMMetaStore metastore;

	metastore = desc->meta_store_type->new(handle);
	return metastore;
}
TMError tm_meta_store_open(TMMetaStore self)
{
	assert(self);
        return self->type->open(self);
}
TMError tm_meta_store_close(TMMetaStore self)
{
	assert(self);
        return self->type->close(self);
}
void tm_meta_store_delete(TMMetaStore self)
{
	assert(self);
        self->type->delete(self);
}

TMError tm_meta_store_get_model_list(TMMetaStore self, TMList *ptr)
{
	assert(self);
        return (self->type->get_model_list(self,ptr));
}
TMError tm_meta_store_add_model(TMMetaStore self,const char*name)
{
	assert(self);
        return (self->type->add_model(self,name));
}
TMError tm_meta_store_get_new_topic(TMMetaStore self,TMTopic *np)
{
	assert(self);
        return (self->type->get_new_topic(self,np));
}
TMError tm_meta_store_dispose_topic(TMMetaStore self,TMTopic topic)
{
	assert(self);
        return (self->type->dispose_topic(self,topic));
}
TMError tm_meta_store_get_topicmax(TMMetaStore self,TMTopic *topicmax)
{
	assert(self);
        return (self->type->get_topicmax(self,topicmax));
}

/* =========================================================================
 
                           TMPropertyStore 
 
   ========================================================================= */

TMPropertyStore tm_property_store_new(TMTopicMapStorageDescriptor desc, void *handle,TMProperty prop, TMValueStore vstore)
{
	TMPropertyStore propertystore;

	propertystore = desc->property_store_type->new(handle,prop,vstore);
	return propertystore;
}
TMError tm_property_store_open(TMPropertyStore self)
{
	assert(self);
        return self->type->open(self);
}
TMError tm_property_store_close(TMPropertyStore self)
{
	assert(self);
        return self->type->close(self);
}
void tm_property_store_delete(TMPropertyStore self)
{
	assert(self);
        self->type->delete(self);
}


TMError tm_property_store_insert(TMPropertyStore self, TMTopic topic, TMVid vid)
{
	assert(self);
        return (self->type->insert(self,topic,vid));
}
TMError tm_property_store_remove(TMPropertyStore self, TMTopic topic)
{
	assert(self);
        return (self->type->remove(self,topic));
}
TMError tm_property_store_lookup_topic(TMPropertyStore self, TMVid vid, TMTopic *np)
{
	assert(self);
        return (self->type->lookup_topic(self,vid,np));
}
TMError tm_property_store_lookup_topics(TMPropertyStore self, TMVid vid, TMTopic **topicsp, int *lenp)
{
	assert(self);
        return (self->type->lookup_topics(self,vid,topicsp,lenp));
}

TMError tm_property_store_lookup_vid(TMPropertyStore self, TMTopic topic, TMVid *vidp)
{ /* FIXME: delete */
	assert(self);
	assert(0);
        return (self->type->get_vid_of_topic(self,topic,vidp));
}
TMError tm_property_store_get_vid_of_topic(TMPropertyStore self, TMTopic topic, TMVid *vidp)
{
	assert(self);
        return (self->type->get_vid_of_topic(self,topic,vidp));
}
TMError tm_property_store_print(TMPropertyStore self, FILE *f)
{
	if(self->type->print)
		return (self->type->print(self,f));
	else
		return TM_OK;
}
TMError tm_property_store_is_normalized(TMPropertyStore self)
{
	return (self->type->is_normalized(self));
}
TMError tm_property_store_normalize(TMPropertyStore self,TMTopicMap topicmap)
{
	return (self->type->normalize(self,topicmap));
}

/* =========================================================================
 
                             TMValueStore
 
   ========================================================================= */
TMValueStore tm_value_store_new(TMTopicMapStorageDescriptor desc, void *handle,TMProperty prop)
{
	TMValueStore valuestore;
	TMValueStoreType *p,store_type = NULL;
	const char *vtype_name = prop->value_type->name;

	for(p = desc->valuestore_types; p && *p; p++)
	{
		if(strcmp( (*p)->valuetype->name,vtype_name)  == 0)
		{
			store_type = *p;
			break;
		}
	}
	if(!store_type)
		return NULL;

	/* FIXME: here we can pass parameters later on */
	valuestore = store_type->new(handle,prop,NULL);
	return valuestore;
}
TMError tm_value_store_open(TMValueStore self)
{
	assert(self);
        return self->type->open(self);
}
TMError tm_value_store_close(TMValueStore self)
{
	assert(self);
        return self->type->close(self);
}
void tm_value_store_delete(TMValueStore self)
{
	assert(self);
        self->type->delete(self);
}



TMError tm_value_store_lookup_vid(TMValueStore self,const void *v,int flag,TMVid *pvid)
{
	assert(self);
        return (self->type->lookup_vid(self,v,flag,pvid));
}


TMError tm_value_store_delete_value(TMValueStore self,TMVid vid)
{
	assert(self);
        return (self->type->delete_value(self,vid));
}

TMError tm_value_store_get_value(TMValueStore self,TMVid vid, void** vp)
{
	assert(self);
        return (self->type->get_value(self,vid,vp) );
}
TMError tm_value_store_print(TMValueStore self, FILE *f)
{
	if(self->type->print)
		return (self->type->print(self,f));
	else
		return TM_OK;
}

TMError tm_value_store_scan_open(TMValueStore self, int opcode, const void *arg, TMValueStoreScan *sp)
{
	TMError e;
	assert(self);
	TMTRACE(TM_STORAGE_TRACE,"enter\n");
	assert(self->type->scan_open);
        e = self->type->scan_open(self,opcode,arg,sp);
	TMTRACE(TM_STORAGE_TRACE,"exit\n");
	return e;
}
TMError tm_value_store_scan_close(TMValueStore self,TMValueStoreScan s)
{
	assert(self);
	TMTRACE(TM_STORAGE_TRACE,"enter\n");
	assert(self->type->scan_close);
        return self->type->scan_close(self,s);
}
int tm_value_store_scan_finished(TMValueStore self,TMValueStoreScan s)
{
	assert(self);
	assert(s);
	TMTRACE(TM_STORAGE_TRACE,"enter/exit\n");
	return (s->next_vid == 0);
}
TMError tm_value_store_scan_fetch(TMValueStore self,TMValueStoreScan s, TMVid *vidp)
{
	assert(self);
	assert(s);
	TMTRACE(TM_STORAGE_TRACE,"enter\n");
	if( s->next_vid == 0)
	{
		assert(0); /* fixme: return error */
	}
	*vidp = s->next_vid;
	TMTRACE(TM_STORAGE_TRACE,"fetching vid=%d\n" _ s->next_vid);

        self->type->scan_prepare_next(self,s);
	TMTRACE(TM_STORAGE_TRACE,"  pre-called 'prepare'.....next will be vid=%d\n" _ s->next_vid);
	/* FIXME: error check */
	TMTRACE(TM_STORAGE_TRACE,"exit\n");
	return TM_OK;
}

#if 0
/* =========================================================================
 
                           TMAssertionStore 
 
   ========================================================================= */

TMError tm_assertion_store_print(TMAssertionStore self, FILE *f)
{
	if(self->type->print)
		return (self->type->print(self,f));
	else
		return TM_OK;
}

TMTopic tm_assertion_store_get_type(TMAssertionStore self)
{
	assert(self);
        return (self->type->get_type(self));
}

TMError tm_assertion_store_insert(TMAssertionStore self,TMTopicAssertion a)
{
	assert(self);
        return (self->type->insert(self,a));
}

TMError tm_assertion_store_cursor_init(TMAssertionStore self, void **statep, TMCond condition)
{
	assert(self);
	return (self->type->cursor_init(self,statep,condition));	
}

TMError tm_assertion_store_cursor_has_more(TMAssertionStore self, void *state, TMTopicAssertion a, int *morep)
{
	assert(self);
	return (self->type->cursor_has_more(self,state,a,morep));	
}
TMError tm_assertion_store_cursor_cleanup(TMAssertionStore self, void *state)
{
	assert(self);
	return (self->type->cursor_cleanup(self,state));	
}

TMError tm_assertion_store_topic_replace(TMAssertionStore self, TMTopic stays, TMTopic disappears, TMStack stays_stack, TMStack disappears_stack)
{
	assert(self);
	return (self->type->topic_replace(self,stays,disappears,stays_stack, disappears_stack));	
}
#endif
/* =========================================================================
 

 
   ========================================================================= */
