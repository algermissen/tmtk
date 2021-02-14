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

#include "tmutil.h"
#include "tmprocmodel.h"
#include "tmmodel.h"
#include "tmtrace.h"
#include "tmassert.h"
#if 0
TMProcModel tm_model_get_procmodel(TMModel self, const char *root_name)
{
	TMProcModel *ppm;
	/*
	TMModel *pm;
	*/
	assert(self);
	assert(root_name);

	for( ppm = self->proc_models; ppm && *ppm; ppm++)
	{
		if(strcmp((*ppm)->root_element_name,root_name) == 0)
			return *ppm;
	}
	/* ------- it's better not to serach required models!
	for( pm = self->required_models; pm && *pm; pm++)
	{
		TMProcModel p;
		if( (p =  tm_model_get_procmodel(*pm,root_name)) != NULL)
			return p;
	}
	---- */
	
	return NULL;
}
#endif


int tm_procmodel_is_id_attribute(TMProcModel model, const char* attr)
{
	if(model->id_attribute_name && strcmp(model->id_attribute_name,attr) == 0)
		return 1;
	else
		return 0;
}
int tm_procmodel_is_ref_attribute(TMProcModel model, const char* attr)
{
	const char *p = attr;

	if( (p = strchr(attr,NS_DELIM)) != NULL)
	{
		attr = p+1;
	}
	else if( (p = strchr(attr,':')) != NULL)
	{
		attr = p+1;
	}


	if(model->ref_attribute_name && strcmp(model->ref_attribute_name,attr) == 0)
	{
		return 1;
	}
	else
	{
		return 0;
	}
}
int tm_procmodel_ignore_element(TMProcModel model, const char* elem)
{
	TMList lp;
	assert(model);
	assert(elem);
	for(lp = model->ignore_elements; lp; lp=lp->next)
	{
		if(strcmp(lp->content,elem) == 0)
			return 1;
	}
	return 0;
}


#if 0
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


static void start(void *data, const char *el, const char **attr)
{
        TMProcModel model = (TMProcModel)data;

	fprintf(stderr,"start element %s\n",el);
	if(strcmp(el,"idAttribute") == 0) {
		assert(_get_att_val(attr,"name"));
		strncpy(model->id_attribute_name,_get_att_val(attr,"name"),
			sizeof(model->id_attribute_name));
		fprintf(stderr,"id attribute now %s\n",model->id_attribute_name);
	} else if(strcmp(el,"refAttribute") == 0) {
		assert(_get_att_val(attr,"name"));
		strncpy(model->ref_attribute_name,_get_att_val(attr,"name"),
			sizeof(model->ref_attribute_name));
		fprintf(stderr,"ref attribute now %s\n",model->ref_attribute_name);
	} else if(strcmp(el,"ignoreElement") == 0) {
		assert(_get_att_val(attr,"name"));
		model->ignore_elements = TMList_push(model->ignore_elements,
			gs_strdup(_get_att_val(attr,"name")) );
	} else if(strcmp(el,"target") == 0) {
		target *t;
		assert(_get_att_val(attr,"match"));
		assert(_get_att_val(attr,"event"));
		t = (target*)malloc(sizeof(struct target));
		assert(t);
		t->event = START_EVENT;
		t->match = gs_strdup(_get_att_val(attr,"match"));
		assert(model->last_target == NULL);
		model->last_target = t;
	} else if(strcmp(el,"property") == 0) {

		target *t;
		assert(_get_att_val(attr,"match"));
		assert(_get_att_val(attr,"event"));
		t = (target*)malloc(sizeof(struct target));
		assert(t);
		t->event = START_EVENT;
		t->match = gs_strdup(_get_att_val(attr,"match"));
		assert(model->last_target == NULL);
		model->last_target = t;

	} else {
		;
	}
}

static void end(void *data, const char *el)
{
        TMProcModel model = (TMProcModel)data;
	fprintf(stderr,"end element %s\n",el);
	if(strcmp(el,"idAttribute") == 0) {
		;
	} else if(strcmp(el,"ignoreElement") == 0) {
		;
	} else if(strcmp(el,"target") == 0) {
		fprintf(stderr,"TMList_push\n");	
		model->targets = TMList_push(model->targets,model->last_target);
		model->last_target = NULL;
	} else {
		;
	}
}
static void char_data(void *user_data, const char *data, int len)
{
	/*
        Model model = (Model)data;
	*/
}

TMProcModel model_create(const char* filename)
{
	TMProcModel model;
        XML_Parser p;
	FILE *file;

	model = (TMProcModel)malloc(sizeof(*model));
	assert(model);
	assert(filename);

	model->elements = NULL;
	model->ignore_elements = NULL;
	model->targets = NULL;
	model->last_target = NULL;

	file = fopen(filename,"r");
	assert(file);
 
        if( (p = XML_ParserCreate(NULL) ) == NULL)
        {
                return NULL;
        }
	XML_SetParamEntityParsing(p, XML_PARAM_ENTITY_PARSING_ALWAYS);
 
        XML_SetUserData(p, (void *) model);
        XML_SetElementHandler(p, start, end);
        XML_SetCharacterDataHandler(p, char_data);
 
	for (;;)
        {
                char buf[1024];
                int len;
 
                len = fread( buf, 1, sizeof(buf), file);
                if (len < 0) {
                        perror("model_create - read error");
                        exit(1);
                }
 
                if (! XML_Parse(p, buf, len, len == 0))
                {
                        fprintf(stderr, "Parse error at line %d in %s\n",
                                XML_GetCurrentLineNumber(p), filename);
                        exit(-1);
                }
                if (len == 0)
                        break;
        }
	return model;
}
#endif
