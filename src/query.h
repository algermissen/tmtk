/* * $Id: stmqlhandler.h,v 1.2 2002/07/31 07:59:08 jan Exp $
 *
 * Copyright (c) 2002 Jan Algermissen
 * See the file "COPYING" for copying permission.
 *
 */

#ifndef TM_QUERY_H
#define TM_QUERY_H

#include <tm.h> 
#include <tmtopicmap.h>

typedef struct TM_T* TM_T;
typedef struct TM_RESULT_T* TM_RESULT_T;
typedef char** TM_ROW_T;


TM_API(TMError_T) gs_query(TM_GRAPH_T topicmap ,const char *q,TM_RESULT_T *rp);


TM_API(void) gs_result_cleanup(TM_RESULT_T res);
TM_API(void) gs_result_reset(TM_RESULT_T res);
TM_API(int) gs_result_length(TM_RESULT_T res);
TM_API(int) gs_result_tuple_size(TM_RESULT_T res);
TM_API(TM_ROW_T) gs_result_get_row(TM_RESULT_T res);



#endif
