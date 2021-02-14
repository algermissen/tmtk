#include <tmmem.h>
#include <tmutil.h>

#include "descriptor.h"

extern struct TMMetaStoreType default_mem_meta_store_type;

extern struct TMPropertyStoreType default_mem_property_store_type;

extern struct TMValueStoreType default_mem_text_store_type;
extern struct TMValueStoreType default_mem_topicset_store_type;
extern struct TMValueStoreType default_mem_integer_store_type;
extern struct TMValueStoreType default_mem_asidp_store_type;
extern struct TMValueStoreType default_mem_locatorset_store_type;



TMError memstore_init(TM tm,void *args, void **handle_ptr)
{
	struct data *dp;
	TM_NEW(dp);

	bzero(dp->errstr,sizeof(dp->errstr));
	dp->tm = tm;

	*handle_ptr = dp;
	
	return TM_OK;
}

void memstore_cleanup(void *args)
{
	struct data *dp = (struct data*)args;
	
	TM_FREE(dp);
}
static TMValueStoreType vstores[] = {
        &default_mem_text_store_type,
        &default_mem_integer_store_type,
        &default_mem_locatorset_store_type,
        &default_mem_topicset_store_type,
        &default_mem_asidp_store_type,
        NULL
};

struct TMTopicMapStorageDescriptor default_mem_storage_descriptor = {
	"MemStore",
	&default_mem_meta_store_type,
	&default_mem_property_store_type,
	vstores,
	memstore_init,
	memstore_cleanup
};

