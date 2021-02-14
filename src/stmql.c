/*
 * $Id: stmql.c,v 1.11 2002/10/02 22:06:25 jan Exp $
 *
 * Copyright (c) 2002 Jan Algermissen
 * See the file "COPYING" for copying permission.
 *
 */
#include <ctype.h>

#include "stmql.h"
#include "lookup.h"	/* for lookup_topic_by_locator() */

#include <util.h>
#include <mem.h>
#include <trace.h>
#define QUOTE_CHAR 39
#define UNQUOTE_CHAR 39
#define MACRO_CHAR '.' 
#define REGEX_CHAR '/' 

/* important, since the code assumes PARSE_FAILED == 1!! */
enum { PARSE_FAILED = 0, PARSE_OK = 1 };

/*
 
Syntax example
 
SELECT DISTINCT PLAYER OF role AS LOCATOR, 
  FROM ::AP_TOPIC_BASENAME:: 
 WHERE PLAYER OF topic IS X 
   AND PLAYER OF topic IN { topic topic topic }


 
stmql                   ::= select | grant | create
create                  ::= 'CREATE' 'GRAPH' locator
grant                   ::= 'GRANT'
select                  ::= 'SELECT' opt_distinct selection \
			    from_clause opt_where_clause

opt_distinct            ::= 'DISTINCT' | -nothing-
selection               ::= player_selection_comma_list

player_selection_comma_list
			::= player_selection
			  | player_selection_comma_list ',' player_selection

player_selection	::= portion 'AS' subject_repr_type

subject_repr_type	::= 'NODE' | 'LOCATOR' | 'DATA'
from_clause             ::= 'FROM' topic_ref

opt_where_clause	::= where_clause | -nothing-
where_clause		::= 'WHERE' search_condition

search_condition	::= search_condition 'AND' search_condition
			  | search_condition 'OR'  search_condition
			  | predicate

predicate		::= portion IS topic_ref 
                          | portion IN topic_set_ref

portion			::= player
			  | 'THIS'

player			::= 'PLAYER OF' topic_ref
			  | topic_ref

 
topic_ref                ::= topic_id | locator | macro | scr
topic_set_ref            ::= '{' topic_ref_comma_list '}'
			 
scr			::= '[' locator ']' 

topic_ref_comma_list	::= topic_ref
			  | topic_ref_comma_list ',' topic_ref


topic_id			:: [0-9]+		
locator                 ::= \S+ 
macro			::= '%' \S+ '%'

--------------------------------------------------------------
selection               ::= portion_comma_list
portion_comma_list      ::= portion
                          | portion_comma_list ',' portion
Notes:

\S matches any non-whitespace character
 
*/

/*
struct select_query {
	int distinct;
	gs_topic_t p_topic;


};
*/

