/*
 * $Id$
 *
 * Copyright (c) 2002 Jan Algermissen
 * See the file "COPYING" for copying permission.
 *
 */
#ifndef OMNIVORE_INCLUDED
#define OMNIVORE_INCLUDED

/** \defgroup Omnivore 
 *
 * @{
 */


/** \typedef Omnivore
 * A class for omnivorous parsers.
 *
 * FIXME: this and that
 */
typedef struct Omnivore *Omnivore;


/** \typedef element
 * Element class.
 *
 * FIXME: make invisible outside omnivore.
 */
typedef struct element element;


#include <tm.h>
#include <tmrdfhandler.h>
#include <tmtopicmap.h>
#include <tmerror.h>
#include <tmassertion.h>
#include <tmmodel.h>


#ifdef __cplusplus
extern "C" {
#endif


typedef TMError(*AssertionHandler)(Omnivore ov, TMAssertion assertion);
typedef TMError(*BuiltInNodeHandler)(Omnivore ov, TMSubject subject);
typedef TMError(*AddPropertyHandler)(Omnivore ov, TMSubject subject, const char *modelname, const char *propname, const void *value);
typedef TMError(*MergeMapHandler)(Omnivore ov, const char *locator);




#include <tmprocmodel.h>

/** Create a new omnivorous parser.
 * Constructor for a new omnivorous parser
 * @return the parser instance
 */
TM_API(Omnivore) Omnivore_new(TM tm);

/** Prepare parser for parsing.
 *
 * @param mode processing mode
 * @param pm the processing model to be used
 * @param pm_arg generic arument to processing model
 * @param tm if the processing model needs it, you may pass in a TM object.
 * @return the parser instance
 */
TM_API(TMError) tm_omnivore_prepare(Omnivore self, const char *mode,const char *pm_name,void* pm_arg,TMTopicMap tm);

/** Delete an Omnivore.
 */
TM_API(void) Omnivore_delete(Omnivore *pself);

/** Set URI of processed document.
 */
TM_API(void) Omnivore_setDocUri(Omnivore ov,const char *uri);


/** Get the current error string.
 *
 */
TM_API(const char *) Omnivore_getErrorString(Omnivore ov);
TM_API(const char *) tm_omnivore_get_error(Omnivore ov);

TM_API(TMTopicMap) tm_omnivore_get_topicmap(Omnivore ov);


/** Specify the TMA to be used for parsing.
 * An omnivore will look for an appropriate processing model in this
 * TMA.
 */


TM_API(void) Omnivore_useModel(Omnivore self,TMModel model);
/* mime-type ?? */

/** Specify the processing model to be used for
 * parsing.
 *
 */
TM_API(TMError) tm_omnivore_set_procmodel(Omnivore self,const char *name);



/* ---------------------------------------------------------------------------
 * Set-Handler functions:
 * ---------------------------------------------------------------------------
 */

/** Register the callback function.
 *
 */
TM_API(void) Omnivore_setAssertionHandler(Omnivore ov,AssertionHandler h);


/** Register the callback function.
 *
 */
TM_API(void) Omnivore_setMergeMapHandler(Omnivore ov,MergeMapHandler h);


/** Register the callback function.
 *
 */
TM_API(void) Omnivore_setBuiltInNodeHandler(Omnivore ov,BuiltInNodeHandler h);


/** Register the callback function.
 *
 */
TM_API(void) Omnivore_setAddPropertyHandler(Omnivore ov,AddPropertyHandler h);


/** Set user data. 
 * Use this for access to data that is not local to handler code.
 */
TM_API(void) Omnivore_setUserData(Omnivore ov,void *userData);


/** Get user data.
 *
 */
TM_API(void*) Omnivore_getUserData(Omnivore ov);


/* ---------------------------------------------------------------------------
 * The follwoing functions are intended to be used by syntax processing models
 * ---------------------------------------------------------------------------
 */

/* A syntax processing model must use this function to have the omnivore
 * to call the appropriate callbacks. Among other things, omnivore will
 * lookup the 'same topic as demanded subject' assertion type for this
 * opperation
 */
TM_API(void) Omnivore_handleNodeDemander(Omnivore self, TMSubject demander_sbj, int flag);


/* A syntax processing model must call this function for a built in topic
 */
TM_API(void) Omnivore_handleBuiltInNode(Omnivore self, TMSubject sbj);


/* A syntax processing model must call this fucntion to add an assertion
 */
TM_API(void) Omnivore_handleAssertion(Omnivore self, TMAssertion assertion);
 

/* a syntax processing model must call this function to add a proerty to
 * a topic
 */
TM_API(void) Omnivore_handleAddProperty(Omnivore self, TMSubject sbj, const char *model, const char *prop, const void *value);


/** Construct Omnivore's error string.
 * 
 */
TM_API(const char*) Omnivore_setError(Omnivore ov,const char *fmt, ...);


/** Parse a chunk of data.
 * @param s the chunk to parse
 * @param len the length of the chunk
 * @param isFinal if true, parsing will stop.
 * @return [FIXME]
 */
TM_API(int) Omnivore_parse(Omnivore self, const char *s, size_t len, int isFinal);


TM_API(TM) tm_omnivore_get_tm(Omnivore self);



TM_API(const char *) tm_omnivore_set_error(Omnivore self, const char *fmt,...);
TM_API(const char*) elem_path(element *elem);
TM_API(const char*) elem_text(element *elem);
TM_API(const char*) elem_uri(element *elem);
TM_API(const char*) elem_ref_uri(element *elem);



/** @} */


#ifdef __cplusplus
} // extern C
#endif



#endif /* OMNIVORE_INCLUDED */
