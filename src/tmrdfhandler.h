/*
 * $Id$
 *
 * Copyright (c) 2002 Jan Algermissen
 * See the file "COPYING" for copying permission.
 *
 */

#ifndef TM_RDFNSHANDLER_INCLUDED
#define TM_RDFNSHANDLER_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif


#include <omnivore.h>
typedef struct TMRDFStatement *TMRDFStatement;
typedef struct TMRDFNamespaceHandler *TMRDFNamespaceHandler;
typedef TMError(*TMRDFStatementHandler)(void *,Omnivore,TMRDFStatement);



struct TMRDFStatement {
        const void *subject;
        /*
          raptor_identifier_type subject_type;
        */
        const void *predicate;
        /*
        raptor_identifier_type predicate_type;
        */
        const void *object;
        /*
        raptor_identifier_type object_type;
        */
        /*
        raptor_uri *object_literal_datatype;
        */
        /*
        const unsigned char *object_literal_language;
        */
};

/** Get handler by RDF predicate URI.
 *
 */
/*
TM_API(TMRDFStatementHandler) tm_rdf_namespace_handler_get_statement_handler(
        TMRDFNamespaceHandler self,
        const char *predicate);
*/

/*
struct _bucket {
	const char *uri;
	TMRDFStatementHandler handler;
};
*/
/*
struct TMRDFNamespaceHandler {
	void **handlers;	
};
*/



#ifdef __cplusplus
}
#endif


#endif /* TM_RDFNSHANDLER_INCLUDED */
