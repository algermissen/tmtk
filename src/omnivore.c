/*
 * $Id$
 *
 * Copyright (c) 2002 Jan Algermissen
 * See the file "COPYING" for copying permission.
 *
 */

#include <tmmodel.h>
#include <omnivore.h>
#include <tmtrace.h>
#include <tmutil.h>
#include <tmmem.h>
#include <tmassert.h>
#include <tmrdfhandler.h>
#include <tmtable.h>


#ifdef HAVE_LIBEXPAT
#include <expat.h>
#endif

#ifdef HAVE_LIBRAPTOR
#include <raptor.h>
#endif

/* some defines, all is FIXME (review!) */
#define MAXELEMENT 256
#define MAXURI 1024
#define MAXID 256
#define MAXITEMS 20
#define MAXID 256
#define MAXDATA 16024
#define MAX_CHILDS 10
#define MAX_NAME_SIZE 256


#define OV_ERROR_OCCURRED(ov) ( *((ov)->errstr) != '\0' )


struct child_count
{
	char name[MAX_NAME_SIZE];
	int count;
};


/* -------------------------------------------------------------------------
 * Element struct.
 * 
 *
 * ------------------------------------------------------------------------- */
struct element
{
        /* FIXME.. use pointer into NS table here for all portions */
        char namespace[MAXELEMENT];
        char name[MAXELEMENT];
        char uri[MAXURI];
        char ref_uri[MAXURI];
        char path[MAXURI];
        char id[MAXID];
        char text[MAXDATA];
        char text_uri[MAXURI];
	struct child_count per_child_counts[MAX_CHILDS];
};
const char* elem_path(element *elem) { return elem->path; }
const char* elem_text(element *elem) { return elem->text; }
const char* elem_uri(element *elem) { return elem->uri; }
const char* elem_ref_uri(element *elem) { return elem->ref_uri; }

/* our built-in line oriented parser */
typedef struct TMLineParser *TMLineParser;
static TMLineParser tm_line_parser_new(Omnivore om);
static void tm_line_parser_delete(TMLineParser *pself);
static const char *tm_line_parser_get_error(TMLineParser self);
static int tm_line_parser_get_current_line_number(TMLineParser self);
static int tm_line_parser_get_current_char_number(TMLineParser self);
static int tm_line_parser_handle_chunk(TMLineParser self,const char* s, int len, int isFinal);


static int _get_childcount(struct element *ep, const char *name,int do_increment);


struct Omnivore
{
	TM tm;
        const char *mode;		/* XML, RDF, etc.. */
	void *parser;			/* expat or raptor */
        int parse_start;		/* needed for raptor's parse_chunk */
        char errstr[2048];		/* holds error string */
        char doc_uri[MAXURI];		/* holds original document URI */
        char current_base_uri[MAXURI];	/* holds current base URI */
        char effective_doc_uri[MAXURI];	/* holds current effective doc URI */
        char default_namespace_uri[MAXURI]; /* ?FIXME? */
 
 
        element element_stack[MAXITEMS];
        int elem_index;
 
        int depth;
        int ignore_depth;
 
        void* user_data;


        AssertionHandler assertion_handler;
        MergeMapHandler merge_map_handler;
        BuiltInNodeHandler built_in_topic_handler;
        AddPropertyHandler add_property_handler;
 
        TMProcModel model;
	void *proc_model_data;
 
        TMModel tm_model;
	TMTopicMap topicmap;
};
/* -------------------------------------------------------------------------
 *
 * Declarations of expat/raptor callbacks.
 *
 * ------------------------------------------------------------------------- */
#ifdef HAVE_LIBRAPTOR
/*
static TMRDFStatementHandler
_find_rdf_handler(Omnivore self,const char *predicate);
*/

static void
_raptor_statement_hdl(void *user_data, const raptor_statement *statement);

static void
_raptor_error_hdl(void *user_data, raptor_locator* loc, const char *message);

static void
_raptor_fatal_error_hdl(void *udata, raptor_locator* loc, const char *message);

static void
_raptor_warning_hdl(void *udata, raptor_locator* loc, const char *message);
#endif

#ifdef HAVE_LIBEXPAT
static void expat_start(void *data, const char *el, const char **attr);
static void expat_end(void *data, const char *el);
static void expat_char_data(void *user_data, const XML_Char *data, int len);
static void expat_ns_start(void *data, const char *prefix, const char *uri);
static void expat_ns_end(void *data, const char *prefix);
#endif

/* -------------------------------------------------------------------------
 *
 *
 *
 * ------------------------------------------------------------------------- */
