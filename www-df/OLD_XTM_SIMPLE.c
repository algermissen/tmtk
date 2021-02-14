/*
 * $Id$
 *
 * Copyright (c) 2002 Jan Algermissen
 * See the file "COPYING" for copying permission.
 *
 */

#include <tm.h>
#include <tmassertion.h>
#include <tmmodel.h>
#include <tmlist.h>

#include <omnivore.h>		/* for struct Omnivore */
#include <tmmem.h> 		/* for TM_FREE macro */
#include <tmutil.h> 
#include <tmassert.h>
#include <tmtrace.h>
#include <tmtable.h>
#include <tmvaluetype.h>
#include <tmvaluetypesystem.h>




/* The name of the model as a macro, in case we need to change it. */

#define OURNAME "sam"
#define SAM_NAME "SAM"

#if 0
struct association
{
	char *p_loc;
	char *a_loc;
	struct {
		char *r_loc;
		char *c_loc;
		char *x_loc;
		char *x_data;
		int x_is_scr;
	} members[TM_MAXMEMBERS];
	int nmember;
	/* FIXME: propably include scope set too */
};


static void gs_init_association(struct association *ap);
static int _assoc_add_member(struct association *ap,
						const char *r_loc,
						const char *c_loc,
						const char *x_loc,
						const char *x_data,
						int x_is_scr);

static void _handle_topic_demander(Omnivore ov, const char *uri, int flag);
static void handle_association(Omnivore ov, struct association *ap);

static void _handle_themes(Omnivore ov, TMList list,TMSubject s);

#define BASE   "http://www.topicmaps.org/xtm/1.0/core.xtm"

#define PSI_AT_INDICATEDSUBJECT_SUBJECTINDICATOR \
				BASE"#at-indicatedSubject-subjectIndicator"

#define PSI_ROLE_INDICATEDSUBJECT	BASE"#role-indicatedSubject"
#define PSI_ROLE_SUBJECTINDICATOR	BASE"#role-subjectIndicator"

#define PSI_DEFAULT_AP			BASE"#default-pattern"
#define PSI_DEFAULT_ROLE		BASE"#default-role"
#define PSI_DEFAULT_PLAYER		BASE"#default-player"

#define PSI_ROLE_TOPIC			BASE"#role-topic"

#define PSI_AT_TOPIC_BASENAME		BASE"#at-topic-basename"
#define PSI_ROLE_BASENAME		BASE"#role-basename"

#define PSI_AT_TOPIC_OCCURRENCE		BASE"#at-topic-occurrence"
#define PSI_ROLE_OCCURRENCE		BASE"#role-occurrence"

#define PSI_AT_CLASS_INSTANCE		BASE"#class-instance"
#define PSI_ROLE_CLASS			BASE"#class"
#define PSI_ROLE_INSTANCE		BASE"#instance"

#define PSI_AT_SUPERCLASS_SUBCLASS	BASE"#superclass-subclass"
#define PSI_ROLE_SUPERCLASS		BASE"#superclass"
#define PSI_ROLE_SUBCLASS		BASE"#subclass"

/*
#define PSI_AP_BASENAME_VARIANTNAME	BASE"#ap-basename-variantname"
#define PSI_ROLE_VARIANTNAME		BASE"#role-variantname"
#define PSI_AP_PATTERN_ROLE_RPC		BASE"#ap-pattern-role-rp"
#define PSI_ROLE_PATTERN		BASE"#role-pattern"
#define PSI_ROLE_RPC			BASE"#role-rpc"
#define PSI_ROLE_ROLE			BASE"#role-role"
*/


TM_STATIC_SUBJECT_N1(sbj_at_indicatedSubject_subjectIndicator,OURNAME,"SubjectAddress",PSI_AT_INDICATEDSUBJECT_SUBJECTINDICATOR);
TM_STATIC_SUBJECT_N1(sbj_role_indicatedSubject,	OURNAME,"SubjectAddress",PSI_ROLE_INDICATEDSUBJECT);
TM_STATIC_SUBJECT_N1(sbj_role_subjectIndicator,	OURNAME,"SubjectAddress",PSI_ROLE_SUBJECTINDICATOR);

TM_STATIC_SUBJECT_N1(sbj_role_topic,			OURNAME,"SubjectAddress",PSI_ROLE_TOPIC);
TM_STATIC_SUBJECT_N1(sbj_at_topic_basename,		OURNAME,"SubjectAddress",PSI_AT_TOPIC_BASENAME);
TM_STATIC_SUBJECT_N1(sbj_role_basename,		OURNAME,"SubjectAddress",PSI_ROLE_BASENAME);
TM_STATIC_SUBJECT_N1(sbj_at_topic_occurrence,		OURNAME,"SubjectAddress",PSI_AT_TOPIC_OCCURRENCE);
TM_STATIC_SUBJECT_N1(sbj_role_occurrence,		OURNAME,"SubjectAddress",PSI_ROLE_OCCURRENCE);
TM_STATIC_SUBJECT_N1(sbj_at_class_instance,		OURNAME,"SubjectAddress",PSI_AT_CLASS_INSTANCE);
TM_STATIC_SUBJECT_N1(sbj_role_class,			OURNAME,"SubjectAddress",PSI_ROLE_CLASS);
TM_STATIC_SUBJECT_N1(sbj_role_instance,		OURNAME,"SubjectAddress",PSI_ROLE_INSTANCE);
TM_STATIC_SUBJECT_N1(sbj_at_superclass_subclass,	OURNAME,"SubjectAddress",PSI_AT_SUPERCLASS_SUBCLASS);
TM_STATIC_SUBJECT_N1(sbj_role_superclass,		OURNAME,"SubjectAddress",PSI_ROLE_SUPERCLASS);
TM_STATIC_SUBJECT_N1(sbj_role_subclass,		OURNAME,"SubjectAddress",PSI_ROLE_SUBCLASS);

