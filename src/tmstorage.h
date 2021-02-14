/*
 * $Id: topicmap.h,v 1.9 2002/10/02 22:06:25 jan Exp $
 *
 * Copyright (c) 2002 Jan Algermissen
 * See the file "COPYING" for copying permission.
 *
 */
#ifndef TM_STORAGE_H
#define TM_STORAGE_H


#ifdef __cplusplus
extern "C" {
#endif

/** \defgroup StorageAPI Storage API
 * This API must be implemented...
 # and this and that.
 */
typedef struct TMTopicMapStorageDescriptor *TMTopicMapStorageDescriptor;


#include "tm.h"
#include "tmtk.h"
#include "tmerror.h"
#include "tmstack.h"
#include "tmlist.h"
#include "tmtopicset.h"
#include "tmproperty.h"
#include "tmtopicmap.h"

#include <stdio.h>  /* for FILE */

 


/* FIXME: make macro for base class API, use in each struct for clarity */ 


/** \defgroup ValueStoreAPI Value Storage API
 * \ingroup StorageAPI
 * This is the value store API.
 * core part of the system
 */
/**@{ */


/**
 * Value lookup flags.
 * These do some good stuff.
 */ 
enum {
	TM_VALUE_CREATE,		/**< Create value without implicit lookup */
	TM_VALUE_LOOKUP,		/**< Lookup only */
	TM_VALUE_LOOKUP_OR_CREATE	/**< Lokkup value, create if not found */
}; 

typedef void* TMExpression;
typedef struct TMValueStoreScan *TMValueStoreScan;
struct TMValueStoreScan {
	TMVid next_vid;
	int op;
	const void *arg;
	void *data;
};
/**
 *
 */
typedef struct TMValueStoreType *TMValueStoreType;

/**
 *
 */
typedef struct TMValueStore *TMValueStore;

/**@}*/

/** \defgroup MetaStoreAPI Meta Storage API
 * \ingroup StorageAPI
 * This is the meta store API.
 * core part of the system
 */
/**@{ */

/**
 * Store for global data about the map.
 * This store for example stores the names of loaded TMAs
 */
typedef struct TMMetaStoreType *TMMetaStoreType;

/**
 *
 */
typedef struct TMMetaStore *TMMetaStore;

/**@}*/

/** \defgroup PropertyStoreAPI Property Storage API
 * \ingroup StorageAPI
 * This is the proerty store API.
 * core part of the system
 */
/**@{ */

/**
 *
 */
typedef struct TMPropertyStoreType *TMPropertyStoreType;

/**
 *
 */
typedef struct TMPropertyStore *TMPropertyStore;

/**@}*/




/* =========================================================================

                                 TMMetaStore

   ========================================================================= */ 

/** \addtogroup ValueStoreAPI
 */
/**@{ */

/**
 *
 */
typedef TMMetaStore(*TMMetaStoreNewFunction)(void*);

/**
 *
 */
typedef void(*TMMetaStoreDeleteFunction)(TMMetaStore);

/**
 *
 */
typedef TMError(*TMMetaStoreOpenFunction)(TMMetaStore);

/**
 *
 */
typedef TMError(*TMMetaStoreCloseFunction)(TMMetaStore);

/**
 *
 */
typedef TMError(*TMMetaStoreGetModelListFunction)(TMMetaStore,TMList*);

/**
 *
 */
typedef TMError(*TMMetaStoreAddModelFunction)(TMMetaStore,const char*);

/**
 *
 */
typedef TMError(*TMMetaStoreGetNewNodeFunction)(TMMetaStore,TMTopic *);

/**
 *
 */
typedef TMError(*TMMetaStoreDisposeNodeFunction)(TMMetaStore,TMTopic);

/**
 *
 */
typedef TMError(*TMMetaStoreGetNodemaxFunction)(TMMetaStore,TMTopic*);

struct TMMetaStoreType {

	TMMetaStoreNewFunction new;
	TMMetaStoreDeleteFunction delete;
	TMMetaStoreOpenFunction open;
	TMMetaStoreCloseFunction close;

	TMMetaStoreGetModelListFunction get_model_list;
	TMMetaStoreAddModelFunction add_model;
	TMMetaStoreGetNewNodeFunction get_new_topic;
	TMMetaStoreDisposeNodeFunction dispose_topic;
	TMMetaStoreGetNodemaxFunction get_topicmax;
};

struct TMMetaStore {
	TMMetaStoreType type;
};


/**@}*/

/** \addtogroup PropertyStoreAPI
 */
/**@{ */


/* =========================================================================

                                 TMPropertyStore

   ========================================================================= */ 


typedef TMPropertyStore(*TMPropertyStoreNewFunction)(void*,TMProperty,TMValueStore vstore);
typedef void(*TMPropertyStoreDeleteFunction)(TMPropertyStore);
typedef TMError(*TMPropertyStoreOpenFunction)(TMPropertyStore);
typedef TMError(*TMPropertyStoreCloseFunction)(TMPropertyStore);

typedef TMError(*TMPropertyStoreInsertFunction)(TMPropertyStore,TMTopic,TMVid);
typedef TMError(*TMPropertyStoreRemoveFunction)(TMPropertyStore,TMTopic);
typedef TMError(*TMPropertyStoreLookupNodeFunction)(TMPropertyStore,TMVid,TMTopic*);
typedef TMError(*TMPropertyStoreGetVidOfTopicFunction)(TMPropertyStore,TMTopic,TMVid*);

typedef TMError(*TMPropertyStoreLookupNodesFunction)(TMPropertyStore,TMVid,TMTopic**array, int *lenp);

typedef TMError(*TMPropertyStoreNodeReplaceFunction)(TMPropertyStore,TMTopic,TMTopic,TMStack,TMStack);
typedef TMError(*TMPropertyStorePrintFunction)(TMPropertyStore,FILE *);
typedef TMError(*TMPropertyStoreIsNormalizedFunction)(TMPropertyStore);
typedef TMError(*TMPropertyStoreNormalizeFunction)(TMPropertyStore,TMTopicMap);

struct TMPropertyStoreType {

	TMPropertyStoreNewFunction new;
	TMPropertyStoreDeleteFunction delete;
	TMPropertyStoreOpenFunction open;
	TMPropertyStoreCloseFunction close;

	TMPropertyStoreInsertFunction insert;
	TMPropertyStoreRemoveFunction remove;
	TMPropertyStoreLookupNodeFunction lookup_topic;
	TMPropertyStoreLookupNodesFunction lookup_topics;
	TMPropertyStoreGetVidOfTopicFunction get_vid_of_topic;

	TMPropertyStorePrintFunction print;
	TMPropertyStoreIsNormalizedFunction is_normalized;
	TMPropertyStoreNormalizeFunction normalize;
};

struct TMPropertyStore {
	TMPropertyStoreType type;
};


/* =========================================================================

                                 TMValueStore

   ========================================================================= */ 
/**@}*/

/** \addtogroup ValueStoreAPI
 */
/**@{ */

/** Create a new value store.
 * In this function, all creation and initialization can take place.
 * There is a seperate open function, that can deal with any opening
 * activities.
 */
typedef TMValueStore(*TMValueStoreNewFunction)(void*, TMProperty prop,void *param);
/** Delete the value store.
 * Put all deletion action here.
 */
typedef void(*TMValueStoreDeleteFunction)(TMValueStore);
/** Open the store.
 * Any opening activities can take place here. The open/close functions
 * may be called more than once in the lifecycle of a store. On example for
 * this is the closing of files if a max-open-file limit is reached.
 */

typedef TMError(*TMValueStoreOpenFunction)(TMValueStore);
/** Close a store.
 * This function closes the store, propably only temporarily.
 */
typedef TMError(*TMValueStoreCloseFunction)(TMValueStore);

/** Lookup the VID of a given value.
 *  This function handles all lookup of VIDs for values, including implicit
 * creation if value is not yet in store.
 * The flag controls the behaviour of the function:
 * @li @c TM_VALUE_LOOKUP Only perform lookup, no implicit creation if value is not found
 * @li @c TM_VALUE_LOOKUP_OR_CREATE Perform lookup and create value if not found
 * @li @c TM_VALUE_CREATE Explicitly create the value. This is use to avoid lookup when the caller knows that value is not in store. It is an erro rif value is in store, check with assertion!!
 *
 * 
 */
typedef TMError(*TMValueStoreLookupVidFunction)(TMValueStore,const void*,int flag,TMVid*);

/**
 * Delete a value from the store.
 * This function must delete the value that corresponds to a certain VID. The VID is
 * invalid after the deletion. It is an error if no value exists for the VID in question.
 * Use an assertin to veryfy this.
 */
typedef TMError(*TMValueStoreDeleteValueFunction)(TMValueStore,TMVid);
/**
 * Get the value that is identified by a certain VID.
 * This function must simply return the value that is stored for a certain
 * VID. The value must be newly allocated, responsibility for this value is
 * fully passed to the caller of this function.
 * It is an error if there exists no value for this VID, caller is responsible for
 * checking this. At least uyse an assertion to test this condition.
 */
typedef TMError(*TMValueStoreGetValueFunction)(TMValueStore,TMVid, void**);
typedef TMError(*TMValueStoreScanOpenFunction)(TMValueStore, int, const void *, TMValueStoreScan* );
typedef TMError(*TMValueStoreScanCloseFunction)(TMValueStore,TMValueStoreScan);
typedef TMError(*TMValueStoreScanPrepareNextFunction)(TMValueStore,TMValueStoreScan);

/*
typedef TMError(*TMValueStoreFetchFunction)(TMValueStore,TMVid *vidp);
*/
/** Print the store.
 * This is used for dumping the topic map.
 */
typedef TMError(*TMValueStorePrintFunction)(TMValueStore,FILE *);

/** The value store type object struct.
 * Implementations must 'hook' the functions into such a struct.
 */
struct TMValueStoreType {

	TMValueType valuetype;

	TMValueStoreNewFunction new;
	TMValueStoreDeleteFunction delete;
	TMValueStoreOpenFunction open;
	TMValueStoreCloseFunction close;

	TMValueStoreLookupVidFunction lookup_vid;
	TMValueStoreDeleteValueFunction delete_value;
	TMValueStoreGetValueFunction get_value;
	TMValueStorePrintFunction print;

	TMValueStoreScanOpenFunction scan_open;
	TMValueStoreScanCloseFunction scan_close;
	TMValueStoreScanPrepareNextFunction scan_prepare_next;

};

/** Value store 'base class'.
 * Value store implementations can
 * provide additional members. This is what the functions operate
 * upon, so there would be stuff like data ind index file handles,
 * open DB connections and the like.
 */
struct TMValueStore {
	TMValueStoreType type;
};




/**@}*/



/* =========================================================================

                                Other Stuff 

   ========================================================================= */ 

/** Storage inititializing function.
 *
 * @param tm the TopicMap object that opens this storage.
 * @param 2 the storage argument that the caller of tm_topicmap_new
 * passed in.
 * @param 3 init functions can set this pointer to a data structire
 * the storage needs/controls. TM object will pass this to all subsequent stoage calls.
 *
 *
 * Notes for stprage developers: init MUST report errors
 * using tm_topicmap_set_error() on tm. The calling TM object
 * will not set the error itself.
 */
typedef TMError (*TMStorageInitFunction)(TM tm,void*,void**);


/** Storage cleanup code
 *
 */
typedef void (*TMStorageCleanupFunction)(void*);



/** Storage decsriptor structure.
 *
 */
struct TMTopicMapStorageDescriptor {
	const char *name;
	TMMetaStoreType meta_store_type;
	TMPropertyStoreType property_store_type;
	TMValueStoreType *valuestore_types;
	/*
	TMAssertionStoreType assertion_store_type;
	*/


	/* need a sort of table that maps valuetypenames to storage types!!! */
	TMStorageInitFunction init_store;
	TMStorageCleanupFunction cleanup_store;
};


/**@} */ 

/** \defgroup Storage
 * TMTK internal storage API (used by TMTopicMap)
 * These functions call the corresponding functions of the
 * store objects.
 * @todo propably make these macros for efficiency.
 */
/**@{ */
TM_API(TMMetaStore) tm_meta_store_new(TMTopicMapStorageDescriptor desc, void *handle);
TM_API(TMError) tm_meta_store_open(TMMetaStore self);
TM_API(TMError) tm_meta_store_close(TMMetaStore self);
TM_API(void) tm_meta_store_delete(TMMetaStore self);

TM_API(TMError) tm_meta_store_get_model_list(TMMetaStore self,TMList *);
TM_API(TMError) tm_meta_store_add_model(TMMetaStore self,const char *name);
TM_API(TMError) tm_meta_store_get_new_topic(TMMetaStore self,TMTopic *);
TM_API(TMError) tm_meta_store_dispose_topic(TMMetaStore self,TMTopic);
TM_API(TMError) tm_meta_store_get_topicmax(TMMetaStore self,TMTopic*);


TM_API(TMValueStore) tm_value_store_new(TMTopicMapStorageDescriptor , void *, TMProperty);
TM_API(TMError) tm_value_store_open(TMValueStore self);
TM_API(TMError) tm_value_store_close(TMValueStore self);
TM_API(void) tm_value_store_delete(TMValueStore self);


TM_API(TMError) tm_value_store_lookup_vid(TMValueStore self,const void *v,int flag,TMVid *pvid);
TM_API(TMError) tm_value_store_delete_value(TMValueStore self,TMVid vid);
TM_API(TMError) tm_value_store_get_value(TMValueStore self,TMVid vid, void** vp);
TM_API(TMError) tm_value_store_print(TMValueStore self,FILE *);
TM_API(TMError) tm_value_store_scan_open(TMValueStore self, int opcode, const void *arg, TMValueStoreScan *sp);
TM_API(TMError) tm_value_store_scan_close(TMValueStore self,TMValueStoreScan s);
TM_API(int) tm_value_store_scan_finished(TMValueStore self,TMValueStoreScan s);
TM_API(TMError) tm_value_store_scan_fetch(TMValueStore self,TMValueStoreScan s, TMVid *vidp);


TM_API(TMPropertyStore) tm_property_store_new(TMTopicMapStorageDescriptor desc, void *handle,TMProperty, TMValueStore vstore);
TM_API(TMError) tm_property_store_open(TMPropertyStore self);
TM_API(TMError) tm_property_store_close(TMPropertyStore self);
TM_API(void) tm_property_store_delete(TMPropertyStore self);

TM_API(TMError) tm_property_store_insert(TMPropertyStore self, TMTopic topic, TMVid vid);
TM_API(TMError) tm_property_store_remove(TMPropertyStore self, TMTopic topic);
TM_API(TMError) tm_property_store_lookup_topic(TMPropertyStore self, TMVid vid, TMTopic *np);
TM_API(TMError) tm_property_store_lookup_vid(TMPropertyStore self, TMTopic topic, TMVid *vidp);
TM_API(TMError) tm_property_store_get_vid_of_topic(TMPropertyStore self, TMTopic topic, TMVid *vidp);
TM_API(TMError) tm_property_store_lookup_topics(TMPropertyStore self, TMVid vid, TMTopic **topicsp, int *lenp);

TM_API(TMError) tm_property_store_print(TMPropertyStore self,FILE *);
TM_API(TMError) tm_property_store_is_normalized(TMPropertyStore self);
TM_API(TMError) tm_property_store_normalize(TMPropertyStore self,TMTopicMap topicmap);
/**@} */ 
#ifdef __cplusplus
} // extern C
#endif
	
#endif