Omnivore Omnivore_new(TM tm)
{
        Omnivore self;
	TM_NEW(self);
	assert(self);
 
	bzero(self,sizeof(*self));
	self->tm = tm;
	self->topicmap = NULL;
	self->mode 			= 0;
	self->user_data 			= NULL;
	self->assertion_handler 		= NULL;
	self->built_in_topic_handler 	= NULL;
	self->add_property_handler 	= NULL;
	self->merge_map_handler 		= NULL;
	self->parse_start			= 1;		/* for raptor chunks */

	self->model = NULL;

	bzero(self->doc_uri,sizeof(self->doc_uri));
	bzero(self->default_namespace_uri,sizeof(self->default_namespace_uri));
	bzero(self->errstr,sizeof(self->errstr));
	bzero(self->element_stack,sizeof(self->element_stack));
	self->elem_index = 0;
	self->depth = 0;
	self->ignore_depth = 0;
	self->tm_model = NULL;

	self->proc_model_data = NULL;

	return self;
}

TMError tm_omnivore_prepare(Omnivore self, const char *mode,const char *pm_name,void* pm_arg,TMTopicMap tm)
{
	TMError e;
	assert(pm_name);

	self->topicmap = tm;
	self->mode = mode;
	self->parse_start = 1;		/* for raptor chunks */

	if( tm_omnivore_set_procmodel(self,pm_name) != TM_OK)
		return TM_FAIL;
	assert(self->model);

	bzero(self->doc_uri,sizeof(self->doc_uri));
	bzero(self->default_namespace_uri,sizeof(self->default_namespace_uri));
	bzero(self->element_stack,sizeof(self->element_stack));
	self->elem_index = 0;
	self->depth = 0;
	self->ignore_depth = 0;
	self->tm_model = NULL;

	if( (e = self->model->init(self,pm_arg, &(self->proc_model_data))) != TM_OK)
	{
#if 0
		actually stupid! There isn;t a parser yet, so aposition is not there
..... fix all this.
		assert(*(self->errstr)); /* make sure PM has set error string */
#endif
		return e;
	}

	if( strcmp(self->mode,"lines") == 0)
	{
		if(self->model->handle_line == NULL)
		{
			tm_set_error(self->tm,
			"processing model '%s' has no line parsing handler",
			self->model->name);
			/* FIXME: cleanup */
			return TM_FAIL;
		}
		/* this parser works on the TM directly ! */
		self->parser = tm_line_parser_new(self);
	}

#ifdef HAVE_LIBEXPAT
	else if( strcmp(self->mode,"xml") == 0)
	{
		if(self->model->start_element == NULL)
		{
			tm_set_error(self->tm,
			"processing model '%s' has no XML start element handler",
			self->model->name);
			/* FIXME: cleanup */
			return TM_FAIL;
		}
		if( (self->parser = XML_ParserCreateNS(NULL, NS_DELIM)) == NULL)
		{
			free(self);
			return TM_ESYS;
		}
 
		XML_SetUserData((XML_Parser)self->parser, (void *) self);
		XML_SetElementHandler((XML_Parser)self->parser, expat_start, expat_end);
		XML_SetNamespaceDeclHandler((XML_Parser)self->parser, expat_ns_start, expat_ns_end);
		XML_SetCharacterDataHandler((XML_Parser)self->parser, expat_char_data);
	}
#endif

#ifdef HAVE_LIBRAPTOR
	else if(strcmp(self->mode,"rdfxml") == 0)
	{
		/* FIXME: assure that this is only called once per process ?? */
		raptor_init();
		if(self->model->rdf_tab == NULL)
		{
			tm_set_error(self->tm,
			"processing model '%s' has no RDF statement handlers",
			self->model->name);
			/* FIXME: cleanup */
			return TM_FAIL;
		}


		self->parser = raptor_new_parser("rdfxml");
		raptor_set_feature((raptor_parser*)self->parser,RAPTOR_FEATURE_ASSUME_IS_RDF,1);
		raptor_set_feature((raptor_parser*)self->parser,RAPTOR_FEATURE_ALLOW_NON_NS_ATTRIBUTES,1);
		raptor_set_statement_handler((raptor_parser*)self->parser,self,_raptor_statement_hdl);
		raptor_set_fatal_error_handler((raptor_parser*)self->parser,self,_raptor_fatal_error_hdl);
		raptor_set_error_handler((raptor_parser*)self->parser,self,_raptor_error_hdl);
		raptor_set_warning_handler((raptor_parser*)self->parser,self,_raptor_warning_hdl);
	}
	else if(strcmp(self->mode,"rdfntriples") == 0)
	{
		assert(!"not yet implemented");
		raptor_init();
		self->parser = raptor_new_parser("ntriples"); 
		/* FIXME: need to continue implementation */
		assert(!"rdfntriples not yet implemented");
	}
#endif
	else
	{
		tm_set_error(self->tm,"unsupported parse mode: %s", self->mode);
		return TM_FAIL;
	}
	return TM_OK;;
}
	
