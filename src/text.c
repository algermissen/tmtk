#include "tmvaluetype.h"
#include "tmtrace.h"
#include "tmmem.h"
#include "tmlist.h"
#include "tmutil.h"
#include "tmassert.h"
#include "tmbasictypes.h"


static void* text_new(TMValueType vtype);
static void* text_new_from_string(TMValueType vtype, const char *s);
static int text_equal(TMValueType, const void *, const void*);
static void text_free(TMValueType vtype, void *s);
static int text_to_string(TMValueType, const void *text, char* buf, size_t size);
static int text_to_xml(TMValueType, const void *text, char* buf, size_t size);
static void *text_parse_argstring(TMValueType vtype, int opcode, const char *argstring);

static struct op_table ops[] = {
	{ "=" , TM_OP_Text_EQUAL },
	{ "LIKE" , TM_OP_Text_LIKE },
	{ NULL, 0 }
};

struct TMValueType Text = {
        "Text",
	0,		/* values do not contain topics */
	text_new,
	text_new_from_string,
	text_to_string,
	text_to_xml,
	text_equal,
	text_free,
	NULL,
	ops,
	text_parse_argstring
	
};

void* text_new(TMValueType vtype)
{
	assert(0); /* FIXME does not make sense, but if NULLs allowd...?! */
	return NULL;
}
void* text_new_from_string(TMValueType vtype, const char *s)
{
	return tm_strdup(s);
}



void text_free(TMValueType vtype, void *s)
{
	assert(s); /* FIXME: allow NULLs?? */
	TM_FREE(s);
}

int text_to_string(TMValueType vtype, const void *text, char* buf, size_t size)
{
	return ( snprintf(buf,size,"%s", text ? (char*)text : "") );	
}
int text_to_xml(TMValueType vtype,const void *text, char* buf, size_t size)
{
	int n;
	n = snprintf(buf,size,"    <text>%s</text>",(char*)text);	
	assert(n < size);
	return n;
}

int text_equal(TMValueType vtype, const void *lhs, const void *rhs)
{
	assert(lhs);
	assert(rhs);
	return ( strcmp((const char*)lhs,(const char *)rhs) == 0);
}
void *text_parse_argstring(TMValueType vtype, int opcode, const char *argstring)
{
	switch(opcode) {
	case TM_OP_Text_EQUAL:
		return tm_strdup(argstring);
	case TM_OP_Text_LIKE:
		return tm_strdup(argstring);
	default:
		assert(0);
	}
	return NULL;
}


