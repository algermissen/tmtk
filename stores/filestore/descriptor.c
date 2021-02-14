#include <tmdefaultstores.h>
#include <tmutil.h>
#include <tmmem.h>

#include <sys/stat.h>
#include <sys/types.h>

#include "descriptor.h"

extern struct TMMetaStoreType default_file_meta_store_type;
/*
extern struct TMValueStoreType default_file_locator_store_type;
*/



TMError filestore_init(TMTopicMap topicmap, void *args, void **handle_ptr)
{
	struct data *dp;
	struct stat stat_buf;

	if(stat((char*)args,&stat_buf) < 0)
	{
		TMTRACE(TM_STORAGE_TRACE, "cannot stat %s, (%s) \n" _
				(char*)args _ strerror(errno) );

		tm_topicmap_set_error(topicmap,"cannot initialize filestore %s, %s" ,
				(char*)args , strerror(errno) );
		return TM_ESYS;
	}
	if( ! S_ISDIR(stat_buf.st_mode) )
	{
		TMTRACE(TM_STORAGE_TRACE, "%s is not a directory\n" _ (char*)args );

		tm_topicmap_set_error(topicmap,
			"cannot initialize filestore, %s is not a directory" ,
			(char*)args );
		return TM_ESYS; /* FIXME: what error ?? */
	}

	TM_NEW(dp);
	dp->topicmap = topicmap;
	
	dp->open_count = 0;
	dp->open_max = 256;
	dp->path = tm_strdup((char*)args);
	bzero(dp->errstr,sizeof(dp->errstr));

	TMTRACE(TM_STORAGE_TRACE,"stat ok, filestorage init [data=%s]\n" _ (char*)args);

	*handle_ptr = dp;

	return TM_OK;
}

void filestore_cleanup(void *args)
{
	struct data *dp = (struct data*)args;
	
	TM_FREE(dp->path);
	TM_FREE(dp);
}

static TMValueStoreType vstores[] = {
	/*
	&default_file_locator_store_type,
	*/
	NULL
};

struct TMTopicMapStorageDescriptor default_file_storage_descriptor = {
	&default_file_meta_store_type,
	NULL,
	vstores,	
	/* assertion store was here */
	filestore_init,
	filestore_cleanup
};