static struct TMList sirs1 = { NULL, PSI_AT_INDICATEDSUBJECT_SUBJECTINDICATOR };
static struct TMList sirs1a = { NULL, PSI_ROLE_INDICATEDSUBJECT };
static struct TMList sirs3 = { NULL, PSI_ROLE_SUBJECTINDICATOR };

static struct TMList sirs2 = { NULL, PSI_ROLE_TOPIC };
static struct TMList sirs4 = { NULL, PSI_AT_TOPIC_BASENAME };
static struct TMList sirs5 = { NULL, PSI_ROLE_BASENAME };
static struct TMList sirs6 = { NULL, PSI_AT_TOPIC_OCCURRENCE };
static struct TMList sirs7 = { NULL, PSI_ROLE_OCCURRENCE };
static struct TMList sirs8 = { NULL, PSI_AT_CLASS_INSTANCE };
static struct TMList sirs9 = { NULL, PSI_ROLE_CLASS };
static struct TMList sirs10 = { NULL, PSI_ROLE_INSTANCE };
static struct TMList sirs11 = { NULL, PSI_AT_SUPERCLASS_SUBCLASS };
static struct TMList sirs12 = { NULL, PSI_ROLE_SUPERCLASS };
static struct TMList sirs13 = { NULL, PSI_ROLE_SUBCLASS };


TM_STATIC_SUBJECT_N1(indicated_sbj_at_indicatedSubject_subjectIndicator,OURNAME,"SubjectIndicators",&sirs1);
TM_STATIC_SUBJECT_N1(indicated_sbj_role_indicatedSubject,OURNAME,"SubjectIndicators",&sirs1a);
TM_STATIC_SUBJECT_N1(indicated_sbj_role_subjectIndicator,OURNAME,"SubjectIndicators",&sirs3);

TM_STATIC_SUBJECT_N1(indicated_sbj_role_topic,OURNAME,"SubjectIndicators",&sirs2);
TM_STATIC_SUBJECT_N1(indicated_sbj_at_topic_basename,OURNAME,"SubjectIndicators",&sirs4);
TM_STATIC_SUBJECT_N1(indicated_sbj_role_basename,OURNAME,"SubjectIndicators",&sirs5);
TM_STATIC_SUBJECT_N1(indicated_sbj_at_topic_occurrence,OURNAME,"SubjectIndicators",&sirs6);
TM_STATIC_SUBJECT_N1(indicated_sbj_role_occurrence,OURNAME,"SubjectIndicators",&sirs7);
TM_STATIC_SUBJECT_N1(indicated_sbj_at_class_instance,OURNAME,"SubjectIndicators",&sirs8);
TM_STATIC_SUBJECT_N1(indicated_sbj_role_class,OURNAME,"SubjectIndicators",&sirs9);
TM_STATIC_SUBJECT_N1(indicated_sbj_role_instance,OURNAME,"SubjectIndicators",&sirs10);
TM_STATIC_SUBJECT_N1(indicated_sbj_at_superclass_subclass,OURNAME,"SubjectIndicators",&sirs11);
TM_STATIC_SUBJECT_N1(indicated_sbj_role_superclass,OURNAME,"SubjectIndicators",&sirs12);
TM_STATIC_SUBJECT_N1(indicated_sbj_role_subclass,OURNAME,"SubjectIndicators",&sirs13);
#endif


/* =========================================================================
   ========================================================================= */
/*
static struct TMList l1 = { NULL, "baseName" };
static struct TMList l2 = { NULL, "instanceOf" };
*/

static struct TMList l2 = { NULL, "occurrence" };
static struct TMList l3 = { &l2, "scope" };
static struct TMList l4 = { &l3, "variant" };

static int xtm_start_element(void *data, Omnivore ov, element *elem);
static int xtm_end_element(void *data, Omnivore ov, element *elem);

struct data {
	TMTopicMap tm;
	TMTopic last_topic_topic;
	struct TMTopicAssertion topic_assertion;

	TMTable at_table;
	TMTable role_table;

	TMTopic at_basenamed_basename_topic;
	TMTopic role_basenamed_topic;
	TMTopic role_basename_topic;
};

static void* _init(TMTopicMap tm)
{
	TMModel model;
	TMError e;
	struct data *dp;
	TM_NEW(dp);
	dp->tm = tm;
	dp->last_topic_topic = 0;
	if( ! dp->tm )
		return (dp);
	/* FIXME: the require stuff could be part of proc model structure! */
	if( (e = tmtk_default_model_lookup_function("SAM",&model)) != TM_OK)
	{
		assert(0);
	}
	if(tm_topicmap_require_model(dp->tm,model) != TM_OK)
	{
		assert(0);
	}
	tm_topicmap_fully_merge(dp->tm);

	dp->at_table = tm_table_new(100,tm_strcmp_v,tm_strhash_v);
	dp->role_table = tm_table_new(100,tm_strcmp_v,tm_strhash_v);

        if( tm_topicmap_get_topic(dp->tm,"IS13250","UniqueName","AtBaseNamedBaseName",
                                &(dp->at_basenamed_basename_topic)) != TM_OK)
        {
                assert(0);
        }
        if( tm_topicmap_get_topic(dp->tm,"IS13250","UniqueName","RoleBaseNamed",
                                &(dp->role_basenamed_topic)) != TM_OK )
        {
                assert(0);
        }
        if( tm_topicmap_get_topic(dp->tm,"IS13250","UniqueName","RoleBaseName",
                                &(dp->role_basename_topic)) != TM_OK )
        {
                assert(0);
        }
	assert(dp->at_basenamed_basename_topic);
	assert(dp->role_basenamed_topic);
	assert(dp->role_basename_topic);

	return dp;
}

static void _delete(void **pself)
{
	/* FIXME */
	/*
	tm_table_delete(&( (*pself)->role_table));
	*/
}