#define CHECK(what) \
	do { \
	int len = strlen(#what); \
	if(ps->len >= len && strncmp(ps->ptr,#what,len) == 0) \
	{ \
		ps->ptr += len; \
		ps->len -= len; \
		return ps->current_token = what; \
	} \
	} while(0) 
 
enum {
        CREATE,
	GRANT,
	SELECT,
        GRAPH,
        NAME,
	NODE,
	BASENAME,
 
        DISTINCT,
	FROM,
	WHERE,
	AND,
	OR,
	IS,
	IN,
	THIS,
	PLAYER,
	OF,
	AS,
 
        COMMA,
	LCP, RCP, LSP, RSP, LAP, RAP, LP, RP, EQUAL,
	HYPHEN,
	REGEX,

	LOCATOR,
	INDICATOR,
	INDICATORDATA,
	DATA,
	ROTACOL,	/* token type LOCATOR !!!! */
	NODE_ID,
	MACRO,
	ERROR,	
        END
}; 

/* production functions assume that next token to analyse has
been called. The must call the next one befor returning !*/

static int get_token(struct gs_parse_state *ps);
static void set_error(struct gs_parse_state *ps,const char *fmt,...);
static int stmql(struct gs_parse_state *ps);
static int select(struct gs_parse_state *ps);
static int opt_distinct(struct gs_parse_state *ps);
static int selection(struct gs_parse_state *ps);
static int player_selection_comma_list(struct gs_parse_state *ps);
static int player_selection(struct gs_parse_state *ps);
static int subject_repr_type(struct gs_parse_state *ps);
static int opt_where_clause(struct gs_parse_state *ps);
static int where_clause(struct gs_parse_state *ps);
static int search_condition(struct gs_parse_state *ps);
static int predicate(struct gs_parse_state *ps);
static int portion(struct gs_parse_state *ps);
static int player(struct gs_parse_state *ps);
static int scr(struct gs_parse_state *ps);
/*
static int grant(struct gs_parse_state *ps);
*/
static int from_clause(struct gs_parse_state *ps);
static int topicset_ref(struct gs_parse_state *ps);
static int topic_ref(struct gs_parse_state *ps);
static int topic_ref_comma_list(struct gs_parse_state *ps);
static int create(struct gs_parse_state *ps);


static void set_syntax_error(struct gs_parse_state *ps);


void gs_parse_state_init(struct gs_parse_state *ps, GS_GRAPH_T g,const char *query)
{
	assert(ps);
	assert(query);
	bzero(ps,sizeof(*ps));
	ps->topicmap = g;
	bzero(ps->errstr,sizeof(ps->errstr));
	bzero(ps->text,sizeof(ps->text));
	ps->query = query;
	ps->ptr = ps->query;
	ps->len = strlen(query);
	
	ps->current_token = 0;
	ps->topic = 0;
	ps->topicset = gs_topicset_new(0 /* unused, 0 will break assertion later on*/ );
	/*
	ps->queries = GSStack_new(gw);
	ps->lastSet = NULL;
	*/
}

void gs_parse_state_cleanup(struct gs_parse_state *ps)
{
	assert(ps);

	gs_topicset_delete(&ps->topicset);
	if(ps->current_select_block->condition.x_topicset != NULL)
		gs_topicset_delete(&(ps->current_select_block->condition.x_topicset));
	bzero(ps,sizeof(*ps));
}

int gs_parse_query(struct gs_parse_state *ps)
{
	int token;

	if( (token = get_token(ps)) == END)
		return PARSE_FAILED;

	if( !stmql(ps) ) return PARSE_FAILED;
	
	return ( (ps->current_token == END) ? PARSE_OK : PARSE_FAILED );
}

/*
 * =======================================
 * stmql ::= select | create | grant
 * =======================================
 */
int stmql(struct gs_parse_state *ps)
{
	GSTRACE(GS_PARSE_TRACE,"ENTER stmql()\n");
	switch(ps->current_token) {
	case SELECT:
		GSTRACE(GS_PARSE_TRACE,"stmql(): SELECT\n"); 
		/* BEGIN init */
		ps->current_select_block = (struct select_block *)
			malloc(sizeof(struct select_block));
		assert(ps->current_select_block);
		ps->current_select_block->distinct = 0;
		ps->current_select_block->nselections = 0;
		ps->current_select_block->rr_topic = 0;
		ps->current_select_block->rx_topic = 0;
		ps->current_select_block->condition.r_topic = 0;
		ps->current_select_block->condition.x_topic = 0;
		ps->current_select_block->condition.x_topicset = NULL;	/* TBD (init function?)!! */
		ps->current_select_block->condition.type = COND_SINGLE; /* assume */
		/* END init */
		get_token(ps);
		if( !select(ps)) return PARSE_FAILED;
		break;
	case GRANT:
		GSTRACE(GS_PARSE_TRACE,"stmql(): GRANT\n"); 
		get_token(ps);
		/*
		grant(ps);
		*/
		break;
	case CREATE:
		GSTRACE(GS_PARSE_TRACE,"stmql(): CREATE\n"); 
		get_token(ps);
		if( !create(ps) ) return PARSE_FAILED;
		break;
	default:
		set_syntax_error(ps);
		return PARSE_FAILED; 
	}
	return PARSE_OK;
}
/*
 * =======================================
 * create ::= 'CREATE' 'GRAPH' locator  (abuse of locator for topicmap name)!! 
 * =======================================
 */
int create(struct gs_parse_state *ps)
{
	GSTRACE(GS_PARSE_TRACE,"ENTER create()\n");
	switch(ps->current_token) {
	case GRAPH:
		GSTRACE(GS_PARSE_TRACE,"create(): GRAPH\n"); 
		get_token(ps);
		assert(ps->current_token == ROTACOL);
		GSTRACE(GS_PARSE_TRACE,"DUMMY: create topicmap %s\n" _ 
			ps->text);	
		break;
	default:
		set_syntax_error(ps);
		return PARSE_FAILED; 
	}
	get_token(ps);
	return PARSE_OK;
}
/*
 * =======================================
 * select ::= "SELECT' opt_distinct from_clause opt_where_clause
 * =======================================
 */
int select(struct gs_parse_state *ps)
{
	GSTRACE(GS_PARSE_TRACE,"ENTER select()\n");
	if( !opt_distinct(ps)) return PARSE_FAILED;
	if( !selection(ps)) return PARSE_FAILED;
	if( !from_clause(ps)) return PARSE_FAILED;
	if( !opt_where_clause(ps)) return PARSE_FAILED;
	get_token(ps);
	return PARSE_OK;
}
/*
 * =======================================
 * opt_distinct ::= 'DISTINCT' | -nothing-
 * =======================================
 */
int opt_distinct(struct gs_parse_state *ps)
{
	GSTRACE(GS_PARSE_TRACE,"ENTER opt_distinct()\n");
	if(ps->current_token == DISTINCT)
	{
		GSTRACE(GS_PARSE_TRACE,"DUMMY: handle DISTINCT\n");
		ps->current_select_block->distinct = 1;
		get_token(ps);
	} 
	return PARSE_OK;
}
/*
 * =========================================
 * selection ::= player_selection_comma_list
 * =======================================
 */
int selection(struct gs_parse_state *ps)
{
	GSTRACE(GS_PARSE_TRACE,"ENTER selection()\n");
	if( !player_selection_comma_list(ps)) return PARSE_FAILED;
	return PARSE_OK;
}
/*
 * ===========================================================
 * player_selection_comma_list ::= player_selection
 *                               | player_selection_comma_list ',' pl_sel.
 * ============================================================
 */
int player_selection_comma_list(struct gs_parse_state *ps)
{
	GSTRACE(GS_PARSE_TRACE,"ENTER player_selection_comma_list()\n");

	while(1)
	{
		if( !player_selection(ps)) return PARSE_FAILED;
		if(ps->current_token != COMMA)	
			break;
		GSTRACE(GS_PARSE_TRACE,"player_selection_comma_list() COMMA\n");
		get_token(ps);
	}

	return PARSE_OK;
}
/*
 * ===========================================================
 * player_selection ::= portion 'AS' subject_repr_type 
 * ============================================================
 */
int player_selection(struct gs_parse_state *ps)
{
	GSTRACE(GS_PARSE_TRACE,"ENTER player_selection()\n");
	if( !portion(ps)) return PARSE_FAILED;
	ps->current_select_block->selections[ps->current_select_block->nselections].r_topic = ps->topic;
	if(ps->current_token != AS)
	{
		set_syntax_error(ps);
		return PARSE_FAILED;
	}
	get_token(ps);	
	if( !subject_repr_type(ps)) return PARSE_FAILED;
	ps->current_select_block->selections[ps->current_select_block->nselections].subject_repr_type = ps->intval;
	ps->current_select_block->nselections++;
	assert(ps->current_select_block->nselections < MAXSELECTIONS);
	return PARSE_OK;
}
/*
 * ===========================================================
 * subject_repr_type ::=
 * ============================================================
 */
int subject_repr_type(struct gs_parse_state *ps)
{
	GSTRACE(GS_PARSE_TRACE,"ENTER subject_repr_type()\n");
	switch(ps->current_token) {
	case LOCATOR:
		GSTRACE(GS_PARSE_TRACE,"subject_repr_type(): LOCATOR\n");
		GSTRACE(GS_PARSE_TRACE,"DUMMY ACTION FOR LOCATOR\n");
		ps->intval = SUBJECT_REPR_TYPE_LOCATOR;
		get_token(ps);
		break;
	case INDICATOR:
		GSTRACE(GS_PARSE_TRACE,"subject_repr_type(): INDICATOR\n");
		GSTRACE(GS_PARSE_TRACE,"DUMMY ACTION FOR INDICATOR\n");
		ps->intval = SUBJECT_REPR_TYPE_SIR_LOCATOR;
		get_token(ps);
		break;
	case INDICATORDATA:
		GSTRACE(GS_PARSE_TRACE,"subject_repr_type(): INDICATORDATA\n");
		GSTRACE(GS_PARSE_TRACE,"DUMMY ACTION FOR INDICATORDATA\n");
		ps->intval = SUBJECT_REPR_TYPE_SIR_DATA;
		get_token(ps);
		break;
	case BASENAME:
		GSTRACE(GS_PARSE_TRACE,"subject_repr_type(): BASENAME\n");
		GSTRACE(GS_PARSE_TRACE,"DUMMY ACTION FOR BASENAME\n");
		ps->intval = SUBJECT_REPR_TYPE_ANY_BASENAME;
		get_token(ps);
		break;
	case NODE:
		GSTRACE(GS_PARSE_TRACE,"subject_repr_type(): NODE\n");
		GSTRACE(GS_PARSE_TRACE,"DUMMY ACTION FOR NODE\n");
		ps->intval = SUBJECT_REPR_TYPE_NODE;
		get_token(ps);
		break;
	case DATA:
		GSTRACE(GS_PARSE_TRACE,"subject_repr_type(): DATA\n");
		GSTRACE(GS_PARSE_TRACE,"DUMMY ACTION FOR DATA\n");
		ps->intval = SUBJECT_REPR_TYPE_DATA;
		get_token(ps);
		break;
	default:
		set_syntax_error(ps);
		return PARSE_FAILED;
	}
	return PARSE_OK;
}

/*
 * =======================================
 * opt_where_clause ::= where_clause | -nothing-
 * =======================================
 */
int opt_where_clause(struct gs_parse_state *ps)
{
	GSTRACE(GS_PARSE_TRACE,"ENTER opt_where_clause()\n");
	if(ps->current_token == WHERE)
	{
		get_token(ps);
		if( !where_clause(ps)) return PARSE_FAILED;
	}
	return PARSE_OK;
}
/*
 * =======================================
 * where_clause ::= 'WHERE' search_condition
 * =======================================
 */
int where_clause(struct gs_parse_state *ps)
{
	GSTRACE(GS_PARSE_TRACE,"ENTER where_clause()\n");
	if( !search_condition(ps)) return PARSE_FAILED;
	get_token(ps);
	return PARSE_OK;
}
/*
 * =======================================
 * search_condition ::= search_condition 'AND' serach_condition
 *                    | search_condition 'OR' search_condition
 *                    | predicate 
 * =======================================
 */
int search_condition(struct gs_parse_state *ps)
{
	GSTRACE(GS_PARSE_TRACE,"ENTER search_condition()\n");
	switch(ps->current_token) {
	case AND:
		assert(0);
		break;
	case OR:
		assert(0);
		break;
	default:
		/* get_token(ps); */
		if( !predicate(ps)) return PARSE_FAILED;
	}
	get_token(ps);
	return PARSE_OK;
}
 
/*
 * =======================================
 * predicate ::= portion 'IS' topic_ref
 * =======================================
 */
int predicate(struct gs_parse_state *ps)
{
	GSTRACE(GS_PARSE_TRACE,"ENTER predicate()\n");
	if( !portion(ps)) return PARSE_FAILED;
	assert(ps->topic);	/* not sure if it's a correct assertion */
	ps->current_select_block->rr_topic = ps->topic;
	ps->current_select_block->condition.r_topic = ps->topic;
	switch(ps->current_token) {
	case IS:
		get_token(ps);
		if( !topic_ref(ps)) return PARSE_FAILED;
		ps->current_select_block->condition.type = COND_SINGLE;
		ps->current_select_block->rx_topic = ps->topic;
		ps->current_select_block->condition.x_topic = ps->topic;
		break;
	case IN:
		get_token(ps);
		if( !topicset_ref(ps)) return PARSE_FAILED;
		ps->current_select_block->condition.type = COND_SET;
		ps->current_select_block->condition.x_topicset = ps->topicset;
		ps->topicset = gs_topicset_new(0);
		break;	
	default:
		set_syntax_error(ps);
		return PARSE_FAILED; 
	}
	get_token(ps);
	return PARSE_OK;
}
/*
 * =======================================
 * portion ::= player | 'THIS' 
 * =======================================
 */
int portion(struct gs_parse_state *ps)
{
	GSTRACE(GS_PARSE_TRACE,"ENTER portion()\n");
	switch(ps->current_token) {
	case THIS:
		GSTRACE(GS_PARSE_TRACE,"DUMMY ACTION FOR THIS\n");
		ps->topic = 0; /* indicates THIS */
		get_token(ps);
		break;
	default:
		if( !player(ps)) return PARSE_FAILED;
	}
	return PARSE_OK;
}
/*
 * =======================================
 * player ::= 'PLAYER OF' topic_ref
 *          | topic_ref 
 * =======================================
 */
int player(struct gs_parse_state *ps)
{
	GSTRACE(GS_PARSE_TRACE,"ENTER player()\n");
	if(ps->current_token == PLAYER)
	{
		get_token(ps);
		assert(ps->current_token == OF);
		get_token(ps);
	}
	
	if( !topic_ref(ps)) return PARSE_FAILED;
	/*
	get_token(ps);
	*/
	return PARSE_OK;
}
/*
 * =======================================
 * from_clause ::= 'FROM' topic_ref
 * =======================================
 */
int from_clause(struct gs_parse_state *ps)
{
	GSTRACE(GS_PARSE_TRACE,"ENTER from_clause()\n");
	get_token(ps);	/* skip from */
	if( !topic_ref(ps)) return PARSE_FAILED;
	ps->current_select_block->p_topic = ps->topic;
	/* TBD: look for ptopic ? */
	return PARSE_OK;
}
/*
 * =======================================
 * scr ::= '[' locator ']' 
 * =======================================
 */
int scr(struct gs_parse_state *ps)
{
	GSError_T e;
	GSTRACE(GS_PARSE_TRACE,"ENTER scr()\n");	
	switch(ps->current_token) {
	case ROTACOL:
		GSTRACE(GS_PARSE_TRACE,"scr(): ROTACOL %s\n" _
			ps->text);
		if( (e = gs_lookup_topic_by_locator(ps->topicmap,ps->text,GS_SCR,
			GS_LOOKUP, &(ps->topic),NULL)) != GS_OK)
		{
			set_error(ps,"cannot lookup topic for %s, %s",
				ps->text, gs_strerror(e));
			return PARSE_FAILED;

		}
		if(ps->topic == GS_NO_NODE)
		{
			set_error(ps,"locator %s not in the topicmap", ps->text);
			return PARSE_FAILED;
		}
		break;
	default:
		set_syntax_error(ps);
		return PARSE_FAILED;
	}
	get_token(ps);
	assert(ps->current_token == RSP);
	get_token(ps);
	return PARSE_OK;
}
/*
 * =======================================
 * topic_ref ::= topic_id 
 *            | locator
 *            | macro
 *            | scr
 * ========================================
 */
int topic_ref(struct gs_parse_state *ps)
{
	int e;
	GSTRACE(GS_PARSE_TRACE,"ENTER topic_ref()\n");
	switch(ps->current_token) {
	case LSP:
		GSTRACE(GS_PARSE_TRACE,"topic_ref(): LSP\n");
		get_token(ps);
		if( !scr(ps)) return PARSE_FAILED;
		break;
	case NODE_ID:
		GSTRACE(GS_PARSE_TRACE,"topic_ref(): NODE_ID %d\n" _
			ps->topic);
		/* ps->topic set, no action needed */
		break;
	case ROTACOL:
		GSTRACE(GS_PARSE_TRACE,"topic_ref(): LOACTOR %s\n" _
			ps->text);
		if( (e = gs_lookup_topic_by_locator(ps->topicmap,ps->text,GS_SIR,
			GS_LOOKUP, &(ps->topic),NULL)) != GS_OK)
		{
			set_error(ps,"cannot lookup topic for %s, %s",
				ps->text, gs_strerror(e));
			return PARSE_FAILED;

		}
		if(ps->topic == GS_NO_NODE)
		{
			set_error(ps,"locator %s not in the topicmap", ps->text);
			return PARSE_FAILED;
		}
		break;
	case MACRO:
		GSTRACE(GS_PARSE_TRACE,"topic_ref(): MACRO %s\n" _
			ps->text);


		if(strcmp(ps->text,"aptsi") == 0) {
			ps->topic = ps->topicmap->ap_topic_subjectIndicator_topic;
		} else if(strcmp(ps->text,"topic") == 0) {
			ps->topic = ps->topicmap->role_topic_topic;
		} else if(strcmp(ps->text,"si") == 0) {
			ps->topic = ps->topicmap->role_subjectIndicator_topic;
		} else {
			const char *loc;
			if((loc = gs_lookup_macro(ps->topicmap,ps->text)) == NULL)
			{
				set_error(ps,"macro %s not known", ps->text);
				return PARSE_FAILED;
			}
			if( (e = gs_lookup_topic_by_locator(ps->topicmap,loc,GS_SIR,
				GS_LOOKUP, &(ps->topic),NULL)) != GS_OK)
			{
				set_error(ps,"cannot lookup topic for %s, %s",
					loc, gs_strerror(e));
				return PARSE_FAILED;
			}
			if(ps->topic == GS_NO_NODE)
			{
				set_error(ps,"macro %s resolved to locator %s, but that is not in the topicmap", 
					ps->text,
					gs_lookup_macro(ps->topicmap,ps->text));
				return PARSE_FAILED;
			}
		}
		break;
	default:
		set_syntax_error(ps);
		return PARSE_FAILED; 
	}
	/* take action here or wait ?
	if(!ptopic....)
	*/
	GSTRACE(GS_PARSE_TRACE,"exit topic_ref(): NODE_ID %d\n" _ ps->topic);
		
	get_token(ps);
	return PARSE_OK;
}
/*
 * =======================================
 * topic_set_ref ::= '{' topic_ref_comma_list '}'
 * ========================================
 */
int topicset_ref(struct gs_parse_state *ps)
{
	/*
	int e;
	*/
	GSTRACE(GS_PARSE_TRACE,"ENTER topic_set_ref()\n");
	gs_topicset_clear(ps->topicset);
	switch(ps->current_token) {
	case LCP:
		GSTRACE(GS_PARSE_TRACE,"topic_set_ref(): LCP\n");
		get_token(ps);
		if( !topic_ref_comma_list(ps)) return PARSE_FAILED;
		break;
	case HYPHEN:
		get_token(ps);
		assert(ps->current_token == REGEX);
		gs_lookup_topics_by_basename(ps->topicmap,ps->text,ps->topicset);
		break;
	case LSP:
		get_token(ps);
		assert(ps->current_token == REGEX);
		gs_lookup_topics_by_scr_data(ps->topicmap,ps->text,ps->topicset); 
		break;
	case REGEX: /* SIR DATA LIKE /regex/ */
		gs_lookup_topics_by_sir_data(ps->topicmap,ps->text,ps->topicset); 
		break;
#if 0
	case NODE_ID:
		GSTRACE(GS_PARSE_TRACE,"topic_ref(): NODE_ID %d\n" _
			ps->topic);
		/* ps->topic set, no action needed */
		break;
	case ROTACOL:
		GSTRACE(GS_PARSE_TRACE,"topic_ref(): LOACTOR %s\n" _
			ps->text);
		if( (e = gs_lookup_topic_by_locator(ps->topicmap,ps->text,GS_SIR,
			GS_LOOKUP, &(ps->topic),NULL)) != GS_OK)
		{
			set_error(ps,"cannot lookup topic for %s, %s",
				ps->text, gs_strerror(e));
			return PARSE_FAILED;

		}
		if(ps->topic == GS_NO_NODE)
		{
			set_error(ps,"locator %s not in the topicmap", ps->text);
			return PARSE_FAILED;
		}
		break;
	case MACRO:
		GSTRACE(GS_PARSE_TRACE,"topic_ref(): MACRO %s\n" _
			ps->text);


		if(strcmp(ps->text,"aptsi") == 0) {
			ps->topic = ps->topicmap->ap_topic_subjectIndicator_topic;
		} else if(strcmp(ps->text,"topic") == 0) {
			ps->topic = ps->topicmap->role_topic_topic;
		} else if(strcmp(ps->text,"si") == 0) {
			ps->topic = ps->topicmap->role_subjectIndicator_topic;
		} else {
			const char *loc;
			if((loc = gs_lookup_macro(ps->topicmap,ps->text)) == NULL)
			{
				set_error(ps,"macro %s not known", ps->text);
				return PARSE_FAILED;
			}
			if( (e = gs_lookup_topic_by_locator(ps->topicmap,loc,GS_SIR,
				GS_LOOKUP, &(ps->topic),NULL)) != GS_OK)
			{
				set_error(ps,"cannot lookup topic for %s, %s",
					loc, gs_strerror(e));
				return PARSE_FAILED;
			}
			if(ps->topic == GS_NO_NODE)
			{
				set_error(ps,"macro %s resolved to locator %s, but that is not in the topicmap", 
					ps->text,
					gs_lookup_macro(ps->topicmap,ps->text));
				return PARSE_FAILED;
			}
		}
		break;
#endif
	default:
		set_syntax_error(ps);
		return PARSE_FAILED; 
	}
	/* take action here or wait ?
	if(!ptopic....)
	*/
	GSTRACE(GS_PARSE_TRACE,"exit topic_set_ref(): set length %d\n" _ -555);
		
	get_token(ps);
	return PARSE_OK;
}
/*
 * ===========================================================
 * topic_ref_comma_list ::= topic_ref
 *                       | topic_ref_comma_list ',' topic_ref
 * ============================================================
 */
int topic_ref_comma_list(struct gs_parse_state *ps)
{
	GSTRACE(GS_PARSE_TRACE,"ENTER topic_ref_comma_list()\n");

	while(1)
	{
		if( !topic_ref(ps)) return PARSE_FAILED;
		gs_topicset_add(ps->topicset,ps->topic);
		if(ps->current_token != COMMA)	
			break;
		GSTRACE(GS_PARSE_TRACE,"topic_ref_comma_list() COMMA\n");
		get_token(ps);
	}

	return PARSE_OK;
}

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

int get_token(struct gs_parse_state *ps)
{
	char *p;
	GSTRACE(GS_PARSE_TRACE,
		"get_token (before \\0 check): parse position now %s\n" _
		ps->ptr);
	if(*ps->ptr == '\0')
	{
		assert(ps->len == 0);
		return (ps->current_token = END);	
	}
	GSTRACE(GS_GRAPH_TRACE,
		"get_token (before WS stripping): parse position now %s\n" _
		ps->ptr);
	while(*ps->ptr == '\n' || isspace(*ps->ptr) )
	{	
		ps->ptr++;
		ps->len--;
		if(*ps->ptr == '\0')
		{
			assert(ps->len == 0);
			return (ps->current_token = END);	
		}
	} 


	GSTRACE(GS_PARSE_TRACE,
		"get_token (after WS stripping): parse position now %s\n" _
		ps->ptr);

	switch(*ps->ptr) {
	case ',': ps->ptr++; ps->len--; return ps->current_token = COMMA;
	case '{': ps->ptr++; ps->len--; return ps->current_token = LCP;
	case '}': ps->ptr++; ps->len--; return ps->current_token = RCP;
	case '[': ps->ptr++; ps->len--; return ps->current_token = LSP;
	case ']': ps->ptr++; ps->len--; return ps->current_token = RSP;
	case '<': ps->ptr++; ps->len--; return ps->current_token = LAP;
	case '>': ps->ptr++; ps->len--; return ps->current_token = RAP;
	case '(': ps->ptr++; ps->len--; return ps->current_token = LP;
	case ')': ps->ptr++; ps->len--; return ps->current_token = RP;
	case '=': ps->ptr++; ps->len--; return ps->current_token = EQUAL;
	case '-': ps->ptr++; ps->len--; return ps->current_token = HYPHEN;
	
	case MACRO_CHAR: {
		char *p = ps->text;
		bzero(ps->text,sizeof(ps->text));
		ps->ptr++; ps->len--;
		while(*ps->ptr && *ps->ptr != MACRO_CHAR)
		{
			/* fprintf(stderr,"%c",*ps->ptr); */
			*p++ = *ps->ptr++;
			ps->len--;
		}
		/* fprintf(stderr,"\n"); */
		if(*ps->ptr == '\0')
		{
			;

		}
		/* skip macro char */
		ps->ptr++; ps->len--;
		return ps->current_token = MACRO; }	
	case REGEX_CHAR: {
		char *p = ps->text;
		bzero(ps->text,sizeof(ps->text));
		ps->ptr++; ps->len--;
		while(*ps->ptr && *ps->ptr != REGEX_CHAR)
		{
			/* fprintf(stderr,"%c",*ps->ptr); */
			*p++ = *ps->ptr++;
			ps->len--;
		}
		/* fprintf(stderr,"\n"); */
		if(*ps->ptr == '\0')
		{
			;

		}
		/* skip regex char */
		ps->ptr++; ps->len--;
		return ps->current_token = REGEX; }	
	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9': {
		char buf[43];	/* enough for any number */
		char *p = buf;
		bzero(buf,sizeof(buf));
		while(*ps->ptr && isdigit(*ps->ptr) )
		{
			 /*fprintf(stderr,"%c",*ps->ptr); */
			*p++ = *ps->ptr++;
			ps->len--;
		}
		/*fprintf(stderr,"\n"); */
		ps->topic = atoi(buf);
		/*fprintf(stderr,"INDEX: %d\n",ps->index);*/
		return ps->current_token = NODE_ID; }	
		break;
	case 'A':
		CHECK( AND );
		CHECK( AS );
		break;
	case 'B':
		CHECK( BASENAME );
		break;
	case 'C':
		CHECK( CREATE );
		break;
	case 'D':
		CHECK( DISTINCT );
		CHECK( DATA );
		break;
	case 'F':
		CHECK( FROM );
		break;
	case 'G':
		CHECK( GRAPH );
		CHECK( GRANT );

		break;
	case 'I':
		CHECK( INDICATORDATA );
		CHECK( INDICATOR );
		CHECK( IS );
		CHECK( IN );

		break;
	case 'L':
		CHECK( LOCATOR );
		break;
	case 'N':
		CHECK( NODE );
		break;
	case 'O':
		CHECK( OR );
		CHECK( OF );
		break;
	case 'P':
		CHECK( PLAYER );
		break;
	case 'S':
		CHECK( SELECT );
		break;
	case 'T':
		CHECK( THIS );
		break;
	case 'W':
		CHECK( WHERE );
		break;
	default:
		break;
	}

	/* free text is locator */
	bzero(ps->text,sizeof(ps->text));
	p = ps->text;
	while(*ps->ptr != '\0' && (! isspace(*ps->ptr)))
	{
		*p++ = *ps->ptr++;
		ps->len--;
	}
	return (ps->current_token = ROTACOL);	

	set_error(ps,"syntax error at offset %d near %s\n",
		ps->ptr - ps->query, ps->ptr);
	return ps->current_token = ERROR;	
}


#if 0 
 LOCATOR,
        NODE_ID,
        AP_TOPIC_SUBJECTINDICATOR,
        MACRO,
GS_TOKEN_T get_token(struct gs_parse_state *ps)
{
	GSTRACE(GS_GRAPH_TRACE,"getToken (before \\0 check): parse position now %s\n" _
		qp->ptr);
	if(*qp->ptr == '\0') {
		assert(qp->len == 0);
		return qp->currentToken = END;	
	}
	GSTRACE(GS_GRAPH_TRACE,"getToken (before WS stripping): parse position now %s\n" _
		qp->ptr);
	while(*qp->ptr == '\n' || isspace(*qp->ptr) )
	{	
		qp->ptr++;
		qp->len--;
		if(*qp->ptr == '\0') {
			assert(qp->len == 0);
			return qp->currentToken = END;	
		}
	} 


	GSTRACE(GS_GRAPH_TRACE,"getToken (after WS stripping): parse position now %s\n" _
		qp->ptr);

	switch(*qp->ptr) {
	case ',': qp->ptr++; qp->len--; return qp->currentToken = KOMMA;
	case '{': qp->ptr++; qp->len--; return qp->currentToken = LCP;
	case '}': qp->ptr++; qp->len--; return qp->currentToken = RCP;
	case '(': qp->ptr++; qp->len--; return qp->currentToken = LP;
	case ')': qp->ptr++; qp->len--; return qp->currentToken = RP;
	case '=': qp->ptr++; qp->len--; return qp->currentToken = EQUAL;

	case 'R':
		if(qp->len >= 12 && strncmp(qp->ptr,"RESOURCEDATA",12) == 0)
		{
			qp->ptr += 12;
			qp->len -= 12;
			return qp->currentToken = RESOURCEDATA;
		}
		GS_setError(qp->gw,GS_ESYNTAX,"sTMQL syntax error at offset %d near %s\n",
			qp->ptr - qp->query, qp->ptr);
		return qp->currentToken = END;	
	case '_':
		if(qp->len >= 4 && strncmp(qp->ptr,"_AX_",4) == 0)
		{
			qp->ptr += 4;
			qp->len -= 4;
			return qp->currentToken = _AX_;
		}
		if(qp->len >= 4 && strncmp(qp->ptr,"_XA_",4) == 0)
		{
			qp->ptr += 4;
			qp->len -= 4;
			return qp->currentToken = _XA_;
		}
		if(qp->len >= 4 && strncmp(qp->ptr,"_AS_",4) == 0)
		{
			qp->ptr += 4;
			qp->len -= 4;
			return qp->currentToken = _AS_;
		}
		if(qp->len >= 4 && strncmp(qp->ptr,"_SA_",4) == 0)
		{
			qp->ptr += 4;
			qp->len -= 4;
			return qp->currentToken = _SA_;
		}
		if(qp->len >= 4 && strncmp(qp->ptr,"_SC_",4) == 0)
		{
			qp->ptr += 4;
			qp->len -= 4;
			return qp->currentToken = _SC_;
		}
		if(qp->len >= 4 && strncmp(qp->ptr,"_CS_",4) == 0)
		{
			qp->ptr += 4;
			qp->len -= 4;
			return qp->currentToken = _CS_;
		}
		GS_setError(qp->gw,GS_ESYNTAX,"sTMQL syntax error at offset %d near %s\n",
			qp->ptr - qp->query, qp->ptr);
		return qp->currentToken = END;	
	case 'F':
		if(qp->len >= 4 && strncmp(qp->ptr,"FROM",4) == 0)
		{
			qp->ptr += 4;
			qp->len -= 4;
			return qp->currentToken = FROM;
		}
		GS_setError(qp->gw,GS_ESYNTAX,"sTMQL syntax error at offset %d near %s\n",
			qp->ptr - qp->query, qp->ptr);
		return qp->currentToken = END;	
	case 'L':
		if(qp->len >= 4 && strncmp(qp->ptr,"LIKE",4) == 0)
		{
			qp->ptr += 4;
			qp->len -= 4;
			return qp->currentToken = LIKE;
		}
		GS_setError(qp->gw,GS_ESYNTAX,"sTMQL syntax error at offset %d near %s\n",
			qp->ptr - qp->query, qp->ptr);
		return qp->currentToken = END;	
	case 'D':
		if(qp->len >= 4 && strncmp(qp->ptr,"DONE",4) == 0)
		{
			qp->ptr += 4;
			qp->len -= 4;
			return qp->currentToken = DONE;
		}
		if(qp->len >= 4 && strncmp(qp->ptr,"DATA",4) == 0)
		{
			qp->ptr += 4;
			qp->len -= 4;
			return qp->currentToken = DATA;
		}
		if(qp->len >= 2 && strncmp(qp->ptr,"DO",2) == 0)
		{
			qp->ptr += 2;
			qp->len -= 2;
			return qp->currentToken = DO;
		}
		GS_setError(qp->gw,GS_ESYNTAX,"sTMQL syntax error at offset %d near %s\n",
			qp->ptr - qp->query, qp->ptr);
		return qp->currentToken = END;	
	case 'I':
		if(qp->len >= 7 && strncmp(qp->ptr,"INDEXES",7) == 0)
		{
			qp->ptr += 7;
			qp->len -= 7;
			return qp->currentToken = INDEX_TYPE;
		}
		GS_setError(qp->gw,GS_ESYNTAX,"sTMQL syntax error at offset %d near %s\n",
			qp->ptr - qp->query, qp->ptr);
		return qp->currentToken = END;	
	case 'A':
		if(qp->len >= 2 && strncmp(qp->ptr,"AS",2) == 0)
		{
			qp->ptr += 2;
			qp->len -= 2;
			return qp->currentToken = AS;
		}
		if(qp->len >= 9 && strncmp(qp->ptr,"ALLSCOPES",9) == 0)
		{
			qp->ptr += 9;
			qp->len -= 9;
			return qp->currentToken = ALLSCOPES;
		}
		if(qp->len >= 3 && strncmp(qp->ptr,"ALL",3) == 0)
		{
			qp->ptr += 3;
			qp->len -= 3;
			return qp->currentToken = ALL;
		}
		if(qp->len >= 7 && strncmp(qp->ptr,"ANYROLE",7) == 0)
		{
			qp->ptr += 7;
			qp->len -= 7;
			return qp->currentToken = ANYROLE;
		}
		GS_setError(qp->gw,GS_ESYNTAX,"sTMQL syntax error at offset %d near %s\n",
			qp->ptr - qp->query, qp->ptr);
		return qp->currentToken = END;	
	case 'a':
		if(qp->len >= 4 && strncmp(qp->ptr,"aAXx",4) == 0)
		{
			qp->ptr += 4;
			qp->len -= 4;
			return qp->currentToken = aAXx;
		}
		if(qp->len >= 4 && strncmp(qp->ptr,"aAMm",4) == 0)
		{
			qp->ptr += 4;
			qp->len -= 4;
			return qp->currentToken = aAMm;
		}
		if(qp->len >= 4 && strncmp(qp->ptr,"aAMr",4) == 0)
		{
			qp->ptr += 4;
			qp->len -= 4;
			return qp->currentToken = aAMr;
		}
		if(qp->len >= 4 && strncmp(qp->ptr,"aAMp",4) == 0)
		{
			qp->ptr += 4;
			qp->len -= 4;
			return qp->currentToken = aAMr;
		}
		GS_setError(qp->gw,GS_ESYNTAX,"sTMQL syntax error at offset %d near %s\n",
			qp->ptr - qp->query, qp->ptr);
		return qp->currentToken = END;	
	case 'x':
		if(qp->len >= 4 && strncmp(qp->ptr,"xAXa",4) == 0)
		{
			qp->ptr += 4;
			qp->len -= 4;
			return qp->currentToken = xAXa;
		}
		GS_setError(qp->gw,GS_ESYNTAX,"sTMQL syntax error at offset %d near %s\n",
			qp->ptr - qp->query, qp->ptr);
		return qp->currentToken = END;	
	case 'm':
		if(qp->len >= 4 && strncmp(qp->ptr,"mAMa",4) == 0)
		{
			qp->ptr += 4;
			qp->len -= 4;
			return qp->currentToken = mAMa;
		}
		if(qp->len >= 4 && strncmp(qp->ptr,"mAMr",4) == 0)
		{
			qp->ptr += 4;
			qp->len -= 4;
			return qp->currentToken = mAMr;
		}
		if(qp->len >= 4 && strncmp(qp->ptr,"mAMp",4) == 0)
		{
			qp->ptr += 4;
			qp->len -= 4;
			return qp->currentToken = mAMr;
		}
		GS_setError(qp->gw,GS_ESYNTAX,"sTMQL syntax error at offset %d near %s\n",
			qp->ptr - qp->query, qp->ptr);
		return qp->currentToken = END;	
	case 'p':
		if(qp->len >= 4 && strncmp(qp->ptr,"pAMa",4) == 0)
		{
			qp->ptr += 4;
			qp->len -= 4;
			return qp->currentToken = rAMa;
		}
		if(qp->len >= 4 && strncmp(qp->ptr,"pAMm",4) == 0)
		{
			qp->ptr += 4;
			qp->len -= 4;
			return qp->currentToken = rAMm;
		}
		GS_setError(qp->gw,GS_ESYNTAX,"sTMQL syntax error at offset %d near %s\n",
			qp->ptr - qp->query, qp->ptr);
		return qp->currentToken = END;	
	case 'r':
		if(qp->len >= 4 && strncmp(qp->ptr,"rAMa",4) == 0)
		{
			qp->ptr += 4;
			qp->len -= 4;
			return qp->currentToken = rAMa;
		}
		if(qp->len >= 4 && strncmp(qp->ptr,"rAMm",4) == 0)
		{
			qp->ptr += 4;
			qp->len -= 4;
			return qp->currentToken = rAMm;
		}
		GS_setError(qp->gw,GS_ESYNTAX,"sTMQL syntax error at offset %d near %s\n",
			qp->ptr - qp->query, qp->ptr);
		return qp->currentToken = END;	
	case 'B':
		if(qp->len >= 14 && strncmp(qp->ptr,"BASENAMESTRING",14) == 0)
		{
			qp->ptr += 14;
			qp->len -= 14;
			return qp->currentToken = BASENAMESTRING;
		}
		if(qp->len >= 4 && strncmp(qp->ptr,"BASE",4) == 0)
		{
			qp->ptr += 4;
			qp->len -= 4;
			return qp->currentToken = BASE;
		}
		GS_setError(qp->gw,GS_ESYNTAX,"sTMQL syntax error at offset %d near %s\n",
			qp->ptr - qp->query, qp->ptr);
		return qp->currentToken = END;	
	case 'S':
		if(qp->len >= 8 && strncmp(qp->ptr,"SUBJECTS",8) == 0)
		{
			qp->ptr += 8;
			qp->len -= 8;
			return qp->currentToken = SUBJECT_TYPE;
		}
		if(qp->len >= 7 && strncmp(qp->ptr,"STRINGS",7) == 0)
		{
			qp->ptr += 7;
			qp->len -= 7;
			return qp->currentToken = STRING_TYPE;
		}
		GS_setError(qp->gw,GS_ESYNTAX,"sTMQL syntax error at offset %d near %s\n",
			qp->ptr - qp->query, qp->ptr);
		return qp->currentToken = END;	
	case 'U':
		if(qp->len >= 4 && strncmp(qp->ptr,"URIS",4) == 0)
		{
			qp->ptr += 4;
			qp->len -= 4;
			return qp->currentToken = URI_TYPE;
		}
		GS_setError(qp->gw,GS_ESYNTAX,"sTMQL syntax error at offset %d near %s\n",
			qp->ptr - qp->query, qp->ptr);
		return qp->currentToken = END;	
	case 'M':
		if(qp->len >= 10 && strncmp(qp->ptr,"MATCHSCOPE",10) == 0)
		{
			qp->ptr += 10;
			qp->len -= 10;
			return qp->currentToken = MATCHSCOPE;
		}
		GS_setError(qp->gw,GS_ESYNTAX,"sTMQL syntax error at offset %d near %s\n",
			qp->ptr - qp->query, qp->ptr);
		return qp->currentToken = END;	
	case 'T':
		if(qp->len >= 8 && strncmp(qp->ptr,"TRAVERSE",8) == 0)
		{
			qp->ptr += 8;
			qp->len -= 8;
			return qp->currentToken = TRAVERSE;
		}
		GS_setError(qp->gw,GS_ESYNTAX,"sTMQL syntax error at offset %d near %s\n",
			qp->ptr - qp->query, qp->ptr);
		return qp->currentToken = END;	
	case QUOTE_CHAR: {
		char *p = qp->text;
		bzero(qp->text,sizeof(qp->text));
		qp->ptr++; qp->len--;
		while(*qp->ptr && *qp->ptr != UNQUOTE_CHAR)
		{
			/* fprintf(stderr,"%c",*qp->ptr); */
			*p++ = *qp->ptr++;
			qp->len--;
		}
		/* fprintf(stderr,"\n"); */
		assert(*qp->ptr != '\0');
		/* skip unquote_char */
		qp->ptr++; qp->len--;
		return qp->currentToken = URI; }	
	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9': {
		char buf[43];	/* enough for any number */
		char *p = buf;
		bzero(buf,sizeof(buf));
		while(*qp->ptr && isdigit(*qp->ptr) )
		{
			 /*fprintf(stderr,"%c",*qp->ptr); */
			*p++ = *qp->ptr++;
			qp->len--;
		}
		/*fprintf(stderr,"\n"); */
		assert(*qp->ptr != '\0');
		qp->index = atoi(buf);
		/*fprintf(stderr,"INDEX: %d\n",qp->index);*/
		return qp->currentToken = INDEX; }	
	default:
		GS_setError(qp->gw,GS_ESYNTAX,"sTMQL syntax error at offset %d near %s\n",
			qp->ptr - qp->query, qp->ptr);
		return qp->currentToken = END;	
	}

	/* not reached */
	return 0;
}

void doQuery(struct parseState *ps)
{
	GSUri_T baseUri = GSUri_new(ps->gw,"a","http://www.topicmaps.org/xtm/1.0/psi1.xtm");
	struct query *qp;
	GSTRACE(GS_GRAPH_TRACE,"-doQuery-\n");
	switch(ps->currentToken) {
	case BASE:
		getToken(ps);
		assert(ps->currentToken == URI);
		GSUri_delete(&baseUri);
		baseUri = GSUri_new(ps->gw,"a",ps->text);	
		getToken(ps);
		/* FALL THROUGH !! */
	case FROM:
		GS_NEW(ps->gw,qp);
		qp->path = NULL;
		qp->baseUri = baseUri;	/* NULL or was set */
		GSStack_push(ps->queries,qp);
		ps->currentQuery = qp;
		getToken(ps);
		doSet(ps);

		qp->seedSet = ps->lastSet;	
		
		/* getToken(ps); */
		assert(ps->currentToken == DO);
		/*
		GS_NEW(ps->gw,ps->path);
		*/

		qp->path = NULL;
		getToken(ps);


		/* This is the empty path */
		if(ps->currentToken != DONE)
		{

			doPath(ps);

			/* path has bee build */

			ps->lastSet = evaluatePathExpression(ps,qp->seedSet,ps->currentQuery->path);

			assert(GSStack_pop(ps->queries) == qp);
			/* cleanup query */
			if(GSStack_size(ps->queries) > 0)
				ps->currentQuery = (struct query *)GSStack_top(ps->queries);
			else
				ps->currentQuery = NULL;

		}
		assert(ps->currentToken == DONE);
		getToken(ps);

		if(ps->currentToken == AS)
		{
			getToken(ps);
			/*
			printf("token %d\n",ps->currentToken);
			*/
			switch(ps->currentToken) {
			case SUBJECT_TYPE:
				*ps->type = GS_SUBJECT_TYPE;
				break;
			case URI_TYPE:
				*ps->type = GS_URI_TYPE;
				break;
			case STRING_TYPE:
				*ps->type = GS_STRING_TYPE;
				break;
			case INDEX_TYPE:
				*ps->type = GS_INDEX_TYPE;
				break;
			default:
				assert(!"wrong type");
			}
			getToken(ps);
			getToken(ps);
			assert(ps->currentToken == END);
			/*
			printf("token %d\n",ps->currentToken);
			*/
			getToken(ps);
			/*
			printf("token %d\n",ps->currentToken);
			*/
		}



		break;
	default:
		assert(!"syntax error");
	}
}

/* most important: ps->lastSet will be set to set after calling doSet */
void doSet(struct parseState *ps)
{
	GSTRACE(GS_GRAPH_TRACE,"-doSet-\n");
	switch(ps->currentToken) {
	case ALL:
		/* TBD if last Set delete set ? */
		ps->lastSet = GSNodeSet_new(ps->gw);	
		ps->topicmapClass->topicmapMakeAll(ps->transaction,ps->lastSet);
		getToken(ps);
		break;
	case ALLSCOPES:
		/* TBD if last Set delete set ? */
		ps->lastSet = GSNodeSet_new(ps->gw);	
		ps->topicmapClass->allSNodes(ps->transaction,ps->lastSet);
		getToken(ps);
		break;
	case FROM:
		doQuery(ps);
		/* lastSet is set now */
		/* getToken is called in doQuery();	*/
		break;
	case ANYROLE:
		ps->lastSet = GSNodeSet_new(ps->gw);
		getToken(ps);
		break;
	case BASENAMESTRING:
	case RESOURCEDATA:
	case DATA: {
		int save_tok = ps->currentToken;
		GSToken_T cmpOp;	
		ps->lastSet = GSNodeSet_new(ps->gw);
		getToken(ps);
		cmpOp = ps->currentToken;
		assert(ps->currentToken == LIKE || ps->currentToken == EQUAL);
		getToken(ps);
		/*TBD!  text OR URI */	
		assert(ps->currentToken == URI);
		switch(cmpOp) {
		case LIKE: {
			int flag = 0;
			switch(save_tok) {
			case DATA: flag = GS_LIKE; break;
			case BASENAMESTRING: flag = GS_BASENAMESTRING_LIKE; break;
			case RESOURCEDATA: flag = GS_RESOURCEDATA_LIKE; break;
			}
			ps->topicmapClass->lookupNodesByData(
				ps->transaction,
				flag,
				ps->text,
				ps->lastSet);
			break; }
		case EQUAL: {
			int flag = 0;
			switch(save_tok) {
			case DATA: flag = GS_EQUAL; break;
			case BASENAMESTRING: flag = GS_BASENAMESTRING_EQUAL; break;
			case RESOURCEDATA: flag = GS_RESOURCEDATA_EQUAL; break;
			}
			ps->topicmapClass->lookupNodesByData(
				ps->transaction,
				flag,
				ps->text,
				ps->lastSet);
			break; }
		default:
			/* TBD: return error */
			;
		}
	
		getToken(ps);

		break; }
	case LCP:
		/* TBD if last Set delete set */
		ps->lastSet = GSNodeSet_new(ps->gw);	
		getToken(ps);
		while(ps->currentToken != RCP)
		{
			GSUri_T u;
			assert(ps->currentToken == URI || ps->currentToken == INDEX);

			if(ps->currentToken == URI)
			{
				if(ps->currentQuery->baseUri)
					u = GSUri_new(ps->gw,"Dh",ps->currentQuery->baseUri,ps->text);
				else
					u = GSUri_new(ps->gw,"a",ps->text);	
				/* lookup and insert in set */		
	
				{
				GSResource_T rrr;
				gw_topic topic;
				rrr = GSResource_new(ps->gw,"U",u);
				ps->topicmapClass->lookupNodeBySIR(ps->transaction,rrr,LOOKUP,&topic);
	/*
				idx = ps->topicmapClass->lookupSubjectIndexBySIRUri(ps->transaction,u,LOOKUP);
	*/
				if(topic != 0 /* TBD !!! */)
				{
					GSNodeSet_insert(ps->lastSet,topic);
				}
				else
				{
					/* TBD LOG this */
					printf("NO SUBJECT FOR URI FOUND\n"); 
				}
				GSResource_delete(&rrr);
				}
			
			}
			else if(ps->currentToken == INDEX)
			{
					GSNodeSet_insert(ps->lastSet,ps->index);
			}

			getToken(ps);	
			if(ps->currentToken == KOMMA)
				getToken(ps);

		}
		getToken(ps);
		break;
	default:
		assert(!"syntax error");
	}		
}

void doPath(struct parseState *ps)
{
	struct path *p,*newp;
	GSTRACE(GS_GRAPH_TRACE,"-doPath-\n");
	do { 
		switch(ps->currentToken) {
		case TRAVERSE:
			
			GS_NEW(ps->gw,newp);	
			newp->op = OP_NO;
			newp->next = NULL;
			if(ps->currentQuery->path == NULL) {
				ps->currentQuery->path = newp;
			} else {
				p = ps->currentQuery->path;
				while(p->next) p = p->next;
				assert(ps->currentQuery->currentPath == p);
				p->next = newp;
			}
			ps->currentQuery->currentPath = newp;

			getToken(ps);
			doTraverse(ps);
			getToken(ps);
			break;
		case MATCHSCOPE:
			GS_NEW(ps->gw,newp);	
			newp->op = OP_NO;
			newp->next = NULL;
			if(ps->currentQuery->path == NULL) {
				ps->currentQuery->path = newp;
			} else {
				p = ps->currentQuery->path;
				while(p->next) p = p->next;
				assert(ps->currentQuery->currentPath == p);
				p->next = newp;
			}
			ps->currentQuery->currentPath = newp;

			/* not yet, next is set ,   getToken(ps); */
			doMatchScope(ps);
			/* set at the end is a problem, set does lookahead, others do sometimes not... */
			break;
		default:
			assert(!"syntax error");
		}
	} while(ps->currentToken == TRAVERSE || ps->currentToken == MATCHSCOPE);
}
void doMatchScope(struct parseState *ps)
{
	GSTRACE(GS_GRAPH_TRACE,"-doMatchScope-\n");
	/* get the match Set */
	getToken(ps);
	doSet(ps);
	ps->currentQuery->currentPath->op = OP_MATCHSCOPE;
	ps->currentQuery->currentPath->arg = ps->lastSet;
}

void doTraverse(struct parseState *ps)
{
	GSTRACE(GS_GRAPH_TRACE,"-doTraverse-\n");
	switch(ps->currentToken) {
	case aAXx:
	case _AX_:
		ps->currentQuery->currentPath->op = OP_aAXx;
		break;
	case xAXa:
	case _XA_:
		ps->currentQuery->currentPath->op = OP_xAXa;
		break;
	case _AS_:
		ps->currentQuery->currentPath->op = OP_AS;
		break;
	case _SA_:
		ps->currentQuery->currentPath->op = OP_SA;
		break;
	case _SC_:
		ps->currentQuery->currentPath->op = OP_SC;
		break;
	case _CS_:
		ps->currentQuery->currentPath->op = OP_CS;
		break;
	
	case aAMm:
		ps->currentQuery->currentPath->op = OP_aAMm;
		getToken(ps);
		assert(ps->currentToken == LP);
		getToken(ps);
		doSet(ps);
		ps->currentQuery->currentPath->arg = ps->lastSet;
		assert(ps->currentToken == RP);
		/* getToken(ps); */
		break;
	case mAMa:
		ps->currentQuery->currentPath->op = OP_mAMa;
		getToken(ps);
		assert(ps->currentToken == LP);
		getToken(ps);
		doSet(ps);
		ps->currentQuery->currentPath->arg = ps->lastSet;
		assert(ps->currentToken == RP);
		/* getToken(ps); */
		break;
	case aAMr:
		ps->currentQuery->currentPath->op = OP_aAMr;
		getToken(ps);
		assert(ps->currentToken == LP);
		getToken(ps);
		doSet(ps);
		ps->currentQuery->currentPath->arg = ps->lastSet;
		assert(ps->currentToken == RP);
		/* getToken(ps); */
		break;
	case mAMr:
		ps->currentQuery->currentPath->op = OP_mAMr;
		getToken(ps);
		assert(ps->currentToken == LP);
		getToken(ps);
		doSet(ps);
		ps->currentQuery->currentPath->arg = ps->lastSet;
		assert(ps->currentToken == RP);
		/* getToken(ps); */
		break;
	case rAMa:
		ps->currentQuery->currentPath->op = OP_rAMa;
		getToken(ps);
		assert(ps->currentToken == LP);
		getToken(ps);
		doSet(ps);
		ps->currentQuery->currentPath->arg = ps->lastSet;
		assert(ps->currentToken == RP);
		/* getToken(ps); */
		break;
	case rAMm:
		ps->currentQuery->currentPath->op = OP_rAMm;
		getToken(ps);
		assert(ps->currentToken == LP);
		getToken(ps);
		doSet(ps);
		ps->currentQuery->currentPath->arg = ps->lastSet;
		assert(ps->currentToken == RP);
		/* getToken(ps); */
		break;
	default:
		 assert(!"syntax error");
	}
}

GSNodeSet_T evaluatePathExpression(struct parseState *ps,GSNodeSet_T seedSet, struct path *path)
{
	GSNodeSet_T currentSeedSet;
	struct path *p;
	
	assert(ps);
	assert(seedSet);
	assert(path);

	currentSeedSet = seedSet;
	for(p = path; p; p=p->next)
	{
		GSNodeSet_T tmp;
		struct yyyy y;
		GSNodeSet_T newSeedSet = GSNodeSet_new(ps->gw);
		GSTRACE(GS_GRAPH_TRACE,"===============================evaluatePathExpression: for %d\n" _ p->op);

		y.ps = ps;
		y.path = p;
		y.targetSet = newSeedSet;

		GSNodeSet_apply(currentSeedSet,collect, &y);
		tmp = currentSeedSet;
		currentSeedSet = newSeedSet;
		GSNodeSet_delete(&tmp);
	}

	return currentSeedSet;
}


GSNodeSet_T evalPath(struct parseState *ps, Idx_T idx, struct path *path)
{
	GSNodeSet_T result;
	/* Idx_T i; */
	assert(ps);
	assert(path);

	result = GSNodeSet_new(ps->gw);
	GSTRACE(GS_GRAPH_TRACE,">>>>>>>>>>>>>>>>>>>> evalPath for seed: %d op: %d\n" _ idx _ path->op);
	switch(path->op) {
	case OP_aAXx:
		ps->topicmapClass->traverseArc(ps->transaction,TRAV_AX,idx,result);
		break;
	case OP_xAXa:
		ps->topicmapClass->traverseArc(ps->transaction,TRAV_XA,idx,result);
		break;
	case OP_AS:
		ps->topicmapClass->traverseArc(ps->transaction,TRAV_AS,idx,result);
		break;
	case OP_SA:
		ps->topicmapClass->traverseArc(ps->transaction,TRAV_SA,idx,result);
		break;
	case OP_SC:
		ps->topicmapClass->traverseArc(ps->transaction,TRAV_SC,idx,result);
		break;
	case OP_CS:
		ps->topicmapClass->traverseArc(ps->transaction,TRAV_CS,idx,result);
		break;
	case OP_mAMa:
		ps->topicmapClass->doAM(ps->transaction,idx,0,(GSNodeSet_T)path->arg,result,DOmAMa);
		break;
	case OP_aAMm:
		ps->topicmapClass->doAM(ps->transaction,idx,0,(GSNodeSet_T)path->arg,result,DOaAMm);
		break;
	case OP_aAMr:
		ps->topicmapClass->doAM(ps->transaction,idx,0,(GSNodeSet_T)path->arg,result,DOaAMr);
		break;
	case OP_mAMr:
		ps->topicmapClass->doAM(ps->transaction,idx,0,(GSNodeSet_T)path->arg,result,DOmAMr);
		break;
	case OP_rAMa:
		ps->topicmapClass->doAM(ps->transaction,idx,0,(GSNodeSet_T)path->arg,result,DOrAMa);
		break;
	case OP_rAMm:
		ps->topicmapClass->doAM(ps->transaction,idx,0,(GSNodeSet_T)path->arg,result,DOrAMm);
		break;
	case OP_MATCHSCOPE: {
		gw_topic stopic;
		GSNodeSet_T stopics_of_checked_atopic;

		ps->topicmapClass->lookupSNodeByComponentNodeSet(
			ps->transaction,
			(GSNodeSet_T)path->arg,
			LOOKUP,
			&stopic
		);

		/* TBD just now, until we know it works */
		assert(stopic != 0);
		
		/* stopic is the scope that is represented by the comp set */

		stopics_of_checked_atopic = GSNodeSet_new(ps->transaction->gw);

		ps->topicmapClass->traverseArc(
			ps->transaction,
			TRAV_AS,
			idx,
			stopics_of_checked_atopic
		);

		/* store checked atopic to result if it is scoped by 'stopic' */
		if(GSNodeSet_hasElement(stopics_of_checked_atopic,stopic))
			GSNodeSet_insert(result,idx);
			
		GSNodeSet_delete(&stopics_of_checked_atopic);

		break; }
	default:
		assert(!"unknown op");
	}

	return result; 

}





void collect(Idx_T n, void *arg)
{
	struct yyyy *yp;
	GSNodeSet_T set;
	yp = (struct yyyy*)arg;

	set = evalPath(yp->ps,n,yp->path);
	GSNodeSet_eat(yp->targetSet,set);
	GSNodeSet_delete(&set);
}
#endif
/*
struct yyyy {
	struct parseState *ps;
	struct path *path;
	GSNodeSet_T targetSet;
};

void collect(Idx_T n, void *arg);
*/
void set_error(struct gs_parse_state *ps,const char *fmt,...)
{
	va_list args;
	assert(ps);
	assert(fmt);
	va_start(args,fmt);
	vsnprintf(ps->errstr,sizeof(ps->errstr),fmt,args);
	va_end(args);
}


const char* gs_parse_state_error(struct gs_parse_state *ps)
{
	assert(ps);
	return ps->errstr;
}



static void set_syntax_error(struct gs_parse_state *ps)
{
	assert(ps);
	set_error(ps,"sTMQL syntax error at offset %d near %s",
                        ps->ptr - ps->query, ps->ptr);
}
