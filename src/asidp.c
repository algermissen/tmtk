#include "tmvaluetype.h"
#include "tmtrace.h"
#include "tmmem.h"
#include "tmlist.h"
#include "tmtopicset.h"
#include "tmassert.h"

#include "tmbasictypes.h"


static int asidp_equal(TMValueType, const void *, const void*);
static void asidp_free(TMValueType vtype, void *s);
static void* asidp_new(TMValueType vtype);
static void* asidp_new_from_string(TMValueType vtype,const char *s);
static int asidp_to_string(TMValueType, const void *asidp, char* buf, size_t size);
static int asidp_to_xml(TMValueType, const void *asidp, char* buf, size_t size);
static void* asidp_set_type(TMValueType, void *asidp,va_list args);
static void* asidp_add_member(TMValueType, void *asidp,va_list args);
static void* asidp_get_player(TMValueType, void *asidp,va_list args);
static void* asidp_replace_topics(TMValueType, void *asidp,va_list args);
static void *asidp_parse_argstring(TMValueType vtype, int opcode, const char *argstring);

static struct function_table ftab[] = {
	{ "setType" , asidp_set_type },
	{ "addMember" , asidp_add_member },
	{ "getPlayer" , asidp_get_player },
	{ "replaceTopics" , asidp_replace_topics },
	{ NULL , NULL}
};
static struct op_table ops[] = {
        { "HAS_TYPE" , TM_OP_ASIDP_HAS_TYPE },
        { "HAS_PLAYER_PAIR" , TM_OP_ASIDP_HAS_PLAYER_PAIR },
        { "VALUE_INCLUDES_TOPIC" , TM_OP_ASIDP_VALUE_INCLUDES_TOPICS },
        { NULL, 0 }
};

struct TMValueType ASIDP = {
        "ASIDP",
	1,		/* values contain topics */
	asidp_new,
	asidp_new_from_string,
	asidp_to_string,
	asidp_to_xml,
	asidp_equal,
	asidp_free,
	ftab,
	ops,
	asidp_parse_argstring

};

void* asidp_new(TMValueType vtype)
{
	struct asidp_value *p;
	TM_NEW(p);
	p->type = 0;
	p->memberships = NULL;
	return p;
}
void* asidp_new_from_string(TMValueType vtype, const char *s)
{
	assert(0);
	/*
	struct asidp_value *p;
	TM_NEW(p);
	p->type = 0;
	p->memberships = NULL;
	return p;
	*/
	return NULL;
}

void asidp_free(TMValueType vtype, void *v)
{
	TMList lp;
	struct asidp_value *p;
	p = (struct asidp_value*)v;
	for(lp=p->memberships; lp; lp=lp->next)
	{
		struct a_membership *amp = (struct a_membership*)lp->content;
		TM_FREE(amp);
	}
	tm_list_delete( &(p->memberships) );

	TM_FREE(p);
}

int asidp_to_string(TMValueType vtype, const void *v, char* buf, size_t size)
{
	int n;
	char locbuf[1024];
	TMList lp;
	const struct asidp_value *asidp = (const struct asidp_value*)v;

	if(v == NULL)
	{
		*buf = '\0';
		return (1); /* FIXME */
	}
	bzero(locbuf,1024);
	for(lp=asidp->memberships;lp;lp=lp->next)
	{
		char b[64];
		sprintf(b,"(R:%d,x:%d)",
				((struct a_membership*)lp->content)->role,
				((struct a_membership*)lp->content)->player
		       );
		strcat(locbuf,b);
	}

	n = snprintf(buf,size,"ASIDP(type=%d, memberships=[%s])",asidp->type,locbuf);	
	assert(n < size);
	return n;
}
int asidp_to_xml(TMValueType vtype,const void *asidp, char* buf, size_t size)
{
	int n;
	n = snprintf(buf,size,"    <asidp>%s</asidp>",(char*)asidp);	
	assert(n < size);
	return n;
}

int asidp_equal(TMValueType vtype, const void *lhs, const void *rhs)
{
	assert(lhs);
	assert(rhs);
	assert(0);
	return ( strcmp((const char*)lhs,(const char *)rhs) == 0);
}