void Omnivore_delete(Omnivore *pself)
{
	Omnivore self;
	assert(pself && *pself);
	self = *pself;
	
	if( strcmp(self->mode,"lines") == 0)
	{
		tm_line_parser_delete((TMLineParser*)&(self->parser) );
	}
	
#ifdef HAVE_LIBEXPAT
	else if(strcmp(self->mode,"xml") == 0)
	{
		free((XML_Parser)self->parser);
	}
#endif

#ifdef HAVE_LIBRAPTOR
	else if( (strcmp(self->mode,"rdfxml") == 0) || (strcmp(self->mode,"rdfntriples")))
	{
		raptor_free_parser((raptor_parser*)self->parser);
       		raptor_finish();
	}
#endif
	else
	{
		tm_omnivore_set_error(self,"parse mode %s not supported (propably the parser you expected is not present)", self->mode);
	}
	/* cleanup processing model data */
	self->model->delete( &(self->proc_model_data) );
}


const char * Omnivore_getErrorString(Omnivore self)
{
	/* FIXME: deprecated */
	assert(self);
	return self->errstr;
}
	

void Omnivore_setDocUri(Omnivore ov,const char *uri)
{
	assert(ov);
	assert(uri);

	assert(!*ov->doc_uri);
	assert(!*ov->effective_doc_uri);
	assert(!*ov->current_base_uri);
	tm_uri_normalize(uri,ov->doc_uri,sizeof(ov->doc_uri));
	/* FIXME: use malloc */
	strcpy(ov->effective_doc_uri,ov->doc_uri);
}

void Omnivore_useModel(Omnivore self,TMModel model)
{
	assert(self);
	assert(model);
	self->tm_model = model;
}

void Omnivore_setUserData(Omnivore self,void *userData)
{
	assert(self);
	self->user_data = userData;
}
void* Omnivore_getUserData(Omnivore self)
{
	assert(self);
	return self->user_data;
}

void Omnivore_setAssertionHandler(Omnivore self,AssertionHandler h)
{
	assert(self);
	self->assertion_handler = h;
}
void Omnivore_setMergeMapHandler(Omnivore self,MergeMapHandler h)
{
	assert(self);
	self->merge_map_handler = h;
}
void Omnivore_setBuiltInNodeHandler(Omnivore self,BuiltInNodeHandler h)
{
	assert(self);
	self->built_in_topic_handler = h;
}
void Omnivore_setAddPropertyHandler(Omnivore self,AddPropertyHandler h)
{
	assert(self);
	self->add_property_handler = h;
}

static char *_no_doc_uri = "http://NO_DOC_URI";
int Omnivore_parse(Omnivore self, const char *s, size_t len, int isFinal)
{
	assert(self);
	if(*(self->doc_uri) == '\0')
		Omnivore_setDocUri(self,_no_doc_uri);

	if(strcmp(self->mode,"lines") == 0)
	{
		assert(self->parser);
		if( ! tm_line_parser_handle_chunk((TMLineParser)self->parser,s,len,isFinal) )
		{
			tm_set_error(self->tm,
			"parse error: %s in line %d, pos %d",
			tm_line_parser_get_error((TMLineParser)self->parser),
			tm_line_parser_get_current_line_number((TMLineParser)self->parser),
			tm_line_parser_get_current_char_number((TMLineParser)self->parser)
			);
			return (0);
		}
		if( OV_ERROR_OCCURRED(self) )
			return 0;

	}

#ifdef HAVE_LIBEXPAT
	else if(strcmp(self->mode,"xml") == 0)
	{
		if( ! XML_Parse((XML_Parser)self->parser,s,len,isFinal) )
		{
			assert( ! OV_ERROR_OCCURRED(self) );  /* is that a correct assumption? */
/* FIXME: if error and syntax error the assertion fails!!! FIX THAT ! */
			
			tm_set_error(self->tm,
			"XML parsing error in line %d: %s",
			XML_GetCurrentLineNumber((XML_Parser)self->parser),
			XML_ErrorString(XML_GetErrorCode((XML_Parser)self->parser)) );
			return 0;
		}
		if( OV_ERROR_OCCURRED(self) )
			return 0;
	}
#endif

#ifdef HAVE_LIBRAPTOR
	else if( (strcmp(self->mode,"rdfxml") == 0) || (strcmp(self->mode,"rdfntriples")))
	{
		TMTRACE(TM_OMNIVORE_TRACE,"start parsing RDF (XML)\n");
		if(self->parse_start)
		{
			raptor_start_parse((raptor_parser*)self->parser,raptor_new_uri(self->doc_uri) );
		}
		/* NOTE: raptor returns 0 for ok */
		if(  raptor_parse_chunk((raptor_parser*)self->parser,(const unsigned char*)s,len,isFinal))
		{
			tm_omnivore_set_error(self,"parse error");
			return 0;
		}
		if( OV_ERROR_OCCURRED(self) )
			return 0;
	}
#endif

	else
	{
		tm_omnivore_set_error(self,"parse mode %s not supported (propably the parser you expected is not present)", self->mode);
		return (0);
	}
	self->parse_start = 0;

	return 1;
}




