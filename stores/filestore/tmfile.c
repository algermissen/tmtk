/*
 * $Id$
 *
 * Copyright (c) 2002 Jan Algermissen
 * See the file "COPYING" for copying permission.
 *
 */
#include <sys/stat.h> /* for stat() */

#include "tmfile.h"
#include <tmutil.h>
#include <tmmem.h>
#include <tmtrace.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>



/* we need some cache, since pages need to be kept as long as they are
 * accessed (between fetch and drop).
 */
#define MIN_CACHE_SIZE (TM_PAGE_SIZE * 100)  


/* the cache consists of these items */
struct page_item {
	TMPage page;	/* tm_file_fetch_page() returns this pointer */ 
	int usecount;	/* usecount > 0 means also 'is locked' */
	/* FIXME: timestamp? */
};
	

struct TMFile {
	int fd;			/* the file handle */	
	int is_open;		/* open status */
	double cache_size;	/* current cache size */
	char *path;		/* file name */
	char errstr[1024];	/* holds error */
	struct {		/* the cache */
		struct page_item *page_items;		
		int pcnt;	/* number of used items */
		int pmax;	/* maximum number (cache_size / page size) */
	} cache;
};

/* local rountines */
static const char *_set_error(TMFile self, const char *fmt,...);
static TMError _readpage(TMFile self,TMPage p, int n);
static TMError _writepage(TMFile self,TMPage p);


/* FIXME: add flag for immediate write etc. !! */
TMFile tm_file_new(const char *path, long cache_size)
{
	TMFile self;
	int i;

	assert(path);
	
	/* FIXME: */
	assert(sizeof(int) == 4);
	assert(sizeof(off_t) == 4);

	TM_NEW(self);
	if(!self)
		return NULL;
	self->is_open = 0;

	self->cache_size = (cache_size >= MIN_CACHE_SIZE) ? cache_size :
							    MIN_CACHE_SIZE;
	self->path = tm_strdup(path);	
	bzero(self->errstr,sizeof(self->errstr));

	self->cache.pmax = self->cache_size / TM_PAGE_SIZE;
	self->cache.page_items = (struct page_item*)TM_ALLOC(
			self->cache.pmax * sizeof(struct page_item) );	
	assert(self->cache.page_items);

	for(i = 0; i < self->cache.pmax; i++)
	{
		self->cache.page_items[i].page = 
				(TMPage)TM_ALLOC(TM_PAGE_SIZE);	
		assert(self->cache.page_items[i].page);
		self->cache.page_items[i].usecount = 0;
	}
	self->cache.pcnt = 0;

	return self;
}

void tm_file_delete(TMFile *pself)
{
	TMFile self;
	int i;
	assert(pself && *pself);
	self = *pself;
	assert(!self->is_open);
	for(i = 0; i < self->cache.pmax; i++)
		TM_FREE(self->cache.page_items[i].page);
	TM_FREE(self->cache.page_items);
	TM_FREE(self->path);
	TM_FREE(self);
}
TMError tm_file_open(TMFile self, int *created)
{
	struct stat stat_buf;

	TMTRACE(TM_STORAGE_TRACE,"tm_file_open(): enter path=%s\n" _ self->path);
	assert(created);
	assert(!self->is_open);
	*created = 0;

	if(stat(self->path,&stat_buf) < 0)
	{
		TMTRACE(TM_STORAGE_TRACE,
			"cannot stat %s, (%s) opening with CREAT\n" _
			self->path _ strerror(errno) );
	
		if( (self->fd = open(self->path,
				O_RDWR | O_CREAT, S_IRUSR | S_IWUSR)) < 0)
		{
			_set_error(self,"cannot create file %s, %s",
				self->path, strerror(errno));
			return TM_ESYS;
		}
		TMTRACE(TM_STORAGE_TRACE,"created %s\n" _ self->path);
		*created = 1;
		self->is_open = 1;
		return TM_OK;
	}


	if( (self->fd = open(self->path,O_RDWR, S_IRUSR | S_IWUSR)) < 0)
        {
		_set_error(self,"cannot open file %s, %s",
			self->path, strerror(errno));
		return TM_ESYS;
        }
	TMTRACE(TM_STORAGE_TRACE,"opened %s\n" _ self->path);

	self->is_open = 1;
	return TM_OK;
}