void *asidp_set_type(TMValueType vtype, void *asidp,va_list args)
{
	TMTopic type;
	type = va_arg(args,TMTopic);
	((struct asidp_value*)asidp)->type = type;
	return NULL;
}
/* FIXME */
static void *asidp_add_membership(TMValueType vtype, struct asidp_value *asidp,struct a_membership *mp)
{
	TMList lp,prev;

	if( ! asidp->memberships)
	{
		asidp->memberships = tm_list_push(asidp->memberships,mp);
	}
	else
	{
		prev = NULL;
		for(lp=asidp->memberships;lp;lp=lp->next)
		{
			if( ((struct a_membership*)lp->content)->role > mp->role)
				break;
			prev = lp;
		}
		if(!prev)
		{
			asidp->memberships = tm_list_push(asidp->memberships,mp);
		}
		else
		{
			TMList tmp;
			tmp = tm_list_push(NULL,mp);	
			tmp->next = prev->next;
			prev->next = tmp;
		}

	}
	assert(asidp->memberships);
/*
	prev = asidp->memberships;
	for(lp=asidp->memberships->next;lp;lp=lp->next);
	{
		assert( ((struct a_membership*)prev->content)->role < ((struct a_membership*)lp->content)->role);
		prev = lp;
	}
*/
/*
	asidp_to_string(&ASIDP,asidp,buf,1024);
	fprintf(stderr,"\n%s\n\n" , buf );
	getchar();
*/
	
	return NULL;
}
void *asidp_add_member(TMValueType vtype, void *v,va_list args)
{
	/*
	TMTopic type;
	*/
	TMList lp,prev;
	struct asidp_value *asidp;
	struct a_membership *mp;
/*
	char buf[1024];
*/

	asidp = (struct asidp_value*)v;
/*
	asidp_to_string(&ASIDP,asidp,buf,1024);
	fprintf(stderr,"\n%s\n\n" , buf );
*/

	TM_NEW(mp);
	mp->role = va_arg(args,TMTopic);
	mp->player = va_arg(args,TMTopic);

	if( ! asidp->memberships)
	{
		asidp->memberships = tm_list_push(asidp->memberships,mp);
	}
	else
	{
		prev = NULL;
		for(lp=asidp->memberships;lp;lp=lp->next)
		{
			if( ((struct a_membership*)lp->content)->role > mp->role)
				break;
			prev = lp;
		}
		if(!prev)
		{
			asidp->memberships = tm_list_push(asidp->memberships,mp);
		}
		else
		{
			TMList tmp;
			tmp = tm_list_push(NULL,mp);	
			tmp->next = prev->next;
			prev->next = tmp;
		}

	}
	assert(asidp->memberships);
/*
	prev = asidp->memberships;
	for(lp=asidp->memberships->next;lp;lp=lp->next);
	{
		assert( ((struct a_membership*)prev->content)->role < ((struct a_membership*)lp->content)->role);
		prev = lp;
	}
*/
/*
	asidp_to_string(&ASIDP,asidp,buf,1024);
	fprintf(stderr,"\n%s\n\n" , buf );
	getchar();
*/
	
	return NULL;
}
void *asidp_replace_topics(TMValueType vtype, void *v,va_list args)
{
	struct asidp_value *asidp;
	int i,len;
	TMTopicSet merge_set;
	TMTopic stays;
	int changed = 0;

	asidp = (struct asidp_value*)v;

	merge_set = va_arg(args,TMTopicSet);
	len = tm_topicset_size(merge_set);
	stays = tm_topicset_get(merge_set,0);
	/* start at 1 because we skip stays */
	for(i=1;i<len;i++)
	{
		TMList lp;
		struct a_membership *mp;
		TMTopic disap = tm_topicset_get(merge_set,i);
		if(asidp->type == disap)
		{
			asidp->type = stays;
			changed = 1;
		}
		for(lp=asidp->memberships;lp;lp=lp->next)
		{
			mp = (struct a_membership*)lp->content;
			if(mp->role == disap)
			{
				mp->role = stays;
				changed = 1;
			}
			if(mp->player == disap)
			{
				mp->player = stays;
				changed = 1;
			}
		}
	}
	if(changed)
	{
		TMList list,lp;
		list = asidp->memberships;	
		asidp->memberships = NULL;

		for(lp=list;lp;lp=lp->next)
		{	
			struct a_membership *p = (struct a_membership*)lp->content;
			asidp_add_membership(&ASIDP,asidp,p);
		}
		tm_list_delete(&list);
	}
	return (void*)changed;
}
void *asidp_get_player(TMValueType vtype, void *v,va_list args)
{
	TMTopic role;
	TMList lp;
	struct asidp_value *asidp;

	asidp = (struct asidp_value*)v;
	role = (TMTopic)va_arg(args,TMTopic);

	for(lp= asidp->memberships; lp; lp=lp->next)
	{
		struct a_membership *mp = (struct a_membership *)lp->content;
		if(mp->role == role)
			return (void*)mp->player;
	}
	return NULL;
}
void *asidp_parse_argstring(TMValueType vtype, int opcode, const char *argstring)
{
        switch(opcode) {
        case TM_OP_ASIDP_HAS_TYPE:
                return (char*)argstring;  /* FIXME: is that nice to do? */
        case TM_OP_ASIDP_HAS_PLAYER_PAIR: {
		struct a_membership *mp;
		TM_NEW(mp);
		sscanf(argstring,"(%d,%d)", (int*)&(mp->role),(int*)&(mp->player));
		return (mp);
		break; }
        default:
                assert(0);
        }
        return NULL;
}