/* -------------------------------------------------------------------------
 *
 *                   Functions for processing models
 *
 * ------------------------------------------------------------------------- */

void Omnivore_handleNodeDemander(Omnivore self, TMSubject demander_sbj, int flag)
{
	assert(0);
#if 0
	TMList sirs = NULL;
	struct TMSubject demanded_sbj;
	struct TMSubject a_sbj;
	struct TMAssertion at;
 
	if(! self->built_in_topic_handler)
		return;

	if(self->built_in_topic_handler)
		self->built_in_topic_handler(self,demander_sbj);

	/* FIXME: this is SAM dependent, that is why we assert that here! */
	assert(strcmp(demander_sbj->props[0].model,"sam") == 0);
	assert(strcmp(demander_sbj->props[0].name,"SubjectAddress") == 0);

	demanded_sbj.N = 1;
	demanded_sbj.props[0].model = demander_sbj->props[0].model;
	demanded_sbj.props[0].name = "SubjectIndicators";
	sirs = tm_list_push(sirs,demander_sbj->props[0].value);
	demanded_sbj.props[0].value = sirs;


	if(self->built_in_topic_handler)
		self->built_in_topic_handler(self,&demanded_sbj);


	if(flag == TM_DEMAND_EXPLICIT)
	{
	/*
	 * Now the assertion:
	 */
	at.type = self->tm_model->at_demander_demanded; 
	assert(at.type);


	a_sbj.N = 1;
	a_sbj.props[0].model = self->tm_model->name;	
	a_sbj.props[0].name = self->tm_model->topic_demander_property->name;	
	a_sbj.props[0].value = self->doc_uri;	

	at.self = &a_sbj;

	at.N = 2;	/* at-demander=-demanded are constrained
			 * by tmtk to be binary assertions */

	at.memberships[0].role    = self->tm_model->role_demander; 
	at.memberships[0].casting = NULL; /* never present */
	at.memberships[0].player  = demander_sbj;

	at.memberships[1].role    = self->tm_model->role_demanded; 
	at.memberships[1].casting = NULL; /* never present */
	at.memberships[1].player  = &demanded_sbj; 


	assert(self->assertion_handler);
	if(self->assertion_handler)
		self->assertion_handler(self,&at);

	}
	tm_list_delete(&sirs);

	if(flag == TM_DEMAND_EXPLICIT)
	{
		;
	}
	else
	{
		;	
	}
#endif /* FIXME */
}

void Omnivore_handleAddProperty(Omnivore self, TMSubject sbj, const char *model, const char *prop,
		const void *value)
{
	TMError e;
	assert(self);
	assert(sbj);
	assert(model);
	assert(prop);
	assert(value);
	 
	if(self->add_property_handler)
	{
		if( (e = self->add_property_handler(self,sbj,model,prop,value)) != TM_OK)
		{
			/* if callback function did not set an errstring, we need to do
			 * that here to abort Omnivore
			 */
			if(! *self->errstr)
			{
				snprintf(self->errstr,sizeof(self->errstr),"XX %s", tm_strerror(e) );
			}
		}
	}
}
void Omnivore_handleAssertion(Omnivore self, TMAssertion assertion)
{
	TMError e;
	assert(self);
	assert(assertion);
	 
	if(self->assertion_handler)
	{
		if( (e = self->assertion_handler(self,assertion)) != TM_OK)
		{
			/* if callback function did not set an errstring, we need to do
			 * that here to abort Omnivore
			 */
			if(! *self->errstr)
			{
				snprintf(self->errstr,sizeof(self->errstr),"Adding of Assertion failed: %s", tm_strerror(e) );
			}
		}
	}
}
void Omnivore_handleBuiltInNode(Omnivore self, TMSubject sbj)
{
        TMError e;
        assert(self);
        assert(sbj);
 
        if(self->built_in_topic_handler)
	{
                if( (e = self->built_in_topic_handler(self,sbj)) != TM_OK)
		{
			/* if callback function did not set an errstring, we need to do
			 * that here to abort Omnivore
			 */
			if(! *self->errstr)
			{
				snprintf(self->errstr,sizeof(self->errstr),"Adding of BuiltInNode failed: %s", tm_strerror(e) );
			}
		}
	}
}

TMTopicMap omnivore_get_topicmap(Omnivore self)
{
	assert(self);
	return (self->topicmap);
}

