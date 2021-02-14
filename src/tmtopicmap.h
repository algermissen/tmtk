/*
 * $Id: topicmap.h,v 1.9 2002/10/02 22:06:25 jan Exp $
 *
 * Copyright (c) 2002 Jan Algermissen
 * See the file "COPYING" for copying permission.
 *
 */

#ifndef TM_TOPICMAP_H
#define TM_TOPICMAP_H

#ifdef __cplusplus
extern "C" {
#endif

/** \defgroup TopicMap
 *
 * @{
 */

/** \typedef TMTopicMap
 * Topic Map Class.
 * The core topic map object class.
 */
typedef struct TMTopicMap *TMTopicMap;

#include "tmtk.h"
#include "tmerror.h"
#include "tmparams.h"

/** FIXME.
 *
 */
typedef int(*TMViewStartEventHandler)(void* user_data, const char *name, void **atts);
/** FIXME.
 *
 */
typedef int(*TMViewEndEventHandler)(void* user_data, const char *name);

#include "tm.h"
struct view
{
        TM tm;
        TMTopicMap topicmap;
        void *user_data;
        TMViewStartEventHandler start;
        TMViewEndEventHandler end;
};


typedef TMError(*TMViewHandler)(struct view*,TMParams);

#include "tmassertion.h" 
#include "tmtopicset.h"
#include "tmcond.h"
#include "tmstorage.h"
#include "tmmodel.h"







typedef TMError(*TMModelLookupFunction)(TM tm,const char *name,TMModel *mp);


/** TopicMap constructor. 
 *
 * Create a new topicmap with a particular storage implementation.
 * @param sd the descriptor of the storage implementation
 * @return the topicmap object.
 */
TM_API(TMTopicMap) tm_topicmap_new(TM tm);


/** TopicMap destructor.
 * Destroy the topicmap object. This function (as all destructors) takes
 * a pointer to the object which will be set to NULL for additional
 * safety.
 * @param pself pointer to topicmap object
 *
 */
TM_API(void) tm_topicmap_delete(TMTopicMap *pself);

/** Set storage type.
 * FIXME
 */
TM_API(TMError) tm_topicmap_set_storage(TMTopicMap self,const char *storage_name);


/** Open the topicmap.
 * After creation, a topicmap has to be opened with the arguments
 * required by the storage implementation that has been choosen
 * when the topicmap was created. This might be a path in the case
 * of a file based storage implementation or stuff like connection
 * and authentification settings in case of an RDBMS implementation.
 * @param args generic argument passed to storage implementation
 * @return TM_OK or error code
 */
TM_API(TMError) tm_topicmap_open(TMTopicMap self, void *args);


/** Close the topicmap.
 * @return TM_OK or error code
 */
TM_API(TMError) tm_topicmap_close(TMTopicMap self);

/** Dump the topicmap.
 * @param file file to dump the topic map to
 */
TM_API(void) tm_topicmap_dump(TMTopicMap self, FILE *file);





/** Start a transaction.
 * [FIXME]
 *
 */
TM_API(TMError) tm_topicmap_start_transaction(TMTopicMap self);


/** Commit a transaction.
 * Rollback/Commit functionality is not yet implemented.
 * (calling this function does a full merge)
 *
 */
TM_API(TMError) tm_topicmap_end_transaction(TMTopicMap self);


/** Rollback a transaction.
 * Rollback/Commit functionality is not yet implemented.
 *
 */
TM_API(TMError) tm_topicmap_rollback_transaction(TMTopicMap self);


/** Fully merge the topic map.
 *  Explicitly trigger a full merge of the topic map.
 *  (currently implied by commiting a transaction)
 */
TM_API(TMError) tm_topicmap_fully_merge(TMTopicMap self);

/** Ensure that a certain model has been loaded.
 * Calling this function will ensure that a certain model and all
 * the models it requires itself are loaded in the topicmap.
 * @param model the model
 * @return TM_OK or error code
 */
TM_API(TMError) tm_topicmap_require_model(TMTopicMap self, const char *name);



/**Add a property/value pair to a topic.
 * This function is used to add a property/value pair to a topic.
 *
 * @param self the topicmap
 * @param topic the topic 
 * @param modelname the name of the topic map model that the property is a part of
 * @param propname the name of the property
 * @param value the value
 * @return TM_OK un success, error code otherwise
 */
TM_API(TMError) tm_topicmap_add_property_to_topic(TMTopicMap self, TMTopic topic,
        const char *modelname, const char *propname, const void *value);

/** FIXME.
 *
 */
TM_API(TMError) tm_topicmap_add_property_to_topic_from_string(TMTopicMap self, TMTopic topic,
        const char *modelname, const char *propname, const char *value);

/**Add a subject to the topic map and return the topic that is its
 * surrogate.
 * This function allways creates a new topic. It does not do any lookup
 * operations based on the properties of the subject object.
 *
 * Passing a NULL pointer will create an anonymous topic (one with no
 * properties). Maybe this functionality will be removed in the future
 * because anonymous topics are a dangerous thing (once you loose hold
 * of them, you cannot find them again!).
 *
 * @param self the topicmap
 * @param subject the subject to add
 * @param np pointer to topic; will be set to subject's surrogate
 * @return TM_OK on success, error code otherwise
 */
TMError tm_topicmap_add_subject(TMTopicMap self, TMSubject subject, TMTopic *np);

/** Add a topic-assertion to the topic map.
 * The structural abstraction of an assertion (A-topic with T-topic and
 * a list of memberships (R-topic,x-topic) is expressed in the TMM as a set
 * of properties (the TMM defined properties). This function will add these
 * properties to the topics in the topic-assertion structure.
 *
 * If the structure does not contain an A-topic, the function will create a
 * a new topic and put it in the structure. This enables the caller of the
 * function to get the topic that represents the assertion.
 *
 * Note: This function can be fully substituted by the appropriate calls to
 * tm_topicmap_add_property_to_topic() and in some sense, it is only prsent
 * for convenience.
 * @param self the topic map object
 * @param astn the assertion structure
 */
TM_API(TMError) tm_topicmap_add_topic_assertion(TMTopicMap self, TMTopicAssertion astn);



/** Get the value of a property of a given topic.
 * Value needs to be deallocated by caller!
 *
 * @param topic the topic
 * @param modelname model name portion of the property
 * @param propname property name
 * @param value pointer to value
 * @return TM_OK or error code
 */
TM_API(TMError) tm_topicmap_get_property(TMTopicMap self,
				      TMTopic topic,
				      const char *modelname,
				      const char *propname,
				      void **value,
				      TMProperty *pp);













/* BELOW HERE IS OLD FUNCTIONS ETC....... */

/* ========================================================================*/
/* ========================================================================*/
/* ========================================================================*/
/* ========================================================================*/
/* ========================================================================*/
/* ========================================================================*/
/* ========================================================================*/
/* ========================================================================*/
/* ========================================================================*/
/* ========================================================================*/
/* ========================================================================*/
/* ========================================================================*/



/** Lookup a topic by a given subject.
 *  SIDP required, one SIDP match is enough, no manipulation of the graph.
 *
 * @param subject the subject
 * @param np will be set to topic or 0 if not found
 * @return TM_OK or error code
 *
 */
TM_API(TMError) tm_topicmap_lookup_topic_by_subject(TMTopicMap self,
					TMSubject subject, TMTopic *np);


/** Lookup the topic that surrogates a certain TMA defined assertion type.
 *
 */
TM_API(TMError) tm_topicmap_lookup_at_topic(TMTopicMap self,
		const char *model_name, const char* at_name,TMTopic *np);


/** Lookup the topic that surogates a certain TMA defined role.
 *
 */
TM_API(TMError) tm_topicmap_lookup_role_topic(TMTopicMap self,
		const char *model_name, const char* role_name,TMTopic *np);


/** Lookup the topic that has a certain property value.
 * Expects property to be an SIDP, returns TM_E_SIDP_EXPECTED otherwise.
 * @param modelname	The name of the model
 * @param propname	The name of the property
 * @param value		The value 
 * @return 		TM_OK or error code
 */
TM_API(TMError) tm_topicmap_lookup_topic_by_property(TMTopicMap self,
		const char *modelname, const char *propname, const void *value,
		TMTopic *np);

/** Lookup the topic that has a certain property value.
 * Expects property to be an SIDP, returns TM_E_SIDP_EXPECTED otherwise.
 * Expects property to be normalized, return TM_E_PSTORE_NOT_NORMALIZED otherwise
 * FIXME: ambiguous lookup!
 * @param modelname	The name of the model
 * @param propname	The name of the property
 * @param value		The value 
 * @return 		TM_OK or error code
 */
TM_API(TMError) tm_topicmap_get_topic(TMTopicMap self,
		const char *modelname, const char *propname, const void *value,
		TMTopic *np, TMProperty *pp);

/** Lookup topics by property value.
 *
 * Lookup the set of topics that have a certain property value or
 * whose property value mactch a search expression.
 *
 * @param modelname	The name of the model
 * @param propname	The name of the property
 * @param value		The value or value search expression
 * @param flag		Flag indicating if an exact match of the
 *  			given value is desired or a search.
 *			Possible values for the flag are:
 * 			TM_LOOKUP_LIKE
 * 			TM_LOOKUP_EQUAL
 * @return 		Error code if an error occurred during
 *			storage access.
 *
 */
TM_API(TMError) tm_topicmap_lookup_topics_by_property(
					TMTopicMap self,
					const char *modelname,
					const char *propname,
					const void *value,
					int flag,
					TMTopicSet topicset);

















/** Add a built-in topic.
 *
 */
TM_API(TMError) tm_topicmap_add_built_in_topic(TMTopicMap self, TMSubject subject, TMTopic *np);


/**Add a property/value pair to a subject.
 * This function is used to add a property/value pair to a topic that is
 * a surrogate for the provided subject. The topic is required to be
 * already present in the topicmap. This does not mean however, that the
 * topic has exactly the given subject, only that the subject is sufficient
 * for looking up that topic. This function will also not cause a merge! 
 * ---> why not ???? <------
 * FIXME: this implies that there could be two+ topics returned !!
 *
 * /param self the topicmap
 * /param subject the subject
 * /param FIXME
 * /return TM_OK un success, error code otherwise
 */
TM_API(TMError) tm_topicmap_add_property(TMTopicMap self, TMSubject subject,
        const char *modelname, const char *propname, const void *value,TMTopic *np);





/** Add an assertion to the topic map
 *
 */
TM_API(TMError) tm_topicmap_add_assertion(TMTopicMap self, TMAssertion astn);












/** Get the list of 'loaded' models.
 *
 */
TM_API(TMError) tm_topicmap_get_models(TMTopicMap self,TMList *listp);


/* Export Topic Map as canonical XML.
 *
 */
TMError tm_topicmap_xml_export(TMTopicMap self, FILE *f);


/* Merge two topic maps */
TM_API(TMError) tm_topicmap_merge(TMTopicMap self, TMTopicMap other);


#if 0
/* FIXME: this is should be static in topicmap, but I need it in models */
TM_API(TMError) _add_property(TMTopicMap self, TMTopic topic,
                const char *modelname, const char *propname, const void *value, TMTopic *p_changed_topic);
#endif 

/** \typedef TMTopicScan
 * Topic scan handle.
 *  FIXME
 */
typedef struct TMTopicScan *TMTopicScan;

/** Open a topic scan with a single condition.
 * FIXME
 */
TM_API(TMError) tm_topicmap_topic_scan_open_with_single_condition(TMTopicMap self, const char *modelname, const char *propname, const char *op, const char *argstring, TMTopicScan *scanp);
/** Open a topic scan with a double condition.
 * FIXME
 */
TM_API(TMError) tm_topicmap_topic_scan_open_with_double_condition(TMTopicMap self,
	const char *modelname1, const char *propname1, const char *op1, const char *argstring1,
	const char *operator,
	const char *modelname2, const char *propname2, const char *op2, const char *argstring2,

	TMTopicScan *scanp);

/** .
 *
 */
TM_API(TMError) tm_topicmap_topic_scan_close(TMTopicMap self, TMTopicScan scan);
/** .
 *
 */
TM_API(int) tm_topicmap_topic_scan_finished(TMTopicMap self, TMTopicScan scan);
/** FIXME.
 *
 */
TM_API(TMError) tm_topicmap_topic_scan_fetch(TMTopicMap self, TMTopicScan scan, TMTopic *tp);



/** Query the topic map.
 * Currently there is no query language for TmTk, instead a different
 * approach has been taken which is named 'parameterized views'. By calling
 * this method, retrieval of a certain view is initiated on the topic map. There
 * is no direct result, instead user-supplied callback-functions will be called
 * similar to the process of parsing an XML document with a SAX parser.
 * \see { tm_topicmap_set_view_start_event_handler }
 * \param self the topic map
 * \param query the query string
 * \return TM_OK on success, error code otherwise 
 */
TM_API(TMError) tm_topicmap_query(TMTopicMap self, void *user_data, TMViewStartEventHandler start, TMViewEndEventHandler end, const char* query);


TM_API(TM) tm_topicmap_get_tm(TMTopicMap self);




/** @} */


/* callback for normalization code in property stores */
TM_API(void) tm_topicmap_add_mergelist(TMTopicMap self, TMTopic *array, int len);


#ifdef __cplusplus
} // extern "C"
#endif

#endif


