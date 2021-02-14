#include "tmvaluetype.h"
#include "tmtrace.h"
#include "tmmem.h"
#include "tmlist.h"
#include "tmutil.h"
#include "tmassert.h"
#include "tmbasictypes.h"


static void* topic_new(TMValueType vtype);
static void* topic_new_from_string(TMValueType vtype, const char *s);
static int topic_equal(TMValueType, const void *, const void*);
static void topic_free(TMValueType vtype, void *s);
static int topic_to_string(TMValueType, const void *topic, char* buf, size_t size);
static int topic_to_xml(TMValueType, const void *topic, char* buf, size_t size);
static void *topic_parse_argstring(TMValueType vtype, int opcode, const char *argstring);

static struct op_table ops[] = {
	/*
	{ "=" , TM_OP_Text_EQUAL },
	{ "LIKE" , TM_OP_Text_LIKE },
	*/
	{ NULL, 0 }
};

struct TMValueType Topic = {
        "Topic",
	1,		/* values do not contain topics */
	topic_new,
	topic_new_from_string,
	topic_to_string,
	topic_to_xml,
	topic_equal,
	topic_free,
	NULL,
	ops,
	topic_parse_argstring
	
};

void* topic_new(TMValueType vtype)
{
	assert(0); /* FIXME does not make sense, but if NULLs allowd...?! */
	return NULL;
}
void* topic_new_from_string(TMValueType vtype, const char *s)
{
	TMTopic t;
	t = atoi(s);
	/* FIXME: other func */
	return (void*)t;
}



void topic_free(TMValueType vtype, void *t)
{
	; /* do nothing */
}

int topic_to_string(TMValueType vtype, const void *topic, char* buf, size_t size)
{
	return ( snprintf(buf,size,"%d",(TMTopic)topic) );	
}
int topic_to_xml(TMValueType vtype,const void *topic, char* buf, size_t size)
{
	int n;
	n = snprintf(buf,size,"    <topic>%d</topic>",(TMTopic)topic);	
	assert(n < size);
	return n;
}

int topic_equal(TMValueType vtype, const void *lhs, const void *rhs)
{
	assert(lhs);
	assert(rhs);
	return ( (TMTopic)lhs == (TMTopic)rhs);
}
void *topic_parse_argstring(TMValueType vtype, int opcode, const char *argstring)
{
	assert(0);
	/*
	switch(opcode) {
	case TM_OP_Text_EQUAL:
		return tm_strdup(argstring);
	case TM_OP_Text_LIKE:
		return tm_strdup(argstring);
	default:
		assert(0);
	}
	*/
	return NULL;
}


