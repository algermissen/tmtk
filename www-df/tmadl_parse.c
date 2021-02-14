/*
 * $Id$
 *
 * Copyright (c) 2002 Jan Algermissen
 * See the file "COPYING" for copying permission.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#include <expat.h>

#include <tmtk.h>
#include <tmassert.h>
#include <tm.h>
#include <tmtrace.h>
#include <tmutil.h>
#include <tmmem.h>
#include <tmstack.h>
#include <tmmodel.h>
#include <tmdisclosureframework.h>

struct user_data {
	TM tm;
	TMDisclosureFramework df;
	TMModel model;
	TMStack elements;
	char text[4096];
	char error[1024];

	TMProperty property;
	TMAssertionType assertion_type;
	TMRoleType role;
	TMSubject subject;
};
static const char * _get_att_val(const char **attr, const char *name)
{
	int i;
	for (i = 0; attr[i]; i += 2) {
		if( strcmp(name,attr[i]) == 0 )
                {
                        return attr[i + 1];
                }
        }
	return NULL;
}

enum {
	UNKNOWN,
	TMA,
	REQUIRE,
	PROPERTY,
	NAME,
	DESCRIPTION,
	VALUETYPE,
	SIDP,
	COMBINATION,
	EQUALITY,
	ASSERTION_TYPE,
	ROLE,
	TOPIC,
	PROPVAL
};
static int _get_element_code(const char*e) {

	if(strcmp(e,"tma") == 0) {
		return TMA;

	} else if(strcmp(e,"require") == 0) {
		return REQUIRE;

	} else if(strcmp(e,"name") == 0) {
		return NAME;

	} else if(strcmp(e,"description") == 0) {
		return DESCRIPTION;

	} else if(strcmp(e,"property") == 0) {
		return PROPERTY;

	} else if(strcmp(e,"valueType") == 0) {
		return VALUETYPE;

	} else if(strcmp(e,"sidp") == 0) {
		return SIDP;
	} else if(strcmp(e,"combination") == 0) {
		return COMBINATION;

	} else if(strcmp(e,"equality") == 0) {
		return EQUALITY;
	} else if(strcmp(e,"assertionType") == 0) {
		return ASSERTION_TYPE;
	} else if(strcmp(e,"role") == 0) {
		return ROLE;
	} else if(strcmp(e,"topic") == 0) {
		return TOPIC;
	} else if(strcmp(e,"propval") == 0) {
		return PROPVAL;

	} else {
		return UNKNOWN;
	}
}


static void start(void *data, const char *el, const char **attr)
{
        struct user_data *dp = (struct user_data*)data;
	int code = _get_element_code(el);
	int last = UNKNOWN;

	if(*(dp->error))
		return;

	bzero(dp->text,sizeof(dp->text));

	TMTRACE(TM_PARSE_TRACE,"\n\nstart %s (%d)\n" _ el _ code);

	if(tm_stack_size(dp->elements) > 0)
	{
		last = (int)tm_stack_top(dp->elements);
	}
	TMTRACE(TM_PARSE_TRACE,"2start %s\n" _ el);

	if(code == TMA) {
		TM_NEW(dp->model);
		dp->model->properties = NULL;  /* FIXME: make init code in model.c */
		dp->model->atypes = NULL;
		dp->model->require = NULL;
		dp->model->subjects = NULL;

	} else if(code == PROPERTY) {
		TM_NEW(dp->property);
		dp->property->model = dp->model->name;
		dp->property->combination_code = NULL;
		dp->property->equality_code = NULL;

	} else if(code == ASSERTION_TYPE) {
		TM_NEW(dp->assertion_type);
		TM_NEW(dp->assertion_type->subject);
		dp->assertion_type->roles = NULL;
		dp->assertion_type->subject->N = 0;
	} else if(code == ROLE) {
		TM_NEW(dp->role);
		TM_NEW(dp->role->subject);
		dp->role->subject->N = 0;
	} else if(code == TOPIC) {
		TM_NEW(dp->subject);
		dp->subject->N = 0;

	} else if(code == PROPVAL && last == ASSERTION_TYPE) {
		int n = dp->assertion_type->subject->N;
		const char *model_name = tm_strdup(_get_att_val(attr,"model"));
		const char *prop_name = tm_strdup(_get_att_val(attr,"prop"));
		dp->assertion_type->subject->props[n].model = model_name;
		dp->assertion_type->subject->props[n].name = prop_name;
		/* value is set in edn element */
	} else if(code == PROPVAL && last == ROLE) {
		int n = dp->role->subject->N;
		const char *model_name = tm_strdup(_get_att_val(attr,"model"));
		const char *prop_name = tm_strdup(_get_att_val(attr,"prop"));
		dp->role->subject->props[n].model = model_name;
		dp->role->subject->props[n].name = prop_name;
		/* value is set in edn element */
	} else if(code == PROPVAL && last == TOPIC) {
		int n = dp->subject->N;
		const char *model_name = tm_strdup(_get_att_val(attr,"model"));
		const char *prop_name = tm_strdup(_get_att_val(attr,"prop"));
		dp->subject->props[n].model = model_name;
		dp->subject->props[n].name = prop_name;
		/* value is set in edn element */
	} else {
		
		;
	}
	tm_stack_push(dp->elements,(void*)code);
	TMTRACE(TM_PARSE_TRACE,"exit start %s\n" _ el);
}