void omnivore_set_topicmap(Omnivore self, TMTopicMap tm)
{
	assert(self);
	assert(tm); /* FIXME: remove later */
	self->topicmap = tm;
}

TMError tm_omnivore_set_procmodel(Omnivore self,const char *name)
{
	TMProcModel pm;
	TMError e;

	assert(self);
	assert(name);
	self->model = NULL;
	if( (e = tm_disclosureframework_lookup_procmodel(
		tm_get_disclosureframework(self->tm),
			self->tm,name,&pm)) != TM_OK)
	{
		return e;
	}
	
	self->model = pm;
	return TM_OK;
}



/* -------------------------------------------------------------------------
 *
 *                              raptor handlers
 *
 * ------------------------------------------------------------------------- */
#ifdef HAVE_LIBRAPTOR

static void _raptor_statement_hdl(void *user_data, const raptor_statement *statement)
{
	struct TMRDFStatement tm_st;
	struct rdf_tab *rt;
	int found = 0;
	Omnivore self = (Omnivore)user_data;

	if(OV_ERROR_OCCURRED(self))
		return;

	TMTRACE(TM_OMNIVORE_TRACE,"enter\n");

	TMTRACE(TM_OMNIVORE_TRACE,"RDF statement: %s %s %s\n" _
		statement->subject _
		statement->predicate _
		statement->object);

	/* Using our own struct to decouple omnivore implementation from raptor! */
	tm_st.subject   = statement->subject;	
	tm_st.predicate = statement->predicate;	
	tm_st.object    = statement->object;	
	/* try to find handler in procmodel RDF handler table */

	/* FIXME: report error if rdf_tab == NULL -> no RDF handler */
	for(rt=self->model->rdf_tab; rt && rt->predicate; rt++)
	{
		if(strcmp(rt->predicate,tm_st.predicate) == 0)
		{
			TMError e;
			if( (e = rt->handler(self->proc_model_data,self,&tm_st)) != TM_OK)
			{
				/* make sure proc model has set error */
				assert(OV_ERROR_OCCURRED(self));
				return;
			}
			found = 1;
			break;
		}

	}
	if(!found)
	{
		
		tm_log(self->tm,TM_LOG_WARNING,"Unable to process RDF statement, no handler for predicate %s found.",statement->predicate );
		tm_log(self->tm,TM_LOG_DEBUG,"RDF statement: S(%s) P(%s) O(%s)" ,
			statement->subject ,
			statement->predicate ,
			statement->object);
	}

	
	TMTRACE(TM_OMNIVORE_TRACE,"exit\n");
	
}

static void _raptor_error_hdl(void *user_data, raptor_locator* locator, const char *message)
{
	Omnivore self = (Omnivore)user_data;
	tm_omnivore_set_error(self,"raptor error: %s", message);
}
static void _raptor_fatal_error_hdl(void *user_data, raptor_locator* locator, const char *message)
{
	Omnivore self = (Omnivore)user_data;
	tm_omnivore_set_error(self,"raptor fatal error: %s", message);
}
static void _raptor_warning_hdl(void *user_data, raptor_locator* locator, const char *message)
{
	Omnivore self = (Omnivore)user_data;
	tm_omnivore_set_error(self,"raptor warning: %s", message);
}
#endif /* HAVE_LIBRAPTOR */

/* -------------------------------------------------------------------------
 *
 *                              expat handlers
 *
 * ------------------------------------------------------------------------- */

#ifdef HAVE_LIBEXPAT
/* expat callback for start-elemnt event.
 */