/* A processing model for a subset of XTM
 */
struct TMProcModel xtm_simple = 
{
	"xtm_simple",		/* name */
	_init,
	_delete,
	"topicMap",	/* root element name (for lookup) */
	"http://www.topicmaps.org/xtm/1.0/",
	"-//TopicMaps.Org//DTD XML Topic Map (XTM) 1.0//EN",
	"http://www.topicmaps.org/xtm/1.0/xtm1.dtd",
	NULL,

	"id",	/* id_attribute_name */
	"href", /* ref_attribute_name */

	&l4,

	&xtm_start_element,
	&xtm_end_element
};


/* =========================================================================

                           XTM PROCESSING MODEL

   ========================================================================= */

/* FIXME, should be in userData or so... */
#if 0
static TMList tpool = NULL;
static TMList apool = NULL;
static TMList spool = NULL;

static struct association association;
static struct association occ_type_assoc;
static char *last_topic_element_uri;
static char *last_occ_element_uri;
static char *last_role_loc = NULL;
static TMList themes = NULL;
static struct TMSubject scope_sbj;

static struct TMList __list = { NULL,NULL };
TM_STATIC_SUBJECT_N1(last_topic_demanded_sbj,OURNAME,"SubjectIndicators",&__list);

static char *_pstrdup(TMList *list, const char*s)
{
	char *t;
	assert(s);
	t = tm_strdup(s);
	*list = tm_list_push(*list,t);
	return t;
}
#endif

