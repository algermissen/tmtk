/*
 * $Id$
 *
 * Copyright (c) 2002 Jan Algermissen
 * See the file "COPYING" for copying permission.
 *
 */

#ifndef TM_VALUETYPE_INCLUDED
#define TM_VALUETYPE_INCLUDED

#include "tmtk.h" 

#ifdef __cplusplus
extern "C" {
#endif

/** \defgroup TMValueType
 * @{
 */


/** Value type structure.
 *
 */
typedef struct TMValueType *TMValueType;

/* is expected to return the number of bytes that would have been written (including \0)
 * This * is the behaviour of snprintf according to the C99 standard, see 'man snprintf'
 * FIXME: solaris etc??
 */
typedef int(*ValueToStrFunction)(TMValueType,const void*,char *,size_t);
typedef int(*ValueEqualFunction)(TMValueType,const void*,const void*);
typedef void(*ValueDeleteFunction)(TMValueType, void*);
typedef void*(*ValueNewFunction)(TMValueType);
typedef void*(*ValueNewFromStringFunction)(TMValueType, const char*);
typedef void*(*ValueCallFunction)(TMValueType,void*,va_list args);
typedef void*(*ValueTypeParseAgrstringFunction)(TMValueType vtype, int opcode, const char *argstring);

struct function_table {
	const char *name;
	ValueCallFunction function;
};

struct op_table {
	const char *opstring;
	int opcode;
};


/*
typedef int(*ValueTypeValueCombinationFunction)(TMValueType,const void*,const void*);
*/



struct TMValueType {
	const char *name;
	int values_contain_topics;
	ValueNewFunction new;
	ValueNewFromStringFunction new_from_string;
	ValueToStrFunction to_string;
	ValueToStrFunction to_xml;
	ValueEqualFunction equal;
	ValueDeleteFunction delete;
	struct function_table *ftab;
	struct op_table *optab;
	ValueTypeParseAgrstringFunction parse_argstring;
};

/** Get operator code from operator string.
 */
TM_API(int) tm_valuetype_parse_opstring(TMValueType vtype, const char *opstring);

/** Do values of this type contain topics?
 */
TM_API(int) tm_valuetype_values_contain_topics(TMValueType vtype);


/** Test values for equality.
 */
TM_API(int) tm_value_equal(TMValueType vtype, const void *lhs, const void *rhs);

/** Get string representation of value.
 */
TM_API(int) tm_value_to_string(TMValueType vtype, const void *v,char* buf, size_t size );

/** Get XML representation of value.
 */
TM_API(int) tm_value_to_xml(TMValueType vtype, const void *v,char* buf, size_t size );


/** Delete a value.
 */
TM_API(void) tm_value_delete(TMValueType vtype, void** vp );
/** Create a new value.
 */
TM_API(void*) tm_value_new(TMValueType vtype );

TM_API(void*) tm_value_new_from_string(TMValueType vtype , const char *);

TM_API(void*) tm_value_call(TMValueType,void*,const char *,...);

TM_API(void*) tm_valuetype_parse_argstring(TMValueType vtype, int opcode, const char *argstring);

#include "tmtrace.h"
#define TMTRACE_VALUE(flag,vtype,value) do { \
        char buf[TM_VALUE_TRACE_BUFSIZE]; \
        tm_value_to_string( (vtype), (value), buf,sizeof(buf)); \
        TMTRACE((flag),"value: %s\n" _ buf); \
        } while(0)



/**
 * @}
 */

#endif