TMError tm_file_close(TMFile self)
{
	int i;
	assert(self);
	assert(self->is_open);
	TMTRACE(TM_STORAGE_TRACE,"tm_file_close(): enter [%s]\n" _ self->path);

	for(i = 0; i < self->cache.pcnt; i++)
	{
		assert(self->cache.page_items[i].usecount == 0);

		/* this assertion enforces the client to flush the file
		 * before calling close. I think that is good, so I leave
		 * the assertion here.
		 */
		assert(! tm_page_changed(self->cache.page_items[i].page));

		if(gs_page_get_changedflag(self->cache.page_items[i].page))
		{
			if(_writepage(self,
				self->cache.page_items[i].page) != TM_OK)
				return TM_ESYS;
		}
	}

	if( close(self->fd) < 0 )
	{
		_set_error(self,"cannot close file %s, %s",
			self->path, strerror(errno));
		return TM_ESYS;
	}
	TMTRACE(TM_STORAGE_TRACE,"tm_file_close(): exit\n");
	
	self->is_open = 0;
	return TM_OK;
}

const char *tm_file_get_error(TMFile self)
{
        assert(self);
        return (self->errstr);
}

TMError tm_file_create_page(TMFile self,GS_INITFUNC_T initfunc,int *np)
{
	off_t offset;
	int n;
	char empty[TM_PAGE_SIZE];	/* an empty page to do initialization
					   on */
	TMPage p;			/* pointer to that page */
	TMTRACE(TM_STORAGE_TRACE,"tm_file_create_page(): enter\n");

	p = empty;

	assert(self);
	assert(np);

		
	if( (offset = lseek(self->fd,0,SEEK_END)) < 0)
	{
		_set_error(self,"cannot seek to end %s, %s",
			self->path, strerror(errno));
		return TM_ESYS;
	}

	TMTRACE(TM_STORAGE_TRACE,"tm_file_create_page(): end of last page: %ld\n" _ offset);

	n = ( (int)offset / TM_PAGE_SIZE ) + 1;

	/* initialize page either with caller's function or the standard one*/
	if(initfunc)
		initfunc(p,n);
	else
		tm_page_init(p,n);

	if( write(self->fd,p,TM_PAGE_SIZE) != TM_PAGE_SIZE)
	{
		_set_error(self,"cannot write new page to %s, %s",
			self->path, strerror(errno));
		return TM_ESYS;
	}
	*np = n;
	TMTRACE(TM_STORAGE_TRACE,"tm_file_create_page(): exit n=%d\n" _ *np);
	return TM_OK;
}

TMError tm_file_fetch_page(TMFile self,int n,TMPage *bp)
{
	int i;
	assert(self);
	assert(bp);
	assert(n > 0);
	*bp = NULL;

	TMTRACE(TM_STORAGE_TRACE,"tm_file_fetch_page(): page %d of %s\n" _
					n _ self->path);

	/*
	 * Step 1: Look for page in our cache.
	 */
	for (i=0; i<self->cache.pcnt; i++)
	{
		if(gs_page_get_number(self->cache.page_items[i].page) == n)
		{
			self->cache.page_items[i].usecount++;

			/* XXXX */
			/*
			_readpage(self->f,self->cache.page_items[i].page,n);
			*/

			/* set callers page pointer to the cached page */
			*bp = self->cache.page_items[i].page;
			return TM_OK;
		}
	}

	/*
	 * Page was not found in the cache, now we need
	 * Step 2: get page from disk if still room in cache
	 */
	
	if(self->cache.pcnt < self->cache.pmax)
	{
		i = self->cache.pcnt;
		/* FIXME: we might keep the old page and restore if fails */

		if(_readpage(self,self->cache.page_items[i].page,n) != TM_OK)
			return TM_ESYS;

		self->cache.page_items[i].usecount = 1;
		*bp = self->cache.page_items[i].page;
		self->cache.pcnt++;
		return TM_OK;
	}


	/*
	 * Since now space left in cache, we need
	 * Step 3: throw one page away and load requested page
	 * from disk.
	 * FIXME: use better algorithm? Does it matter?
	 */

	for(i = 0; i < self->cache.pmax; i++)
	{
		if(!self->cache.page_items[i].usecount == 0)
		{
			/* take the first available spot */
			break;
		}
	}

	/* FIXME: how to handle that situation */
	assert(i < self->cache.pmax); /* or cache too small !! */

	if(gs_page_get_changedflag(self->cache.page_items[i].page))
	{
		if(_writepage(self,self->cache.page_items[i].page) != TM_OK)
			return TM_ESYS;
	}

	if(_readpage(self,self->cache.page_items[i].page,n) != TM_OK)
		return TM_ESYS;

	assert(gs_page_get_number(self->cache.page_items[i].page) == n);
		
	self->cache.page_items[i].usecount = 1;
	*bp = self->cache.page_items[i].page;

	return TM_OK;
}

