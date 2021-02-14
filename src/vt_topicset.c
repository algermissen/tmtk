#include "tmvaluetype.h"
#include "tmtrace.h"
#include "tmmem.h"
#include "tmlist.h"
#include "tmtopicset.h"
#include "tmassert.h"

#include "tmbasictypes.h"


static int topicset_equal(TMValueType, const void *, const void*);
static void topicset_free(TMValueType vtype, void *s);
static void* topicset_new(TMValueType vtype);
static void* topicset_new_from_string(TMValueType vtype,const char *s);
static int topicset_to_string(TMValueType, const void *topicset, char* buf, size_t size);
static int topicset_to_xml(TMValueType, const void *topicset, char* buf, size_t size);
static void* topicset_add_member(TMValueType, void *topicset,va_list args);
/*
static void* topicset_get_player(TMValueType, void *topicset,va_list args);
static void* topicset_set_type(TMValueType, void *topicset,va_list args);
*/
static void* topicset_replace_topics(TMValueType, void *topicset,va_list args);
static void *topicset_parse_argstring(TMValueType vtype, int opcode, const char *argstring);

static struct function_table ftab[] = {
	{ "addMember" , topicset_add_member },
	{ "replaceTopics" , topicset_replace_topics },
	{ NULL , NULL}
};
static struct op_table ops[] = {
	/*
        { "HAS_TYPE" , TM_OP_ASIDP_HAS_TYPE },
        { "HAS_PLAYER_PAIR" , TM_OP_ASIDP_HAS_PLAYER_PAIR },
	*/
        { "VALUE_INCLUDES_TOPIC" , TM_OP_TOPICSET_VALUE_INCLUDES_TOPICS },
        { NULL, 0 }
};

struct TMValueType TopicSet = {
        "TopicSet",
	1,		/* values contain topics */
	topicset_new,
	topicset_new_from_string,
	topicset_to_string,
	topicset_to_xml,
	topicset_equal,
	topicset_free,
	ftab,
	ops,
	topicset_parse_argstring

};

void* topicset_new(TMValueType vtype)
{
	TMTopicSet self;
	self = tm_topicset_new(5);
	return self;
}
void* topicset_new_from_string(TMValueType vtype, const char *s)
{
	assert(0);
	/*
	struct topicset_value *p;
	TM_NEW(p);
	p->type = 0;
	p->memberships = NULL;
	return p;
	*/
	return NULL;
}

void topicset_free(TMValueType vtype, void *v)
{
	TMTopicSet self;
	self = (TMTopicSet)(TMTopicSet*)v;
	tm_topicset_delete(&self);
}

int topicset_to_string(TMValueType vtype, const void *v, char* buf, size_t size)
{
	TMTopicSet self;
	char locbuf[1024];
	int n,i;
	locbuf[0] = '\0';
	self = (TMTopicSet)v;
	n = tm_topicset_size(self);
	for(i=0;i<n;i++)
	{
		char b[43];
		sprintf(b,"%d,",tm_topicset_get(self,i));
		strcat(locbuf,b);
	}
	/* FIXME everything */	
	n = snprintf(buf,size,"TopicSet[%s]",locbuf);
	return n;
}
int topicset_to_xml(TMValueType vtype,const void *topicset, char* buf, size_t size)
{
	int n;
	char locbuf[1024];
	topicset_to_string(vtype,topicset,locbuf,1024);
	n = snprintf(buf,size,"    <topicset>%s</topicset>",locbuf);	
	assert(n < size);
	return n;
}

int topicset_equal(TMValueType vtype, const void *lhs, const void *rhs)
{
	assert(lhs);
	assert(rhs);
	return ( tm_topicset_equal((TMTopicSet)lhs,(TMTopicSet)rhs) );
}


void *topicset_add_member(TMValueType vtype, void *v,va_list args)
{
	TMTopicSet self;
	TMTopic topic;
	self = (TMTopicSet)v;

	topic = va_arg(args,TMTopic);
	tm_topicset_add(self,topic);

	return NULL;
}
void *topicset_replace_topics(TMValueType vtype, void *v,va_list args)
{
	TMTopicSet self;
	int i,len;
	TMTopicSet merge_set;
	TMTopic stays;
	int changed = 0;


	self = (TMTopicSet)v;

	merge_set = va_arg(args,TMTopicSet);
	len = tm_topicset_size(merge_set);
	stays = tm_topicset_get(merge_set,0);
	/* start at 1 because we skip stays */
	for(i=1;i<len;i++)
	{
		TMTopic disap = tm_topicset_get(merge_set,i);
		if( tm_topicset_contains(self,disap) )
		{
			assert(!tm_topicset_contains(self,stays));
			/* FIXME:  not sure now what this case means. See also in
			ASIDP!!! what problems arise when stays is also there
			example: list size decreases! */
			tm_topicset_remove(self,disap);
			tm_topicset_add(self,stays);
			
			changed = 1;
		}
	}
	return (void*)changed;
}

void *topicset_parse_argstring(TMValueType vtype, int opcode, const char *argstring)
{
	assert(0);
#if 0
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
#endif
        return NULL;
}