int xtm_start_element(void *s,Omnivore ov, element *elem)
{
	TMError e;
	struct data *self = (struct data *)s;

       if(strcmp(elem_path(elem),"/topicMap") == 0) {
		;
       } else if(strcmp(elem_path(elem),"/topicMap/topic") == 0) {
	       struct TMSubject sbj;
	       struct TMList sirs;
		sirs.next = NULL;
		sirs.content = (void*)elem_uri(elem);

		sbj.N              = 1;
		sbj.props[0].model = SAM_NAME;
		sbj.props[0].name  = "SubjectIndicators";
		sbj.props[0].value = &sirs;

		if( (e = tm_topicmap_add_subject(self->tm,&sbj,&(self->last_topic_topic) )) != TM_OK)
		{
			tm_omnivore_set_error(ov,"xtm_simple: proce error: %s" ,
					tm_topicmap_get_error(self->tm));
			return (0);
		}

	} else if(strcmp(elem_path(elem),"/topicMap/topic/subjectIdentity") == 0) {
	       ;
	} else if(strcmp(elem_path(elem),"/topicMap/topic/instanceOf") == 0) {
	       ;
	} else if(strcmp(elem_path(elem),"/topicMap/topic/instanceOf/topicRef") == 0) {
	       ;
	} else if(strcmp(elem_path(elem),"/topicMap/topic/instanceOf/subjectIndicatorRef") == 0) {
	       ;
        } else if(strcmp(elem_path(elem),"/topicMap/topic/subjectIdentity/resourceRef") == 0 ) {
		assert(elem_ref_uri(elem) && *elem_ref_uri(elem));

		if( tm_topicmap_add_property_to_topic(self->tm,self->last_topic_topic,
			SAM_NAME,"SubjectAddress",elem_ref_uri(elem)) != TM_OK)
		{
			tm_omnivore_set_error(ov,"xtm_simple: proce error: %s" ,
					tm_topicmap_get_error(self->tm));
			assert(0);
		}

        } else if(strcmp(elem_path(elem),"/topicMap/topic/subjectIdentity/topicRef") == 0
               || strcmp(elem_path(elem),"/topicMap/topic/subjectIdentity/subjectIndicatorRef")==0){
		struct TMList list;
		list.next = NULL;
		list.content = (char*)elem_ref_uri(elem);
		if( tm_topicmap_add_property_to_topic(self->tm,self->last_topic_topic,
			SAM_NAME,"SubjectIndicators",&list) != TM_OK)
		{
			tm_omnivore_set_error(ov,"xtm_simple: proce error: %s" ,
					tm_topicmap_get_error(self->tm));
			assert(0);
		}

	/*
	 * ----------------------------------------------------------------
	 *
	 *                   <baseName> and subelements
	 *
	 * ----------------------------------------------------------------
	 */

 
        } else if(strcmp(elem_path(elem),"/topicMap/topic/baseName") == 0) {
		TMTopic a_topic;
		struct TMSubject sbj;
		struct TMList sirs;
		sirs.next = NULL;
		sirs.content = (void*)elem_uri(elem);

		sbj.N              = 1;
		sbj.props[0].model = SAM_NAME;
		sbj.props[0].name  = "SubjectIndicators";
		sbj.props[0].value = &sirs;

		if( (e = tm_topicmap_add_subject(self->tm,&sbj,&a_topic )) != TM_OK)
		{
			tm_omnivore_set_error(ov,"xtm_simple: proce error: %s" ,
					tm_topicmap_get_error(self->tm));
			return (0);
		}
		tm_topic_assertion_init(&(self->topic_assertion));
		self->topic_assertion.A = a_topic;
		self->topic_assertion.T = self->at_basenamed_basename_topic;
		self->topic_assertion.N = 1;
		self->topic_assertion.memberships[0].R = self->role_basenamed_topic;
		self->topic_assertion.memberships[0].x = self->last_topic_topic;

        } else if(strcmp(elem_path(elem),"/topicMap/topic/baseName/baseNameString") == 0) {
		;

	/*
	 * ----------------------------------------------------------------
	 *
	 *                   <association> and subelements
	 *
	 * ----------------------------------------------------------------
	 */


        } else if(strcmp(elem_path(elem),"/topicMap/association") == 0) {
		TMTopic a_topic;
		struct TMSubject sbj;
		struct TMList sirs;
		sirs.next = NULL;
		sirs.content = (void*)elem_uri(elem);

		sbj.N              = 1;
		sbj.props[0].model = SAM_NAME;
		sbj.props[0].name  = "SubjectIndicators";
		sbj.props[0].value = &sirs;

		if( (e = tm_topicmap_add_subject(self->tm,&sbj,&a_topic )) != TM_OK)
		{
			tm_omnivore_set_error(ov,"xtm_simple: proce error: %s" ,
					tm_topicmap_get_error(self->tm));
			return (0);
		}
		tm_topic_assertion_init(&(self->topic_assertion));
		self->topic_assertion.A = a_topic;
        } else if(strcmp(elem_path(elem),"/topicMap/association/instanceOf") == 0) {
		;

        } else if(strcmp(elem_path(elem),"/topicMap/association/instanceOf/topicRef") == 0
	       || strcmp(elem_path(elem),"/topicMap/association/instanceOf/subjectIndicatorRef")==0){

		TMTopic t_topic;
		struct TMSubject sbj;
		struct TMList sirs;
		assert(elem_ref_uri(elem) && *elem_ref_uri(elem));

		t_topic = (TMTopic)tm_table_get(self->at_table,elem_ref_uri(elem));

		if( ! t_topic)
		{
		sirs.next = NULL;
		sirs.content = (void*)elem_ref_uri(elem);

		sbj.N              = 1;
		sbj.props[0].model = SAM_NAME;
		sbj.props[0].name  = "SubjectIndicators";
		sbj.props[0].value = &sirs;

		if( (e = tm_topicmap_add_subject(self->tm,&sbj,&t_topic )) != TM_OK)
		{
			tm_omnivore_set_error(ov,"xtm_simple: proce error: %s" ,
					tm_topicmap_get_error(self->tm));
			return (0);
		}
		assert(t_topic);


		tm_table_put(self->at_table,tm_strdup(elem_ref_uri(elem)),(void*)t_topic);
		}
		self->topic_assertion.T = t_topic;

        } else if(strcmp(elem_path(elem),"/topicMap/association/member") == 0) {
		int N = self->topic_assertion.N;
		self->topic_assertion.memberships[N].R = 0;
		self->topic_assertion.memberships[N].x = 0;

        } else if(strcmp(elem_path(elem),"/topicMap/association/member/roleSpec") == 0) {
		;

        } else if(strcmp(elem_path(elem),"/topicMap/association/member/roleSpec/topicRef") == 0
	       || strcmp(elem_path(elem),"/topicMap/association/member/roleSpec/subjectIndicatorRef") == 0) {
		TMTopic r_topic;
		struct TMSubject sbj;
		struct TMList sirs;
		int N;
		assert(elem_ref_uri(elem) && *elem_ref_uri(elem));
		N = self->topic_assertion.N; /* for shorter code lines below */

		r_topic = (TMTopic)tm_table_get(self->role_table,elem_ref_uri(elem));

		if( ! r_topic)
		{



		sirs.next = NULL;
		sirs.content = (void*)elem_ref_uri(elem);

		sbj.N              = 1;
		sbj.props[0].model = SAM_NAME;
		sbj.props[0].name  = "SubjectIndicators";
		sbj.props[0].value = &sirs;

		if( (e = tm_topicmap_add_subject(self->tm,&sbj,&r_topic )) != TM_OK)
		{
			tm_omnivore_set_error(ov,"xtm_simple: proce error: %s" ,
					tm_topicmap_get_error(self->tm));
			return (0);
		}
		assert(r_topic);
		tm_table_put(self->role_table,tm_strdup(elem_ref_uri(elem)),(void*)r_topic);
		}

		assert(self->topic_assertion.memberships[N].R == 0);
		assert(self->topic_assertion.memberships[N].x == 0);
		self->topic_assertion.memberships[N].R = r_topic;

        } else if(strcmp(elem_path(elem),"/topicMap/association/member/topicRef") == 0 
	       || strcmp(elem_path(elem),"/topicMap/association/member/subjectIndicatorRef") == 0 ){

		TMTopic x_topic;
		struct TMSubject sbj;
		struct TMList sirs;
		int N;
		assert(elem_ref_uri(elem) && *elem_ref_uri(elem));
		N = self->topic_assertion.N; /* for shorter code lines below */
		sirs.next = NULL;
		sirs.content = (void*)elem_ref_uri(elem);

		sbj.N              = 1;
		sbj.props[0].model = SAM_NAME;
		sbj.props[0].name  = "SubjectIndicators";
		sbj.props[0].value = &sirs;

		if( (e = tm_topicmap_add_subject(self->tm,&sbj,&x_topic )) != TM_OK)
		{
			tm_omnivore_set_error(ov,"xtm_simple: proce error: %s" ,
					tm_topicmap_get_error(self->tm));
			return (0);
		}
		if(self->topic_assertion.memberships[N].R == 0)
		{
			tm_omnivore_set_error(ov,"players without a role not supported, element uri is '%s'", elem_uri(elem));
			return (0);
		}
		if(self->topic_assertion.memberships[N].x != 0)
		{
			tm_omnivore_set_error(ov,"multiple role players not supported, element uri is '%s'", elem_uri(elem));
			return (0);
		}
		self->topic_assertion.memberships[N].x = x_topic;


        } else if(strcmp(elem_path(elem),"/topicMap/association/member/resourceRef") == 0) {

		TMTopic x_topic;
		struct TMSubject sbj;
		int N;
		assert(elem_ref_uri(elem) && *elem_ref_uri(elem));
		N = self->topic_assertion.N; /* for shorter code lines below */

		sbj.N              = 1;
		sbj.props[0].model = SAM_NAME;
		sbj.props[0].name  = "SubjectAddress";
		sbj.props[0].value = elem_ref_uri(elem);

		if( (e = tm_topicmap_add_subject(self->tm,&sbj,&x_topic )) != TM_OK)
		{
			tm_omnivore_set_error(ov,"xtm_simple: proce error: %s" ,
					tm_topicmap_get_error(self->tm));
			return (0);
		}
		assert(self->topic_assertion.memberships[N].R != 0);
		assert(self->topic_assertion.memberships[N].x == 0);
		self->topic_assertion.memberships[N].x = x_topic;



#if 0
	/*
	 * <topic> <instanceOf>
	 */
        } else if(strcmp(elem_path(elem),"/topicMap/topic/instanceOf") == 0) {
		_handle_topic_demander(ov,elem_uri(elem),TM_DEMAND_EXPLICIT);
		gs_init_association(&association);
		association.a_loc = _pstrdup(&tpool,elem_uri(elem));
		association.p_loc = PSI_AT_CLASS_INSTANCE;
		_assoc_add_member(&association,PSI_ROLE_INSTANCE,NULL,last_topic_element_uri,NULL,0);

        } else if(strcmp(elem_path(elem),"/topicMap/topic/instanceOf/topicRef") == 0) {
		assert(elem_ref_uri(elem) && *elem_ref_uri(elem));
		/* The topicRef demands a topic */
		_handle_topic_demander(ov,elem_ref_uri(elem),TM_DEMAND_IMPLICIT);
		_assoc_add_member(&association,PSI_ROLE_CLASS,NULL,elem_ref_uri(elem),NULL,0);
		handle_association(ov,&association);

        } else if(strcmp(elem_path(elem),"/topicMap/topic/instanceOf/subjectIndicatorRef") == 0) {
		assert(elem_ref_uri(elem) && *elem_ref_uri(elem));
		/* The topicRef demands a topic */
		_handle_topic_demander(ov,elem_ref_uri(elem),TM_DEMAND_EXPLICIT);
		_assoc_add_member(&association,PSI_ROLE_CLASS,NULL,elem_ref_uri(elem),NULL,0);
		handle_association(ov,&association);



        } else if(strcmp(elem_path(elem),"/topicMap/topic/occurrence/instanceOf") == 0) {
		_handle_topic_demander(ov,elem_uri(elem),TM_DEMAND_EXPLICIT);
		gs_init_association(&occ_type_assoc);
		occ_type_assoc.a_loc = _pstrdup(&tpool,elem_uri(elem));
		occ_type_assoc.p_loc = PSI_AT_CLASS_INSTANCE;
		_assoc_add_member(&occ_type_assoc,PSI_ROLE_INSTANCE,NULL,last_occ_element_uri,NULL,0);

        } else if(strcmp(elem_path(elem),"/topicMap/topic/occurrence/instanceOf/topicRef") == 0) {
		assert(elem_ref_uri(elem) && *elem_ref_uri(elem));
		/* The topicRef demands a topic */
		_handle_topic_demander(ov,elem_ref_uri(elem),TM_DEMAND_IMPLICIT);
		_assoc_add_member(&occ_type_assoc,PSI_ROLE_CLASS,NULL,elem_ref_uri(elem),NULL,0);
		handle_association(ov,&occ_type_assoc);

        } else if(strcmp(elem_path(elem),"/topicMap/topic/occurrence/instanceOf/subjectIndicatorRef") == 0) {
		assert(elem_ref_uri(elem) && *elem_ref_uri(elem));
		/* The topicRef demands a topic */
		_handle_topic_demander(ov,elem_ref_uri(elem),TM_DEMAND_EXPLICIT);
		_assoc_add_member(&occ_type_assoc,PSI_ROLE_CLASS,NULL,elem_ref_uri(elem),NULL,0);
		handle_association(ov,&occ_type_assoc);




        } else if(strcmp(elem_path(elem),"/topicMap/topic/baseName") == 0) {
		_handle_topic_demander(ov,elem_uri(elem),TM_DEMAND_EXPLICIT);
		gs_init_association(&association);
		association.p_loc = PSI_AT_TOPIC_BASENAME;
		association.a_loc = _pstrdup(&tpool,elem_uri(elem));
		_assoc_add_member(&association,PSI_ROLE_TOPIC,NULL,last_topic_element_uri,NULL,0);



        } else if(strcmp(elem_path(elem),"/topicMap/topic/occurrence") == 0) {
		last_occ_element_uri = _pstrdup(&tpool,elem_uri(elem));
		_handle_topic_demander(ov,elem_uri(elem),TM_DEMAND_EXPLICIT);
		gs_init_association(&association);
		association.p_loc = PSI_AT_TOPIC_OCCURRENCE;
		association.a_loc = _pstrdup(&tpool,elem_uri(elem));
		_assoc_add_member(&association,PSI_ROLE_TOPIC,NULL,last_topic_element_uri,NULL,0);


        } else if(strcmp(elem_path(elem),"/topicMap/topic/baseName/baseNameString") == 0) {
		assert(association.p_loc);

	/*
	 * ----------------------------------------------------------------
	 *
	 *                   <association> and subelements
	 *
	 * ----------------------------------------------------------------
	 */


        } else if(strcmp(elem_path(elem),"/topicMap/association") == 0) {
		gs_init_association(&association);
		association.a_loc = _pstrdup(&apool,elem_uri(elem));
		_handle_topic_demander(ov,elem_uri(elem),TM_DEMAND_EXPLICIT);

        } else if(strcmp(elem_path(elem),"/topicMap/association/instanceOf/topicRef") == 0) {

		assert(elem_ref_uri(elem) && *elem_ref_uri(elem));
		association.p_loc = _pstrdup(&apool,elem_ref_uri(elem));
		_handle_topic_demander(ov,elem_ref_uri(elem),TM_DEMAND_IMPLICIT);

	} else if(strcmp(elem_path(elem),"/topicMap/association/instanceOf/subjectIndicatorRef") == 0 ) {
		assert(elem_ref_uri(elem) && *elem_ref_uri(elem));
		association.p_loc = _pstrdup(&apool,elem_ref_uri(elem));
		_handle_topic_demander(ov,elem_ref_uri(elem),TM_DEMAND_EXPLICIT);

        } else if(strcmp(elem_path(elem),"/topicMap/association/member") == 0) {
		last_role_loc = NULL;

        } else if(strcmp(elem_path(elem),"/topicMap/association/member/roleSpec/topicRef") == 0) {
		assert(elem_ref_uri(elem) && *elem_ref_uri(elem));
		last_role_loc = _pstrdup(&apool,elem_ref_uri(elem));
		_handle_topic_demander(ov,elem_ref_uri(elem),TM_DEMAND_IMPLICIT);

	} else if(strcmp(elem_path(elem),"/topicMap/association/member/roleSpec/subjectIndicatorRef") == 0 ) {
		assert(elem_ref_uri(elem) && *elem_ref_uri(elem));
		last_role_loc = _pstrdup(&apool,elem_ref_uri(elem));
		_handle_topic_demander(ov,elem_ref_uri(elem),TM_DEMAND_EXPLICIT);

        } else if(strcmp(elem_path(elem),"/topicMap/association/member/topicRef") == 0) {
		assert(elem_ref_uri(elem) && *elem_ref_uri(elem));
		_assoc_add_member(&association,last_role_loc,NULL,_pstrdup(&apool,elem_ref_uri(elem)),NULL,0);
		_handle_topic_demander(ov,elem_ref_uri(elem),TM_DEMAND_IMPLICIT);

        } else if(strcmp(elem_path(elem),"/topicMap/association/member/subjectIndicatorRef") == 0 ) {
		assert(elem_ref_uri(elem) && *elem_ref_uri(elem));
		_assoc_add_member(&association,last_role_loc,NULL,_pstrdup(&apool,elem_ref_uri(elem)),NULL,0);
		_handle_topic_demander(ov,elem_ref_uri(elem),TM_DEMAND_EXPLICIT);

        } else if(strcmp(elem_path(elem),"/topicMap/association/member/resourceRef") == 0) {
		struct TMSubject sbj;
		assert(elem_ref_uri(elem) && *elem_ref_uri(elem));
		sbj.N = 1;
		sbj.props[0].model = OURNAME;
		sbj.props[0].name = "SubjectAddress";
		sbj.props[0].value = elem_ref_uri(elem);
		Omnivore_handleBuiltInNode(ov,&sbj);
		_assoc_add_member(&association,last_role_loc,NULL,_pstrdup(&apool,elem_ref_uri(elem)),NULL,1);


	/*
	 * Scope and children
	 */

        } else if(tm_strendswith(elem_path(elem),"scope")) {
		assert(themes == NULL);
		TMTRACE(TM_PROC_TRACE,"start scope\n");
		/* FIXME: start ransaction */

        } else if(tm_strendswith(elem_path(elem),"scope/topicRef") 
		|| tm_strendswith(elem_path(elem),"scope/subjectIndicatorRef")) {
		TMTRACE(TM_PROC_TRACE,"theme %s\n" _ elem_ref_uri(elem) );
		themes = tm_list_push(themes, _pstrdup(&spool,elem_ref_uri(elem)));

        } else if(strcmp(elem_path(elem),"/topicMap/mergeMap") == 0) {
		/* FIXME: handle themes set init */
                ;

#endif
        } else {
		tm_omnivore_set_error(ov,"syntax error: unsupported path %s", elem_uri(elem));
		return (0);
	}
 
        return 1;
}
int xtm_end_element(void *s,Omnivore ov, element *elem)
{
	TMError e;
	struct data *self = (struct data *)s;
	if(strcmp(elem_path(elem),"/topicMap") == 0) {
		;


        } else if(strcmp(elem_path(elem),"/topicMap/association/member") == 0) {
		int N = self->topic_assertion.N;
		if(self->topic_assertion.memberships[N].R == 0)
		{
			tm_omnivore_set_error(ov,"member without role not supported, element uri is '%s'", elem_uri(elem));
			return (0);
		}
		if(self->topic_assertion.memberships[N].x == 0)
		{
			tm_omnivore_set_error(ov,"member without role-player not supported, element uri is '%s'", elem_uri(elem));
			return (0);
		}
		self->topic_assertion.N++;

        } else if(strcmp(elem_path(elem),"/topicMap/association") == 0) {

		if(self->topic_assertion.T == 0)
		{
			tm_omnivore_set_error(ov,"untyped association not supported, element uri is '%s'", elem_uri(elem));
			return (0);
		}
		/* FIXME error here or wait for topic map ?? 
		assert(self->topic_assertion.N >= 2);
		*/
		if( tm_topicmap_add_topic_assertion(self->tm,&(self->topic_assertion)) != TM_OK)
		{
			tm_omnivore_set_error(ov,"unable to add assertion to topic map, %s, element URI is %s", tm_topicmap_get_error(self->tm),elem_uri(elem));
			return (0);
		}

		/*
		int i;
		TMList lp;
		if(! association.p_loc) 
			association.p_loc = PSI_DEFAULT_AP;

		for(i=0;i < association.nmember;i++)
		{
			if(!association.members[i].r_loc)
				association.members[i].r_loc = PSI_DEFAULT_ROLE;

			if(!association.members[i].x_loc)
				association.members[i].x_loc = PSI_DEFAULT_PLAYER;
		}
		handle_association(ov,&association);

		for(lp = apool; lp; lp=lp->next)
			TM_FREE(lp->content);
		TMList_delete(&apool);
		assert(apool == NULL);
		*/

	/*
	 * ----------------------------------------------------------------
	 *
	 *                   <baseName> and subelements
	 *
	 * ----------------------------------------------------------------
	 */

        } else if(strcmp(elem_path(elem),"/topicMap/topic/baseName/baseNameString") == 0) {
		TMTopic b_topic;
		struct TMSubject sbj;
                struct TMList sirs;
                sirs.next = NULL;
                sirs.content = (void*)elem_uri(elem);

		sbj.N                       = 2;
		sbj.props[0].model          = SAM_NAME; 
		sbj.props[0].name           = "SubjectIndicators"; 
		sbj.props[0].value          = &sirs;
		sbj.props[1].model          = SAM_NAME; 
		sbj.props[1].name           = "IndicatorData"; 
		sbj.props[1].value          = elem_text(elem);

		if( (e = tm_topicmap_add_subject(self->tm,&sbj,&b_topic )) != TM_OK)
		{
			tm_omnivore_set_error(ov,"xtm_simple: proce error: %s" ,
					tm_topicmap_get_error(self->tm));
			return (0);
		}
		self->topic_assertion.N = 2;
		self->topic_assertion.memberships[1].R = self->role_basename_topic;
		self->topic_assertion.memberships[1].x = b_topic;

		if( tm_topicmap_add_topic_assertion(self->tm,&(self->topic_assertion)) != TM_OK)
		{
			tm_omnivore_set_error(ov,"unable to add assertion to topic map, %s, element URI is %s", tm_topicmap_get_error(self->tm),elem_uri(elem));
			return (0);
		}


#if 0
	} else if(strcmp(elem_path(elem),"/topicMap/topic") == 0) {
		TMList lp;
		for(lp = tpool; lp; lp=lp->next)
			TM_FREE(lp->content);
		TMList_delete(&tpool);
		assert(tpool == NULL);

        } else if(strcmp(elem_path(elem),"/topicMap/topic/baseName/baseNameString") == 0) {
		struct TMSubject sbj;

		sbj.N                       = 2;
		sbj.props[0].model          = OURNAME; 
		sbj.props[0].name           = "SubjectAddress"; 
		sbj.props[0].value          = elem_uri(elem);
		sbj.props[1].model          = OURNAME; 
		sbj.props[1].name           = "SubjectData"; 
		sbj.props[1].value          = elem_text(elem);

		/* FIXME: what did this mean ?? can't use static function because subject is copmplex.... */
		Omnivore_handleNodeDemander(ov,&sbj,TM_DEMAND_EXPLICIT);

		_assoc_add_member(&association,PSI_ROLE_BASENAME,NULL,elem_uri(elem),NULL,0);
		handle_association(ov,&association);

        } else if(strcmp(elem_path(elem),"/topicMap/topic/occurrence/resourceData") == 0) {
		struct TMSubject sbj;

		sbj.N                       = 2;
		sbj.props[0].model          = OURNAME; 
		sbj.props[0].name           = "SubjectAddress"; 
		sbj.props[0].value          = elem_uri(elem);
		sbj.props[1].model          = OURNAME; 
		sbj.props[1].name           = "SubjectData"; 
		sbj.props[1].value          = elem_text(elem);
		Omnivore_handleBuiltInNode(ov,&sbj);

		_assoc_add_member(&association,PSI_ROLE_OCCURRENCE,NULL,elem_uri(elem),NULL,1);
		handle_association(ov,&association);

        } else if(strcmp(elem_path(elem),"/topicMap/topic/occurrence/resourceRef") == 0) {
		struct TMSubject sbj;

		sbj.N                       = 1;
		sbj.props[0].model          = OURNAME; 
		sbj.props[0].name           = "SubjectAddress"; 
		sbj.props[0].value          = elem_ref_uri(elem);

		Omnivore_handleBuiltInNode(ov,&sbj);

		_assoc_add_member(&association,PSI_ROLE_OCCURRENCE,NULL,elem_ref_uri(elem),NULL,1);
		handle_association(ov,&association);

        } else if(strcmp(elem_path(elem),"/topicMap/association/member") == 0) {
		/*
		if(last_role_loc)
			TM_FREE(last_role_loc);
		last_role_loc = NULL;
		*/

        } else if(strcmp(elem_path(elem),"/topicMap/association") == 0) {
		int i;
		TMList lp;
		if(! association.p_loc) 
			association.p_loc = PSI_DEFAULT_AP;

		for(i=0;i < association.nmember;i++)
		{
			if(!association.members[i].r_loc)
				association.members[i].r_loc = PSI_DEFAULT_ROLE;

			if(!association.members[i].x_loc)
				association.members[i].x_loc = PSI_DEFAULT_PLAYER;
		}
		handle_association(ov,&association);

		for(lp = apool; lp; lp=lp->next)
			TM_FREE(lp->content);
		TMList_delete(&apool);
		assert(apool == NULL);

        } else if(tm_strendswith(elem_path(elem),"scope")) {
		TMList lp;
		if(themes != NULL)
		{
			TMTRACE(TM_PROC_TRACE,"end scope\n");
			_handle_themes(ov,themes,&scope_sbj);

			/* FIXME: end transaction => start conferring ?!?!? */

			/* delete themes pool */
			for(lp = spool; lp; lp=lp->next)
				TM_FREE(lp->content);
			TMList_delete(&spool);

			/* FIXME: cleanup themes */
			TMList_delete(&themes);
			themes = NULL;
		}
        } else if(strcmp(elem_path(elem),"/topicMap/mergeMap") == 0) {

		/* FIXME: added themes !*/
		/*
		if(ov->merge_map_handler)
			ov->merge_map_handler(ov->user_data, elem_ref_uri(elem));
		*/
#endif
        } else {
		/* FIXME: error at end element possible ? I guess not.
		tm_omnivore_set_error(ov,"syntax error: unsupported path %s", elem_uri(elem));
		return (0);
		*/
	}
 
        return 1;
}