static void end(void *data, const char *el)
{
	int code,c,last = UNKNOWN;
        struct user_data *dp = (struct user_data*)data;
	if(*(dp->error))
		return;
	code = _get_element_code(el);
	c = (int)tm_stack_pop(dp->elements);
	assert(c == code);

	
	if(tm_stack_size(dp->elements) > 0)
	{
		last = (int)tm_stack_top(dp->elements);
	}
	TMTRACE(TM_PARSE_TRACE,"end %s, last %d\n" _ el _ last);

	if(code == NAME && last == TMA) {
		dp->model->name = tm_strdup(dp->text);

	} else if(code == REQUIRE) {
		TMError e;
		TMModel model = NULL;
		char *model_name = dp->text;
		assert(model_name);

		if( (e = tm_disclosureframework_lookup_model(
                dp->df,dp->tm, model_name,&model)) != TM_OK)
		{
			snprintf(dp->error,sizeof(dp->error),"cannot load required model, %s",
				tm_get_error(dp->tm));
			return;
		}
		assert(model);
		dp->model->require = tm_list_push(dp->model->require, model);

	} else if(code == NAME && last == PROPERTY) {
		dp->property->name = tm_strdup(dp->text);
		/* +2 for the :: +1 for the \0 */
		dp->property->fullname = TM_ALLOC( strlen(dp->model->name)+strlen(dp->text)+2+1);
		sprintf((char*)dp->property->fullname,"%s::%s",dp->model->name, dp->text);

	} else if(code == NAME && last == ASSERTION_TYPE) {
		int n = dp->assertion_type->subject->N;
		dp->assertion_type->name = tm_strdup(dp->text);
		dp->assertion_type->subject->props[n].model = tm_strdup("http://www.gooseworks.org/disclosures/SAM.xml");
		dp->assertion_type->subject->props[n].name = tm_strdup("SubjectIndicators");
		dp->assertion_type->subject->props[n].value = tm_list_push(NULL,tm_strdup(dp->text));
		dp->assertion_type->subject->N++;

	} else if(code == NAME && last == ROLE) {
		int n = dp->role->subject->N;
		dp->role->name = tm_strdup(dp->text);
		dp->role->subject->props[n].model = tm_strdup("http://www.gooseworks.org/disclosures/SAM.xml");
		dp->role->subject->props[n].name = tm_strdup("SubjectIndicators");
		dp->role->subject->props[n].value = tm_list_push(NULL,tm_strdup(dp->text));
		dp->role->subject->N++;

	} else if(code == PROPVAL && last == ASSERTION_TYPE) {
		TMProperty prop;
		int n = dp->assertion_type->subject->N;
		TMTRACE(TM_PARSE_TRACE,"n=%d\n" _ n);
		prop = tm_model_get_property(dp->model,dp->assertion_type->subject->props[n].model,dp->assertion_type->subject->props[n].name);
		assert(prop);
		TMTRACE(TM_PARSE_TRACE,"MARK A\n" );
		/* model and prop names have been set in start-elemenet */
		TMTRACE(TM_PARSE_TRACE,"property: %s\n" _ prop->name);
		dp->assertion_type->subject->props[n].value =
			tm_value_new_from_string(prop->value_type,dp->text);
		dp->assertion_type->subject->N++;

	} else if(code == PROPVAL && last == ROLE) {
		TMProperty prop;
		int n = dp->role->subject->N;
		TMTRACE(TM_PARSE_TRACE,"n=%d\n" _ n);
		prop = tm_model_get_property(dp->model,dp->role->subject->props[n].model,dp->role->subject->props[n].name);
		assert(prop);
		TMTRACE(TM_PARSE_TRACE,"MARK A\n" );
		/* model and prop names have been set in start-elemenet */
		TMTRACE(TM_PARSE_TRACE,"property: %s\n" _ prop->name);
		dp->role->subject->props[n].value =
			tm_value_new_from_string(prop->value_type,dp->text);
		dp->role->subject->N++;
	} else if(code == PROPVAL && last == TOPIC) {
		TMProperty prop;
		int n = dp->subject->N;
		TMTRACE(TM_PARSE_TRACE,"n=%d\n" _ n);
		prop = tm_model_get_property(dp->model,dp->subject->props[n].model,dp->subject->props[n].name);
		assert(prop);
		TMTRACE(TM_PARSE_TRACE,"MARK A\n" );
		/* model and prop names have been set in start-elemenet */
		TMTRACE(TM_PARSE_TRACE,"property: %s\n" _ prop->name);
		dp->subject->props[n].value =
			tm_value_new_from_string(prop->value_type,dp->text);
		dp->subject->N++;

	/* ------------------------------------------------- */

	} else if(code == DESCRIPTION && last == TMA) {
		dp->model->description = tm_strdup(dp->text);

	} else if(code == DESCRIPTION && last == PROPERTY) {
		dp->property->description = tm_strdup(dp->text);

	} else if(code == DESCRIPTION && last == ASSERTION_TYPE) {
		dp->assertion_type->description = tm_strdup(dp->text);

	} else if(code == DESCRIPTION && last == ROLE) {
		dp->role->description = tm_strdup(dp->text);

	/* ------------------------------------------------- */

	} else if(code == COMBINATION && last == PROPERTY) {
		dp->property->combination_code = tm_strdup(dp->text);
	} else if(code == EQUALITY && last == PROPERTY) {
		dp->property->equality_code = tm_strdup(dp->text);
	} else if(code == VALUETYPE && last == PROPERTY) {
		dp->property->value_type = tm_lookup_valuetype(dp->tm,dp->text);
		assert(dp->property->value_type);
	} else if(code == SIDP && last == PROPERTY) {
		dp->property->type = (strcmp(dp->text,"yes") == 0) ? TM_SIDP : TM_OP;

	/* ------------------------------------------------- */

	} else if(code == PROPERTY) {
		dp->model->properties = tm_list_push(dp->model->properties,dp->property);
		dp->property = NULL;

	} else if(code == ROLE) {
		dp->assertion_type->roles = tm_list_push(dp->assertion_type->roles,dp->role);
		dp->role = NULL;

	} else if(code == ASSERTION_TYPE) {
		dp->model->atypes = tm_list_push(dp->model->atypes,dp->assertion_type);
		dp->assertion_type = NULL;

	} else if(code == TOPIC) {
		dp->model->subjects = tm_list_push(dp->model->subjects,dp->subject);
		dp->subject = NULL;

	} else {
		;
	}
	bzero(dp->text,sizeof(dp->text));
}
static void char_data(void *udata, const char *data, int len)
{
        struct user_data *dp = (struct user_data*)udata;
	int last;
	assert(tm_stack_size(dp->elements) > 0);


	last = (int)tm_stack_top(dp->elements);
	TMTRACE(TM_PARSE_TRACE,"enter char_data, lastcode=%d\n" _ last);
	if(last == SIDP
	|| last == COMBINATION
	|| last == EQUALITY
	|| last == NAME
	|| last == DESCRIPTION
	|| last == REQUIRE
	|| last == PROPVAL
	|| last == VALUETYPE )
	{
		TMTRACE(TM_PARSE_TRACE,"CHARDATA: last: %d\n" _ last);
		assert(len < (sizeof(dp->text) - strlen(dp->text)) );
		/*
		bzero(dp->text,sizeof(dp->text));
		*/
		strncat(dp->text,data,len);
		TMTRACE(TM_PARSE_TRACE,"==== %s ====\n" _ dp->text);
	}
	TMTRACE(TM_PARSE_TRACE,"exit char_data\n");

}