TMError tm_file_drop_page(TMFile self, TMPage page)
{
	int i;

	assert(self);
	assert(page);

	TMTRACE(TM_STORAGE_TRACE,
		"tm_file_drop_page(): page %d of %s\n" _
		gs_page_get_number(page) _ self->path);

	for(i = 0; i < self->cache.pmax; i++)
	{
		if(self->cache.page_items[i].page == page)
		{
			assert(self->cache.page_items[i].usecount > 0);
			self->cache.page_items[i].usecount--;
			/* XXXX */
			/*
			_writepage(self,self->cache.page_items[i].page);
			*/
			return TM_OK;
		}
	}

	return TM_OK;
}


TMError tm_file_flush_cache(TMFile self)
{
	int i;
	assert(self);
	
	for(i = 0; i < self->cache.pcnt; i++)
	{
		if(tm_page_changed(self->cache.page_items[i].page))
		{
			if(_writepage(self,self->cache.page_items[i].page)
				!= TM_OK)
				return TM_ESYS;
		}
	}
	return TM_OK;
}

#if 0


static const char *files[] = {
	"topics",
	"data",
	"locpfxtree",
	"wordpfxtree",
	"wordindex",
	NULL
};

TMError gs_storage_create_topicmap(const char *name)
{
	char buf[256];
	int i = 0;
	int fd;
	assert(name);


	assert(name[strlen(name)-1] != '/');	
	TMTRACE(TM_STORAGE_TRACE,"creating topicmap _%s_\n" _ name );

	if( mkdir(name,O_RDWR | O_CREAT | O_EXCL) < 0)
	{
		TMTRACE(TM_STORAGE_TRACE,"cannot create directory %s, %s\n" _
			name _ strerror(errno));
		return GS_ESYS;
	}
	if( chmod(name,S_IRUSR | S_IWUSR | S_IXUSR) )
	{
		/* FIXME: unlink directory ?? */
		TMTRACE(TM_STORAGE_TRACE,"cannot chmod on %s, %s\n" _
			buf _ strerror(errno));
		return GS_ESYS;
	}
	while(files && files[i] != NULL)
	{
		snprintf(buf,sizeof(buf),"%s/%s",name,files[i]);
		if( (fd = open(buf,O_RDWR | O_CREAT, S_IRUSR | S_IWUSR)) < 0)
		{
			/* FIXME: unlink directory ?? */
			TMTRACE(TM_STORAGE_TRACE,"cannot open %s, %s\n" _
				buf _ strerror(errno) );
			return GS_ESYS;
		}
		i++;
	}

	return GS_OK;
}