/* =========================================================================

                               STATIC ROUTINES

   ========================================================================= */


#if 0
int _assoc_add_member(struct association *ap,
			const char *r_loc,
			const char *c_loc,
			const char *x_loc,
			const char *x_data,
			int x_is_scr)
{
	int i;
	int n = ap->nmember;
	assert(ap);
	/*
	assert(r_loc);
	assert(x_loc);
	*/

	for(i=0;i<n;i++)
	{
		if(r_loc == NULL)
		{
			if(ap->members[i].r_loc == NULL)
				return n;
			else
				continue;
		}
		if(strcmp(ap->members[i].r_loc,(char*)r_loc) == 0)
			return n;
	}
	
	ap->members[n].r_loc    = (char*)r_loc;
	ap->members[n].c_loc    = (char*)c_loc;
	ap->members[n].x_loc    = (char*)x_loc;
	ap->members[n].x_data   = (char*)x_data;

	ap->members[n].x_is_scr = x_is_scr;

	ap->nmember++;
	return ap->nmember;
}


void gs_init_association(struct association *ap)
{
	int i;
	assert(ap);
	ap->p_loc = NULL;
	ap->a_loc = NULL;
	ap->nmember=0;
	for(i = 0; i < TM_MAXMEMBERS; i++)
	{
		ap->members[i].r_loc = NULL;
		ap->members[i].c_loc = NULL;
		ap->members[i].x_loc = NULL;
		ap->members[i].x_data = NULL;
		ap->members[i].x_is_scr = 0;
	}
}


