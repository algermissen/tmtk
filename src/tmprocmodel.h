/*
 * $Id$
 *
 * Copyright (c) 2002 Jan Algermissen
 * See the file "COPYING" for copying permission.
 *
 */

#ifndef TM_PROCMODEL_INCLUDED
#define TM_PROCMODEL_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

/** \defgroup TMProcModel
 *
 * @{
 */


/** Processing Model Definition.
 *
 */
typedef struct TMProcModel *TMProcModel;

#include "tmtk.h" 
#include "tm.h" 
#include "tmerror.h" 
#include "tmassertion.h"
#include "tmlist.h"
#include "tmvaluetype.h"
#include "tmtopicmap.h"
#include "omnivore.h"


typedef TMError(*TMProcModelInitFunction)(Omnivore,void *arg,void **data);
typedef void(*TMProcModelDeleteFunction)(void**);

#include <tmrdfhandler.h>
struct rdf_tab {
	const char *predicate;
	TMRDFStatementHandler handler;
};

#include <omnivore.h>
/* Handler for markup processing */
/* FIXME: Omnivore and element dependecy and elemet type !! */
typedef int(*ElementStartEventHandler)(void *data,Omnivore ov, element *elem,const char **attr);
typedef int(*ElementEndEventHandler)(void *data,Omnivore ov, element *elem);
typedef int(*LineEventHandler)(void*, const char *line, int isFinal);

struct TMProcModel {
	const char *name;
	TMProcModelInitFunction init;
	TMProcModelDeleteFunction delete;
	const char *root_element_name;
	const char *namespace;
	const char *public_id;
	const char *system_id;
	/* const char **elements; -- unused*/ 
	char *id_attribute_name;
	char *ref_attribute_name;
	TMList ignore_elements;
	TMList text_elements;
	/*
	TMList targets;
	target *last_target;
	*/
	ElementStartEventHandler start_element;
	ElementEndEventHandler end_element;
	/*
	TMSubject at_demander_demanded;
	TMSubject role_demander;
	TMSubject role_demanded;
	*/
	LineEventHandler handle_line;
	struct rdf_tab *rdf_tab;
};


/** Check if a given XML attribute is defined as an ID attribute
 * by the defining processing model.
 * Omnivore uses this to construct element locators.
 */
TM_API(int) tm_procmodel_is_id_attribute(TMProcModel model, const char* attr);

/** Check if a given XML attribute is defined as a reference attribute
 * by the defining processing model.
 * Omnivore uses this to construct element references.
 */
TM_API(int) tm_procmodel_is_ref_attribute(TMProcModel model, const char* attr);

/** Check if a certain element is supposed to be ignored when
 * processing syntactical instances.
 *
 */
TM_API(int) tm_procmodel_ignore_element(TMProcModel model, const char* elem);

#if 0
==============================================================================
code for creating models from PMDL (not used)
enum { START_EVENT, END_EVENT };

typedef struct target {
	int event;
	const char *match;
} target;
OV_API(TMProcModel) model_create(const char* file);
OV_API(int) Model_startElement(Model model, element *elem, EmitAssertionHandler h);
OV_API(int)Model_endElement(Model model, element *elem, EmitAssertionHandler h);
OV_API(int) Model_hasTargetFor(Model model, ..element in full context..);
==============================================================================
#endif

/** @} */

#ifdef __cplusplus
}
#endif


#endif /* TM_PROCMODEL_INCLUDED */
