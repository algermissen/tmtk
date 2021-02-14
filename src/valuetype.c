#include "tmvaluetype.h"
#include "tmassert.h"
#include "tmtrace.h"

int tm_valuetype_parse_opstring(TMValueType vtype, const char *opstring)
{
	struct op_table *pp;

	TMTRACE(TM_VALUETYPE_TRACE,"enter [vtype=%s, op=%s]\n" _ vtype->name _ opstring);

	for(pp = vtype->optab;  pp &&  (pp)->opstring; pp++)
	{
		struct op_table *ot = pp;
		if(strcmp(ot->opstring,opstring) == 0)
		{
			return (ot->opcode);
		}
	}
	TMTRACE(TM_VALUETYPE_TRACE,"exit\n");
	return (0);
}

int tm_valuetype_values_contain_topics(TMValueType vtype)
{
	return(vtype->values_contain_topics);
}
void *tm_valuetype_parse_argstring(TMValueType vtype, int opcode, const char *argstring)
{

        return( vtype->parse_argstring(vtype,opcode,argstring)  );
}

int tm_value_equal(TMValueType vtype, const void *lhs, const void *rhs)
{
        return( vtype->equal(vtype,lhs,rhs)  );
}

int tm_value_to_string(TMValueType vtype, const void *v, char* buf, size_t size )
{
        return( vtype->to_string(vtype,v,buf,size)  );
}
int tm_value_to_xml(TMValueType vtype, const void *v,char* buf, size_t size )
{
        return( vtype->to_xml(vtype,v,buf,size)  );
}

void tm_value_delete(TMValueType vtype, void **vp )
{
	void *v = *vp;
	TMTRACE(TM_VALUETYPE_TRACE,"enter\n");
        vtype->delete(vtype,v);
	TMTRACE(TM_VALUETYPE_TRACE,"mid\n");
	*vp = NULL;
	TMTRACE(TM_VALUETYPE_TRACE,"exit\n");
}

void *tm_value_new(TMValueType vtype)
{
        return (vtype->new(vtype));
}
void *tm_value_new_from_string(TMValueType vtype, const char *s)
{
        return (vtype->new_from_string(vtype,s));
}
/*
typedef void*(*ValueCallFunction)(TMValueType,void*,const char*,va_list args);

struct function_table {
	        const char *name;
		        ValueCallFunction function;
};
*/


void *tm_value_call(TMValueType vtype, void *v,const char *name,...)
{
	va_list args;
	struct function_table *ftpp;
	void *rv;

	TMTRACE(TM_VALUETYPE_TRACE,"enter [vtype=%s, function=%s]\n" _ vtype->name _ name);

	va_start(args,name);
	for(ftpp = vtype->ftab;  ftpp &&  (ftpp)->name; ftpp++)
	{
		struct function_table *ftp = ftpp;
		if(strcmp(ftp->name,name) == 0)
		{
			TMTRACE(TM_VALUETYPE_TRACE,"found function %s\n" _ ftp->name);

			rv = ftp->function(vtype,v,args);
			va_end(args);
			TMTRACE(TM_VALUETYPE_TRACE,"exit\n");
			return rv;
		}
	}
	TMTRACE(TM_VALUETYPE_TRACE,"not found function %s\n" _ name);
	va_end(args);
	/* FIXME: trace */
	assert(!"function not found");
	TMTRACE(TM_VALUETYPE_TRACE,"exit\n");
	return NULL;
}
