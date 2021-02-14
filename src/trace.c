/*
 * $Id: util.c,v 1.8 2002/10/12 19:05:57 jan Exp $
 *
 * Copyright (c) 2002 Jan Algermissen
 * See the file "COPYING" for copying permission.
 *
 */
#include "tmtrace.h"
#include <stdio.h>	/* for printfs */

int tm_trace(const char *func, const char * fmt, ...)
{
	va_list args;
 
       	va_start(args,fmt);
	if(func)
       		fprintf(stderr,"%33s: ",func);
       	vfprintf(stderr,fmt,args);
 
       	va_end(args);
 
       	return 0;
}
/*
int gs_trace_str(const char *s, int len)
{
	fprintf(stderr,"GSTRACE_STR: _");
	while(len-- > 0)
		fprintf(stderr,"%c",*s++);
	fprintf(stderr,"_\n");
	return 0;
}
*/


int tm_set_trace_mask(const char * shortnames)
{
#if !defined(NDEBUG) && defined(TM_TRACEFLAG)
        TM_TRACEFLAG = 0;
        if (shortnames && *shortnames)
        {
                char * ptr = (char *) shortnames;
                for(; *ptr; ptr++) {
                        switch (*ptr) {
                        case 'g': TM_TRACEFLAG |= TM_SHOW_GRAPH_TRACE; break;
                        case 'o': TM_TRACEFLAG |= TM_SHOW_OMNIVORE_TRACE; break;
                        case 's': TM_TRACEFLAG |= TM_SHOW_STORAGE_TRACE; break;
                        case 'p': TM_TRACEFLAG |= TM_SHOW_PROC_TRACE; break;
                        case 'v': TM_TRACEFLAG |= TM_SHOW_VALUETYPE_TRACE; break;
                        case 'm': TM_TRACEFLAG |= TM_SHOW_MODEL_TRACE; break;
                        case 'P': TM_TRACEFLAG |= TM_SHOW_PARSE_TRACE; break;
                        case 'q': TM_TRACEFLAG |= TM_SHOW_QUERY_TRACE; break;
			/*
                        case 'l': TM_TRACEFLAG |= TM_SHOW_LPTPAGE_TRACE; break;
                        case 'H': TM_TRACEFLAG |= TM_SHOW_HANDLER_TRACE; break;
                        case 'u': TM_TRACEFLAG |= TM_SHOW_LOOKUP_TRACE; break;
                        case 'L': TM_TRACEFLAG |= TM_SHOW_LOG_TRACE; break;
                        case 'E': TM_TRACEFLAG |= TM_SHOW_ERR_TRACE; break;
                        case 'h': TM_TRACEFLAG |= TM_SHOW_HTTP_TRACE; break;
                        case 'P': TM_TRACEFLAG |= TM_SHOW_PAGE_TRACE; break;
                        case 'X': TM_TRACEFLAG |= TM_SHOW_X_TRACE; break;
                        case 'O': TM_TRACEFLAG |= TM_SHOW_OMNIVORE_TRACE; break;
			*/
                        case 'a': TM_TRACEFLAG |= TM_SHOW_APP_TRACE; break;
                        case '*': TM_TRACEFLAG |= TM_SHOW_ALL_TRACE; break;
                        default:
				;
                                /* bad argument */
                        }
                }
                if (!TM_TRACEFLAG)
                        TM_TRACEFLAG = TM_SHOW_ALL_TRACE;
        }
	else
        {
                TM_TRACEFLAG = TM_SHOW_ALL_TRACE;
        }
        return TM_TRACEFLAG;
#else
        return 0;
#endif
}