void expat_start(void *data, const char *el, const char **attr)
{
	element *ep;
	const char *local_name;
	char buf[MAXURI];
	char fragment[MAXURI];
	int parent_me_count = 0;
	int i;
	Omnivore ov = (Omnivore)data;

	if( OV_ERROR_OCCURRED(ov) )
		return;

	ov->depth++;

	TMTRACE(TM_OMNIVORE_TRACE,"############> expat start element\n");

	/*
	 * Code for handling ignored subtrees.
	 */

	if(ov->ignore_depth)
	{
		TMTRACE(TM_OMNIVORE_TRACE,"  -ignored-\n");
		return;
	}

	if( (local_name = strrchr(el,NS_DELIM)) != NULL)
		local_name++;
	else {
		local_name = el;
	}
	TMTRACE(TM_OMNIVORE_TRACE,"start element:\n");
	TMTRACE(TM_OMNIVORE_TRACE,"  fullname: %s\n" _ el);
	TMTRACE(TM_OMNIVORE_TRACE,"local name: %s\n" _ local_name);

	assert(ov->model);

	/* Here we check if we are processing markup that can be
	 * ignored. If so, Omnivore will ignore all child elements
	 * of this element.
	 * FIXME: is use of local_name ok?
	 */
	if(tm_procmodel_ignore_element(ov->model,local_name))
	{
		assert(ov->ignore_depth == 0);
		ov->ignore_depth = ov->depth;	
		TMTRACE(TM_OMNIVORE_TRACE,"  -starting to ignore-\n");
		return;
	}


	/* FIXME: init struct */
	ep = &(ov->element_stack[ov->elem_index]);
	bzero(ep,sizeof(*ep));

	if(ov->elem_index > 0)
	{
		struct element *parent;
		parent = &(ov->element_stack[ov->elem_index-1]);

		parent_me_count = _get_childcount(parent,local_name,1);

	}
	else
	{
		parent_me_count = 1;
	}


	/*
	 * construct element URI
	 */

	*(ep->id) = '\0';
	
	for (i = 0; attr[i]; i += 2)
	{
		TMTRACE(TM_OMNIVORE_TRACE,"-Attr: %s = %s\n" _
						attr[i] _ attr[i + 1]);
		
		if( tm_procmodel_is_id_attribute(ov->model,attr[i]) )
		{
			/* construct element Uri form doc and ID-what did this comment mean?*/
			strcpy(ep->id,attr[i + 1]);
			break;
		}
	}

	bzero(buf,sizeof(buf));
	bzero(fragment,sizeof(fragment));


	if( *(ep->id) ) {

		strcpy(fragment,ep->id);
	} else {
		char b[MAXURI];
		int i;
		bzero(b,sizeof(b));

		for(i = 0; i < ov->elem_index; i++)
		{
			char c[MAXURI];
			/*
			char f[MAXURI];
			*/
			if(*ov->element_stack[i].id) {
				snprintf(c,sizeof(c),"/%s[id()='%s']",
					ov->element_stack[i].name,
					ov->element_stack[i].id);
			} else {
				if(i == 0)
				{
					snprintf(c,sizeof(c),"/%s[1]",
						ov->element_stack[i].name);
				}
				else
				{
					int n;
					element *p = &(ov->element_stack[i-1]);
					n = _get_childcount(p,ov->element_stack[i].name,0);
					if(n == 0)
						n=1;	
					snprintf(c,sizeof(c),"/%s[%d]",
						ov->element_stack[i].name, n);

				}
			}
			/*
			snprintf(f,sizeof(f),"/%s",
				ov->element_stack[i].name);
			*/
			
			strcat(b,c);
		}
		snprintf(fragment,sizeof(fragment),
			"xpointer(\"%s/%s[%d]\")", b, local_name, parent_me_count );
	
	}
	TMTRACE(TM_OMNIVORE_TRACE,"fragment is %s\n" _ fragment);


	/* now construct full path */
	/* FIXME: keep current path up to date and just store a pointer to
	 * it in element struct or do..
	 */
	{
		char d[MAXURI];
		int i;
		bzero(d,sizeof(d));

		for(i = 0; i < ov->elem_index; i++)
		{
			char f[MAXURI];
			snprintf(f,sizeof(f),"/%s",
				ov->element_stack[i].name);
			
			strcat(d,f);
		}
		snprintf(ep->path, sizeof(ep->path), "%s/%s",d,local_name);
	
	}
	TMTRACE(TM_OMNIVORE_TRACE,"path is %s\n" _ ep->path);

	/* FIXME: current base URI if we find xml:base ?*/
	tm_uri_from_id(ov->effective_doc_uri,fragment,ep->uri,sizeof(ep->uri));
	TMTRACE(TM_OMNIVORE_TRACE,"element URI is %s\n" _ ep->uri );


	/*
	 * Now the same for the ref URI
	 */

	for (i = 0; attr[i]; i += 2)
	{
		TMTRACE(TM_OMNIVORE_TRACE,"-Attr: %s = %s\n" _
					attr[i] _ attr[i + 1]);
		
		if( tm_procmodel_is_ref_attribute(ov->model,attr[i]) )
		{
			/* construct element Uri form doc and ID */
			/*
			fprintf(stderr,"found ref attribute %s\n",attr[i]);
			*/
			tm_uri_from_ref(ov->effective_doc_uri,attr[ i+1 ],
					ep->ref_uri,sizeof(ep->ref_uri) );
			TMTRACE(TM_OMNIVORE_TRACE,"ref uri=%s\n" _ ep->ref_uri);
			break;
		}
	}
	

	assert(ov->elem_index < MAXITEMS);

	/*XXXXXX
	bzero(ep->text,sizeof(ep->text));
	*/

	strcpy(ep->name,local_name);

	snprintf(ep->text_uri,MAXURI,"%s/text()",ep->uri );
	/*
	LOG(ov,TM_LOG_PARSE, ep->uri );
	*/
	if(ov->model->start_element)
		ov->model->start_element(ov->proc_model_data,ov,ep,attr);
	ov->elem_index++;
	
	TMTRACE(TM_OMNIVORE_TRACE,"exit\n");
}