TMError gs_storage_destroy_topicmap(const char *name)
{
	char buf[GS_MAXPATH];
	assert(name);

	TMTRACE(TM_STORAGE_TRACE,"destroying topicmap _%s_\n" _ name );

	/* ok, if we got here, topicmap has been removed an we can delete */
	
	snprintf(buf,sizeof(buf),"topicmaps/%s/topics",name);

	/* check if user has permissions to destroy topicmap */

	if( unlink(buf) < 0)
	{
		TMTRACE(TM_STORAGE_TRACE,"unlink error: %s\n" _ strerror(errno));
		/*
		gs_thread_set_error(tp,GS_EXXX,
			"cannot remove topics file, %s",strerror(errno));
		*/
		return GS_ESYS;
	}
	snprintf(buf,sizeof(buf),"topicmaps/%s/data",name);

	/* check if user has permissions to destroy topicmap */

	if( unlink(buf) < 0)
	{
		TMTRACE(TM_STORAGE_TRACE,"unlink error: %s\n" _ strerror(errno));
		/*
		gs_thread_set_error(tp,GS_EXXX,
			"cannot remove data file, %s",strerror(errno));
		*/
		return GS_ESYS;
	}
	snprintf(buf,sizeof(buf),"topicmaps/%s/locpfxtree",name);

	/* check if user has permissions to destroy topicmap */

	if( unlink(buf) < 0)
	{
		TMTRACE(TM_STORAGE_TRACE,"unlink error: %s\n" _ strerror(errno));
		/*
		gs_thread_set_error(tp,GS_EXXX,
			"cannot remove lpt file, %s",strerror(errno));
		*/
		return GS_ESYS;
	}

	snprintf(buf,sizeof(buf),"topicmaps/%s",name);

	/* check if user has permissions to destroy topicmap */

	if( rmdir(buf) < 0)
	{
		TMTRACE(TM_STORAGE_TRACE,"rmdir error: %s\n" _ strerror(errno));
		/*
		gs_thread_set_error(tp,GS_EXXX,"cannot destroy %s, %s" ,
			name, strerror(errno));
		*/
		return GS_ESYS;
	}
	return GS_OK;
}


#endif /* if 0 */

#ifdef TEST
 
#include "test.h"
 
int main(int argc, char **args)
{
        GS_TEST_T test;
	GS_FILE_T file;
	byte p[GS_PAGE_SIZE];
	int created = 0;
	TMError e;
 
        test = gs_test_new(__FILE__);

	e = gs_storage_open_file("/xxtmp/testfile", NULL, 20000, &file, &created);
	GS_TEST(test, e == GS_ESYS);
	/*
	printf("errno=%s\n", strerror(errno));
	*/
	e = gs_storage_open_file("/tmp/testfile", NULL, 20000, &file, &created);
	GS_TEST(test, e == GS_OK);

	_readpage(file->f,p,1);
	GS_TEST(test, gs_page_get_number(p) == 1);

	

	/*
        GS_TEST(test, gs_page_get_number(p) == 0);

	*/

	gs_storage_close_file(file); 
 
        gs_test_delete(&test);
 
        return 0;
}
 
 
#endif

const char *_set_error(TMFile self, const char *fmt,...)
{
        va_list args;
        assert(self);
        assert(fmt);
        va_start(args,fmt);
        vsnprintf(self->errstr,sizeof(self->errstr),fmt,args);
        va_end(args);
        return (self->errstr);
}


TMError _readpage(TMFile self ,TMPage p, int n)
{
	off_t offset;
	assert(self);
	assert(p);
	assert(n > 0);
	
	TMTRACE(TM_STORAGE_TRACE,"  _readpage(): n=%d\n" _ n);

	offset = TM_PAGE_SIZE * (n-1); 
	if( lseek(self->fd,offset,SEEK_SET) != offset)
	{
		_set_error(self,"cannot seek file %s to %ld, %s",
			self->path, offset, strerror(errno));
		return TM_ESYS;
	}

	if( read(self->fd,p,TM_PAGE_SIZE) != TM_PAGE_SIZE)
	{
		_set_error(self,"reading file %s failed, %s",
			self->path, strerror(errno));
		return TM_ESYS;
	}

	/* check the returned page number */
	assert(gs_page_get_number(p) == n);

	return TM_OK;
}
TMError _writepage(TMFile self,TMPage p)
{
	off_t offset;
	int n;

	assert(self);
	assert(p);

	n = gs_page_get_number(p);
	TMTRACE(TM_STORAGE_TRACE,"  _writepage() enter n=%d\n" _ n );

	tm_page_clear_changedflag(p);

	offset = TM_PAGE_SIZE * (n-1); 
	if(lseek(self->fd,offset,SEEK_SET) != offset)
	{
		_set_error(self,"cannot seek file %s to %ld, %s",
			self->path, offset, strerror(errno));
		return TM_ESYS;
	}

	if(write(self->fd,p,GS_PAGE_SIZE) != GS_PAGE_SIZE)
	{
		_set_error(self,"writng to file %s failed, %s",
			self->path, strerror(errno));
		return TM_ESYS;
	}

	return TM_OK;
}

