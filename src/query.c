/*
 * $Id: stmqlhandler.c,v 1.13 2002/10/02 22:06:25 jan Exp $
 *
 * Copyright (c) 2002 Jan Algermissen
 * See the file "COPYING" for copying permission.
 *
 */

#include "query.h"
#include "stmql.h"
#include "assertion.h"
#include "lookup.h"
#include "stmql.h"

#include <mem.h>
#include <trace.h>

/* size of allocated result set, this needs to be changed to
   dynamic expansion and a per-thread buffer I think. */
#define SIZE 300000


struct GS_RESULT_T {
        int len;
        int tuple_size;
        int row_cnt;
        char ***tuples;
};

GSError_T gs_query(GS_GRAPH_T topicmap, const char *q,GS_RESULT_T *rpp)
{
	int i;
	int result_length = 0;
	int unique_role = 1;	/* assume unique to simplify loop code */
	int tuple_size;
	char len_buf[43], size_buf[43]; 
        GS_RESULT_T resp;
        char qbuf[GS_MAXLINE];
	char *buf;
        const char *s;
        char *t;
	struct gs_parse_state parse_state;
	const char *pairs[10];
	gs_topic_t *result_tuples;
	gs_assertion_cursor_t cursor;

	assert(topicmap);


 
        for(s = q,t = qbuf; *s; s++,t++)
        {
                if(*s == '\n' || *s == '\r' || *s == '\t')
                        *t = ' ';
                else
                        *t = *s;
                while(isspace(*t) && *(s+1) && isspace(*(s+1))  )
                        s++;
        }
        *t = '\0';

	buf = qbuf; 

	/*
	 * Initialize the parse state and parse the query.
	 */

	gs_parse_state_init(&parse_state,topicmap,buf);
	
	if(!gs_parse_query(&parse_state) )
	{
		resp = NULL;
		return GS_OK;



		fprintf(stderr,"STMQL error: %s\n",
			gs_parse_state_error(&parse_state) );
		/*
		e = gs_send_response_header(t,
			GSTP_STRING,GSTP_VERSION,GSTP_ERROR,
			gs_parse_state_error(&parse_state),NULL);
		assert(!e);
		assert(0);
		*/
		return GS_EXXX;
	}


	tuple_size =  parse_state.current_select_block->nselections;
	GSTRACE(GS_HANDLER_TRACE,"tuple size: %d\n" _ tuple_size); 
	result_tuples 
		= (gs_topic_t *)malloc(SIZE * tuple_size * sizeof(gs_topic_t));
	assert(result_tuples);

	/* testing if really unique role as we assume by initialisation */
	if(tuple_size > 1)
	{
		for(i=0; i < tuple_size-1; i++)
		{
			if(parse_state.current_select_block->selections[i].r_topic
			!= parse_state.current_select_block->selections[i+1].r_topic)
			{
				unique_role = 0;
				break;
			}
		}
	}



	/*
	gs_assertion_cursor_init2(topicmap,
		parse_state.current_select_block->p_topic,&cursor,
		&(parse_state.current_select_block->condition));
	*/
	if(parse_state.current_select_block->condition.type == COND_SINGLE)
	{
		gs_assertion_cursor_init(topicmap,
			parse_state.current_select_block->p_topic,&cursor,
			parse_state.current_select_block->condition.r_topic,
			parse_state.current_select_block->condition.x_topic);
	} else if(parse_state.current_select_block->condition.type == COND_SET) {
		gs_assertion_cursor_init3(topicmap,
			parse_state.current_select_block->p_topic,&cursor,
			parse_state.current_select_block->condition.r_topic,
			parse_state.current_select_block->condition.x_topicset);
	} else {
		assert(0);
	}
		

	i = 0;	
	while( gs_assertion_cursor_has_more(&cursor) )
	{
		struct assertion a;
		int k;
		int ofs = i*tuple_size;
		assert(ofs < SIZE-2);
		GSTRACE(GS_HANDLER_TRACE,"working on the %dth assertion (start is 0)\n" _ i);
		gs_assertion_cursor_next(&cursor,&a);
		
		/* append a tuple at the end */
		for(k = 0; k < tuple_size; k++)
		{
			int j;
			int found = 0;
			gs_topic_t role_to_select;
			role_to_select = parse_state.current_select_block->selections[k].r_topic;

			GSTRACE(GS_HANDLER_TRACE,"looking for player of %d\n" _ role_to_select);


			for(j = 0; j < a.nmember; j++)
			{
				GSTRACE(GS_HANDLER_TRACE,"assertion has player of %d : %d\n" _
					 a.members[j].r_topic _ a.members[j].x_topic);


				/* select role 0 means take ANODE !! */

				if(role_to_select == 0)
				{
					result_tuples[ofs+k] = a.a_topic;
					found = 1;
					break;
				}
				else if(a.members[j].r_topic == role_to_select)
				{
					found = 1;
					/*
					if(tuple_size > 1 || (! parse_state.current_select_block->distinct))
					{
					*/
					result_tuples[ofs+k] =
						a.members[j].x_topic;
					GSTRACE(GS_HANDLER_TRACE,
						"stored %d: %d \n" _
							role_to_select _
							a.members[j].x_topic);
					/*
					} else {
					
						int l;
						int f = 0;
						for(l = 0; l<i;l++)
						{
							if(result_tuples[l] ==
							 a.members[j].x_topic)
							{ f = 1; break; }
						}
						if(f) { i--; break; }
						result_tuples[ofs+k] =
							a.members[j].x_topic;
						GSTRACE(GS_HANDLER_TRACE,
							"stored %d: %d \n" _
								role_to_select _
								a.members[j].x_topic);

					}
					*/
					break;
				}

			}
			/* role must be in the assertion !! */
			if(!found)
			{
				GSTRACE(GS_ERR_TRACE,"role %d not found in assertion\n" _ role_to_select);
				/*	
				e = gs_send_response_header(t,
					GSTP_STRING,GSTP_VERSION,GSTP_ERROR,
					"role not found",NULL);
				assert(!e);
				*/
				assert(0);
				gs_assertion_cursor_cleanup(&cursor);
				free(result_tuples);
				gs_parse_state_cleanup(&parse_state);
				return GS_OK;
			}
			assert(found);
		}
		/* now tuple has been stored */
		if(parse_state.current_select_block->distinct && unique_role)
		{
					
			int l;
			for(l = 0; l<i;l++)
			{
				int my_ofs;
				my_ofs = l*tuple_size;
				if(memcmp(result_tuples+my_ofs,result_tuples+ofs,(tuple_size*sizeof(gs_topic_t)))==0)
				{
					i--;	/* delete tuple we just inserted */ 
					break;
				}
			}
		}
		
		i++;
	}	
	result_length = i;
/*
	gs_assertion_cursor_cleanup(&cursor);
*/


	sprintf(len_buf,"%d",result_length);
	sprintf(size_buf,"%d",parse_state.current_select_block->nselections);

	pairs[0] = "ResultLength";
	pairs[1] = len_buf;
	pairs[2] = "TupleSize";
	pairs[3] = size_buf;
	pairs[4] = NULL;
	pairs[5] = NULL;
	/*
	e = gs_send_response_header(t,GSTP_STRING,GSTP_VERSION,GSTP_OK,
								"Ok",pairs);
	if(e != GS_OK)
	{
		return e;

	}
	assert(!e);
	*/

	/* possibly apply order or group clause ! */

	resp = (GS_RESULT_T)malloc(sizeof(*resp));
        assert(resp);
 
        resp->len = result_length; 
        resp->tuple_size = tuple_size; 
 
        resp->tuples = (char ***)malloc(resp->len * sizeof(char*));



	for(i = 0; i < result_length; i++)
	{
		int j;
		int ofs = i*tuple_size;

		GSTRACE(GS_HANDLER_TRACE,"building result tuple no. %d\n" _ i);
		/* if assertion not matches condition, continue; */
		resp->tuples[i] = (char **)malloc(resp->tuple_size * sizeof(char*));

		for(j=0;j < tuple_size;j++)
		{
			char buf[GS_MAXDATA];
			gs_topic_t topic;
			int n;
			topic = result_tuples[ofs+j];
			switch(parse_state.current_select_block->selections[j].subject_repr_type) {
			case SUBJECT_REPR_TYPE_LOCATOR:	
				bzero(buf,sizeof(buf));
				gs_lookup_scr_locator(topicmap,topic,buf,sizeof(buf));
				n = strlen(buf); 
				break;
			case SUBJECT_REPR_TYPE_NODE:	
				n = snprintf(buf,sizeof(buf),"%d",topic);
				break;
			case SUBJECT_REPR_TYPE_DATA:	
				bzero(buf,sizeof(buf));
				gs_lookup_scr_data(topicmap,topic,buf,sizeof(buf));
				n = strlen(buf); 
				break;
			case SUBJECT_REPR_TYPE_SIR_LOCATOR:	
				bzero(buf,sizeof(buf));
				gs_lookup_sir_locator(topicmap,topic,buf,sizeof(buf));
				n = strlen(buf); 
				break;
			case SUBJECT_REPR_TYPE_SIR_DATA:	
				bzero(buf,sizeof(buf));
				gs_lookup_sir_data(topicmap,topic,buf,sizeof(buf));
				n = strlen(buf); 
				break;
			case SUBJECT_REPR_TYPE_ANY_BASENAME:	
				bzero(buf,sizeof(buf));
				gs_lookup_basename(topicmap,topic,buf,sizeof(buf));
				n = strlen(buf); 
				break;
			default:
				assert(0);
			}
			GSTRACE(GS_HANDLER_TRACE,"will now write field %s\n" _ buf);
			resp->tuples[i][j] = (char*)malloc(strlen(buf)+1);
                        strcpy(resp->tuples[i][j],buf);
		}
	}
        resp->row_cnt = 0;
        *rpp = resp;


	free(result_tuples);

	gs_parse_state_cleanup(&parse_state);
	return GS_OK;
}



 
void gs_result_cleanup(GS_RESULT_T res)
{
        int i,j;
	if(!res)
		return;
        for(i=0; i<res->len; i++)
                for(j=0; j<res->tuple_size; j++)
                        free(res->tuples[i][j]);
 
        free(res);
}
void gs_result_reset(GS_RESULT_T res)
{
	if(!res)
		return;
	res->row_cnt = 0;
}
int gs_result_length(GS_RESULT_T res)
{
	if(!res)
		return 0;
        assert(res);
        return res->len;
}
int gs_result_tuple_size(GS_RESULT_T res)
{
	if(!res)
		return 0;
        assert(res);
        return res->tuple_size;
}
GS_ROW_T gs_result_get_row(GS_RESULT_T res)
{
        int c;
	if(!res)
		return NULL;
        if(res->row_cnt >= res->len)
                return NULL;
        c = res->row_cnt++;
        return res->tuples[c];
}

