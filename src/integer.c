#include "tmvaluetype.h"
#include "tmtrace.h"
#include "tmmem.h"
#include "tmlist.h"
#include "tmutil.h"
#include "tmassert.h"
#include "tmbasictypes.h"


static void* integer_new(TMValueType vtype);
static void* integer_new_from_string(TMValueType vtype, const char *s);
static int integer_equal(TMValueType, const void *, const void*);
static void integer_free(TMValueType vtype, void *s);
static int integer_to_string(TMValueType, const void *topic, char* buf, size_t size);
static int integer_to_xml(TMValueType, const void *topic, char* buf, size_t size);
static void *integer_parse_argstring(TMValueType vtype, int opcode, const char *argstring);

static struct op_table ops[] = {
	{ "=" , TM_OP_Integer_EQUAL },
	{ "<" , TM_OP_Integer_LT },
	{ ">" , TM_OP_Integer_GT },
	{ "<=" , TM_OP_Integer_LT_OR_EQUAL },
	{ ">=" , TM_OP_Integer_GT_OR_EQUAL },
	{ NULL, 0 }
};

struct TMValueType Integer = {
        "Integer",
	0,		/* values do not contain topics */
	integer_new,
	integer_new_from_string,
	integer_to_string,
	integer_to_xml,
	integer_equal,
	integer_free,
	NULL,
	ops,
	integer_parse_argstring
	
};

void* integer_new(TMValueType vtype)
{
	assert(0); /* FIXME does not make sense, but if NULLs allowd...?! */
	return NULL;
}
void* integer_new_from_string(TMValueType vtype, const char *s)
{
	int i;
	i = atoi(s);
	/* FIXME: other func */
	return (void*)i;
}



void integer_free(TMValueType vtype, void *t)
{
	; /* do nothing */
}

int integer_to_string(TMValueType vtype, const void *v, char* buf, size_t size)
{
	return ( snprintf(buf,size,"%d",(int)v) );	
}
int integer_to_xml(TMValueType vtype,const void *v, char* buf, size_t size)
{
	int n;
	n = snprintf(buf,size,"    <integer>%d</integer>",(int)v);	
	assert(n < size);
	return n;
}

int integer_equal(TMValueType vtype, const void *lhs, const void *rhs)
{
	assert(lhs);
	assert(rhs);
	return ( (int)lhs == (int)rhs);
}
void *integer_parse_argstring(TMValueType vtype, int opcode, const char *argstring)
{
	switch(opcode) {
	case TM_OP_Integer_EQUAL:
	case TM_OP_Integer_LT:
	case TM_OP_Integer_GT:
	case TM_OP_Integer_LT_OR_EQUAL:
	case TM_OP_Integer_GT_OR_EQUAL:
		return (void*)atoi(argstring);
	default:
		assert(0);
	}
	return NULL;
}