TMModel tm_create_tma_from_file(TMDisclosureFramework df,TM tm,FILE *file)
{
	struct user_data data;
        XML_Parser p;
	char buf[4096];
	int done;

	assert(file);

	bzero(&data,sizeof(data));
	data.tm = tm;
	data.df = df;
	data.elements = tm_stack_new(0);

        if( (p = XML_ParserCreate(NULL) ) == NULL)
        {
		assert(0); /* FIXME */
                return NULL;
        }
	XML_SetParamEntityParsing(p, XML_PARAM_ENTITY_PARSING_ALWAYS);
 
        XML_SetUserData(p, (void *) &data );
        XML_SetElementHandler(p, start, end);
        XML_SetCharacterDataHandler(p, char_data);
	do {
		size_t len = fread(buf, 1, sizeof(buf), file);
		done = len < sizeof(buf);
		if (!XML_Parse(p, buf, len, done)) {
			TMTRACE(TM_PARSE_TRACE,
				"%s at line %d\n" _
				XML_ErrorString(XML_GetErrorCode(p)) _
				XML_GetCurrentLineNumber(p));
			tm_set_error(tm,"%s at line %d\n" _
				XML_ErrorString(XML_GetErrorCode(p)) _
				XML_GetCurrentLineNumber(p));
				return NULL;
		}
		if(*(data.error))
		{
			tm_set_error(tm,"TMADL parse error: %s", data.error);
			return NULL;

		}
	} while (!done);
	/* 
	for (;;)
        {
                char buf[4096];
                int len;
 
                len = fread( buf, 1, sizeof(buf), file);
                if (len < 0) {
			tm_set_error(tm,"read error, %s", strerror(errno));
			return NULL;
                }
 
                if (! XML_Parse(p, buf, len, len < 4096))
                {
			FIXME: set error return NULL if we use this loop at all in the future....
                        TMTRACE(TM_PARSE_TRACE, "Parse error at line %d \n" _
                                XML_GetCurrentLineNumber(p));
                        exit(-1);
                }
                if (len < 4096)
                        break;
        }
	*/
	tm_stack_delete(&(data.elements));
	return data.model;
}
