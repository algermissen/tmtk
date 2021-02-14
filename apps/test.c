#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <topicmaps.h>
#include <errno.h>
#include <stdarg.h>

int main(int argc, char **args)
{
	TM tm;	
	TMTopicMap topicmap;
	/*
	TMError e;
	*/
	if(tm_init(&tm,"www-df",NULL) != TM_OK)
	{
		fprintf(stderr,"ERROR: %s\n",tm_get_error(tm));
	}

	topicmap = tm_topicmap_new(tm);
	if( tm_topicmap_set_storage(topicmap,"MemStore") != TM_OK)
	{
		fprintf(stderr,"ERROR: %s\n",tm_get_error(tm));
	}
	if( tm_topicmap_open(topicmap,NULL) != TM_OK)
	{
		fprintf(stderr,"ERROR: %s\n",tm_get_error(tm));
	}
	/*
	if( tm_topicmap_require_model(topicmap,"http://www.gooseworks.org/disclosures/SAM.xml") != TM_OK)
	*/
	if( tm_topicmap_require_model(topicmap,args[1]) != TM_OK)
	{
		fprintf(stderr,"ERROR: %s\n",tm_get_error(tm));
	}
	tm_topicmap_dump(topicmap,stdout);
	if( tm_topicmap_close(topicmap) != TM_OK)
	{
		fprintf(stderr,"ERROR: %s\n",tm_get_error(tm));
	}
	tm_topicmap_delete(&topicmap);

	tm_cleanup(&tm);
	return(0);
}