void _handle_topic_demander(Omnivore ov, const char *uri, int flag)
{
		struct TMSubject sbj;

		sbj.N = 1;
		sbj.props[0].model = OURNAME;
		sbj.props[0].name = "SubjectAddress";
		sbj.props[0].value = uri;

		Omnivore_handleNodeDemander(ov,&sbj,flag);
}
void _handle_themes(Omnivore ov, TMList themes, TMSubject s)
{
	TMList lp;
	
	TMList list = NULL;
	for(lp = themes; lp; lp=lp->next)
	{
		list = tm_list_push(list,tm_list_push(NULL,lp->content) );
	}
	s->N = 1;
	s->props[0].model = OURNAME;
	s->props[0].name = "SetMembers";
	s->props[0].value = list;
	/*FIXME: resource leak list !!!!!! */

	Omnivore_handleBuiltInNode(ov,s);
}

void handle_association(Omnivore ov, struct association *ap)
{
	struct TMAssertion assertion;
	struct TMSubject type_sbj;
	struct TMSubject self_sbj;

	struct TMSubject role_sbj[TM_MAXMEMBERS];
	struct TMSubject casting_sbj[TM_MAXMEMBERS];
	struct TMSubject player_sbj[TM_MAXMEMBERS];

	int i;

	TMList lp;
	TMList type_sirs = NULL;
	TMList assertion_sirs = NULL;
	TMList other_lists = NULL;

	assert(ap->p_loc);

	type_sirs = tm_list_push(type_sirs,ap->p_loc);	
	assert(type_sirs);
	type_sbj.N = 1;
	type_sbj.props[0].model = OURNAME;
	type_sbj.props[0].name = "SubjectIndicators";
	type_sbj.props[0].value = type_sirs;

	assertion.type = &type_sbj;

	if(ap->a_loc)
	{
		assertion_sirs = tm_list_push(assertion_sirs,ap->a_loc);	
		self_sbj.N = 1;
		self_sbj.props[0].model = OURNAME;
		self_sbj.props[0].name = "SubjectIndicators";
		self_sbj.props[0].value = assertion_sirs;

		assertion.self = &self_sbj;
	}
	else
	{
		assertion.self = NULL;
	}

	for(i=0; i<ap->nmember; i++)
	{
		TMList r_list = NULL, c_list = NULL, x_list = NULL;

		TMSubject rsp = &role_sbj[i];
		TMSubject csp = &casting_sbj[i];
		TMSubject psp = &player_sbj[i];

		assert(ap->members[i].r_loc);

		r_list = tm_list_push(r_list,ap->members[i].r_loc);
		other_lists = tm_list_push(other_lists,r_list);

		rsp->N = 1;
		rsp->props[0].model = OURNAME;
		rsp->props[0].name = "SubjectIndicators";
		rsp->props[0].value = r_list;

		assertion.memberships[i].role = rsp;


		if(ap->members[i].c_loc)
		{
			c_list = tm_list_push(c_list,ap->members[i].c_loc);
			other_lists = tm_list_push(other_lists,c_list);
			csp->N = 1;
			csp->props[0].model = OURNAME;
			csp->props[0].name = "SubjectIndicators";
			csp->props[0].value = c_list;
		}
		else
		{
			csp = NULL;
		}
		assertion.memberships[i].casting = csp;

	

		psp->N = 1;
		psp->props[0].model = OURNAME;



		if(ap->members[i].x_is_scr) {
			psp->props[0].name = "SubjectAddress";
			psp->props[0].value = ap->members[i].x_loc;
		} else {

			x_list = tm_list_push(x_list,ap->members[i].x_loc);
			other_lists = tm_list_push(other_lists,x_list);
			psp->props[0].name = "SubjectIndicators";
			psp->props[0].value = x_list;
		}


		assertion.memberships[i].player = psp;

	}
	assertion.N = ap->nmember;

	Omnivore_handleAssertion(ov,&assertion);
#if 0
	/* FIXME: this should be handle... */
	if(ov->emit_assertion_handler) 
               	ov->emit_assertion_handler(ov,&assertion);
#endif

	tm_list_delete(&type_sirs);
	tm_list_delete(&assertion_sirs);

	for(lp = other_lists; lp; lp=lp->next)
	{
		TMList k;
		k = (TMList)lp->content;
		tm_list_delete( &k );
	}
	tm_list_delete(&other_lists);
}


#endif