/* Callback for end-elemnt event.
 */
void expat_end(void *data, const char *el)
{
	const char *local_name;
	element *ep;
	Omnivore ov = (Omnivore)data;

	if( OV_ERROR_OCCURRED(ov) )
		return;

	/* are we in an ignored subtree ?*/
	if(ov->ignore_depth && ov->ignore_depth < ov->depth)
	{
		ov->depth--;
		TMTRACE(TM_OMNIVORE_TRACE,"  -end tag ignored-\n");
		return;
	}

	/* in ignored subtree, but leaving it now ? */
	if(ov->ignore_depth && ov->ignore_depth == ov->depth)
	{
		ov->ignore_depth = 0;
		TMTRACE(TM_OMNIVORE_TRACE,"  -stopping to ignore-\n");
		ov->depth--;
		return;	/* just leave ignored subtree, no action */
	}

	if( (local_name = strrchr(el,NS_DELIM)) != NULL)
		local_name++;
	else
		local_name = el;

	TMTRACE(TM_OMNIVORE_TRACE,"end element:\n");
	TMTRACE(TM_OMNIVORE_TRACE,"  fullname: %s\n" _ el);
	TMTRACE(TM_OMNIVORE_TRACE,"local name: %s\n" _ local_name);


	ov->elem_index--;
	ep = &(ov->element_stack[ov->elem_index]);


	ov->depth--;
	TMTRACE(TM_OMNIVORE_TRACE,"popped element: _%s_\n" _
			ov->element_stack[ov->elem_index-1].name);


	/* call processing model's end callback */
	if(ov->model->end_element)
		ov->model->end_element(ov->proc_model_data,ov,ep);

}
void expat_char_data(void *user_data, const XML_Char *data, int len)
{
	element *ep;
	int space_available;
	int is_text_elem = 0;
	TMList lp;
	Omnivore ov = (Omnivore)user_data;
	if( OV_ERROR_OCCURRED(ov) )
		return;
	/* are we in an ignored subtree? */
	if(ov->ignore_depth && ov->ignore_depth < ov->depth)
	{
		TMTRACE(TM_OMNIVORE_TRACE,"  -char data ignored-\n");
		return;
	}
	ep = &(ov->element_stack[ov->elem_index-1]);
	/* ignore non-text defined elems or similar */
	for(lp=ov->model->text_elements;lp;lp=lp->next)
	{
		if(strcmp( (char*)lp->content, ep->name ) ==0)
		{
			is_text_elem = 1;
			break;
		}
	}
	if(!is_text_elem)
		return;
	


	/* FIXME: dynamic growth if needed + flag if dealloc on end event */
	space_available = (sizeof(ep->text)-strlen(ep->text) );
	assert(space_available > len);

	/* text buffer was bzero-ed, so no \0 needed here */
	strncat(ep->text,data,len);
	TMTRACE(TM_OMNIVORE_TRACE,"element text now: [%s]\n" _ ep->text);
}

void expat_ns_start(void *data, const char *prefix, const char *uri)
{
	Omnivore ov = (Omnivore)data;
	assert(uri); /* FIXME: is that assumption correct? */
	TMTRACE(TM_OMNIVORE_TRACE,"namespace start: %s -> %s\n" _
		 prefix ? prefix : "(null)" _ uri);
	if(prefix == NULL)
	{
		TMTRACE(TM_OMNIVORE_TRACE,"default namespace is %s\n" _ uri);
		strncpy(ov->default_namespace_uri,uri,
			sizeof(ov->default_namespace_uri));
	}
	/* FIXME: store in table for lookup, normalize URI and use
	 * standardized prefix? */
} 

void expat_ns_end(void *data, const char *prefix)
{
	Omnivore ov = (Omnivore)data;
	assert(ov);
	TMTRACE(TM_OMNIVORE_TRACE,"namespace end: %s\n" _
		 prefix ? prefix : "(null)");
}

#endif



/* local line parser stuff */

#define INITIAL_BUF_SIZE 2000
struct TMLineParser {
	int buf_size;
	char *buf;
	int buf_len;
	char errstr[1024];
	int line;
	int pos;
	Omnivore omnivore;
};



