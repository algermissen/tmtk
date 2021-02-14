#ifndef TM_DEVEL_H
#define TM_DEVEL_H


/* some macros to simplify code */
/* Note that the macros contain returns! */


#define TM_GET_PARAM(tm,params,name,var) \
	do { \
		(var) = tm_params_get((params),(name)); \
        	if( ! (var) ) \
		{ \
			tm_set_error((tm), "%s is a required parameter", name ); \
			return (TM_E_PARAM); \
		} \
	} while(0)



#define GET_PROP(tm,propvar,m,modelname,propname) \
        do { \
        if( ( (propvar) = tm_model_get_property((m),modelname,propname)) == NULL) \
                { \
                        tm_set_error((tm),"property %s::%s not found", \
                                                        modelname, propname); \
                        return TM_E_PROP_NOT_FOUND; \
                } \
        } while(0)
#define TM_GET_PROPERTY(tm,propvar,m,modelname,propname) \
        do { \
        if( ( (propvar) = tm_model_get_property((m),modelname,propname)) == NULL) \
                { \
                        tm_set_error((tm),"property %s::%s not found", \
                                                        modelname, propname); \
                        return TM_E_PROP_NOT_FOUND; \
                } \
        } while(0)
 
#define TM_GET_TOPIC(topicmap,topicvar,modelname,propname,propvalue) \
        do { \
                TMError e; \
                if( (e=tm_topicmap_get_topic((topicmap), \
                        (modelname),(propname),(propvalue), \
                                (topicvar),NULL)) != TM_OK) \
                { \
                        return (e); \
                } \
                if(!*(topicvar)) \
                { \
                        tm_set_error( tm_topicmap_get_tm((topicmap)),"topic with %s::%s=%s not found", \
                                                        modelname, propname, propvalue); \
                        return (TM_E_TOPIC_NOT_FOUND); \
                } \
        } while(0)

#define TM_GET_PROPVAL(topicmap,topic,modelname,propname,propvalue,propvar) \
        do { \
                TMError e; \
                if( (e=tm_topicmap_get_property((topicmap),(topic), \
				(modelname),(propname),(propvalue),(propvar))) != TM_OK) \
                { \
                        return (e); \
                } \
        } while(0)

#endif