TMLineParser tm_line_parser_new(Omnivore om)
{
	TMLineParser self;
	TM_NEW(self);
	self->buf = TM_ALLOC( INITIAL_BUF_SIZE );
	self->buf_size = INITIAL_BUF_SIZE;
	bzero(self->buf,INITIAL_BUF_SIZE);
	self->buf_len = 0;
	bzero(self->errstr,sizeof(self->errstr) );
	self->line = 1; /* wenn something happens, we are already in line 1 */
	self->pos = 0;

	self->omnivore = om;
	return (self);
}
void tm_line_parser_delete(TMLineParser *pself)
{
	assert(pself);
	assert(*pself);
	TM_FREE( (*pself)->buf );
	TM_FREE(*pself);

}
const char *tm_line_parser_get_error(TMLineParser self)
{
	assert(self);
	return (self->errstr);

}
int tm_line_parser_get_current_line_number(TMLineParser self)
{
	assert(self);
	return (self->line);
}
int tm_line_parser_get_current_char_number(TMLineParser self)
{
	assert(self);
	return (self->pos);
}
int tm_line_parser_handle_chunk(TMLineParser self,const char* s, int len, int isFinal)
{
	int i = 0;
	assert(self);
	while(i < len)
	{
		assert( self->buf_len < self->buf_size);
		assert( s[i] != '\0'); /* this would be quite strange... */
		self->pos++;
		self->buf[ self->buf_len ] = s[i];
		if(s[i] == '\n')
		{
			self->line++;
			self->buf[ self->buf_len ] = '\0';	/* terminate line */
			/* here ! */
			assert(self->omnivore->model->handle_line); /* FIXME: report error */
			if( ! self->omnivore->model->handle_line(self->omnivore->proc_model_data,self->buf,9999) )
			{
				/* FIXME: what to do else? */
				return (0);
			}

			/* Clear buffer. The bzero could be ommited (FIXME) */
			bzero(self->buf,self->buf_size);
			self->buf_len = 0;	
			self->pos = 0;
		}
		else
		{
			self->buf_len++;
			if( self->buf_len >= self->buf_size)
			{
			tm_omnivore_set_error(self->omnivore,"line too long");
			return 0;
			}
			assert( self->buf_len < self->buf_size);
			/* FIXME: expand buffer */
		}

		i++;

	}

	return (1);
}

const char *tm_omnivore_set_error(Omnivore self, const char *fmt,...)
{
	char posbuf[1024]; /* FIXME */
	va_list args;
	assert(self);
	assert(fmt);
	va_start(args,fmt);
	vsnprintf(self->errstr,sizeof(self->errstr),fmt,args);
	va_end(args);

	sprintf(posbuf,"[POSITION UNKNOWN]");

	tm_log(self->tm,TM_LOG_DEBUG,"omnivore_set_error _%s_",self->errstr);
	if(strcmp(self->mode,"lines") == 0)
	{
		if(self->parser)
		{
		snprintf(posbuf,sizeof(posbuf),"%s::%d column %d",
			self->doc_uri,
			tm_line_parser_get_current_line_number((TMLineParser)self->parser),
			tm_line_parser_get_current_char_number((TMLineParser)self->parser)
			);
		}
	}

#ifdef HAVE_LIBEXPAT
	else if(strcmp(self->mode,"xml") == 0)
	{
		snprintf(posbuf,sizeof(posbuf),"%s:%d column %d",
			self->doc_uri,
			XML_GetCurrentLineNumber((XML_Parser)self->parser),
			XML_GetCurrentColumnNumber((XML_Parser)self->parser)
			);
	}
#endif

#ifdef HAVE_LIBRAPTOR
	else if( (strcmp(self->mode,"rdfxml") == 0) || (strcmp(self->mode,"rdfntriples")))
	{
		raptor_locator *loc;
		loc = raptor_get_locator((raptor_parser*)self->parser);
		raptor_format_locator(posbuf,sizeof(posbuf),loc);
	}
#endif

	else
	{
		assert(0);
	}

	strcat(self->errstr," ");
	strcat(self->errstr,posbuf);
	tm_log(self->tm,TM_LOG_DEBUG,"omnivore_set_error _%s_",self->errstr);
	tm_set_error(self->tm,"%s",self->errstr);

	return (self->errstr);
}


int _get_childcount(struct element *ep, const char *name,int do_increment)
{
	int i=0;
	/*
	struct child_count *cp;
	*/
	assert(strlen(name) < MAX_NAME_SIZE);
	while(*(ep->per_child_counts[i].name))
	{
		if( strcmp(ep->per_child_counts[i].name,name) == 0)
		{
			if(do_increment)
				ep->per_child_counts[i].count++;
			return (ep->per_child_counts[i].count);

		}
		i++;
		assert(i < MAX_CHILDS);
	}
	if(!do_increment)
		return 0;
	strcpy(ep->per_child_counts[i].name,name);
	ep->per_child_counts[i].count = 1;
	return 1;
}
TMTopicMap tm_omnivore_get_topicmap(Omnivore self)
{
	assert(self);
	return self->topicmap;
}
TM tm_omnivore_get_tm(Omnivore self)
{
	assert(self);
	return self->tm;
}


