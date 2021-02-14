/*
 * $Id: storage.c,v 1.4 2002/09/11 22:03:36 jan Exp $
 *
 * Copyright (c) 2002 Jan Algermissen
 * See the file "COPYING" for copying permission.
 *
 */

#include "storage.h"
#include <tmutil.h>
#include <tmmem.h>
#include <tmtrace.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define MIN_CACHE_SIZE (GS_PAGE_SIZE * 100)  

#include <sys/stat.h>

#define TM_MAX_PATH_LENGTH 1024

/* the cache consists of these items */
/* callers are provided with a pointer into the page member */
struct page_item {
	GS_PAGE_T page;
	int usecount;	
	byte lock;
	/* timestamp? */
};

	

struct gs_file_t {
	int f;
	char path[TM_MAX_PATH_LENGTH];
	struct {
		struct page_item *page_items;		
		int pcnt;
		int pmax;
	} cache;
};

static TMError _readpage(int f,GS_PAGE_T p, int n);
static TMError _writepage(int f,GS_PAGE_T p);


TMError gs_storage_open_file(const char *path, GS_INITFUNC_T initfunc,
			long cache_size, GS_FILE_T *filep, int *created_flag)
{
	GS_FILE_T file;
	char empty[GS_PAGE_SIZE];
	GS_PAGE_T p;
	off_t n;
	int i;
	struct stat stat_buf;
	p = empty;
	TMTRACE(TM_STORAGE_TRACE,"gs_storage_open_file(): enter path=%s\n" _
		path);

	assert(filep);
	assert(created_flag);

	GS_NEW(file);
	if(!file)
		return TM_ESYS;

	strncpy(file->path,path,sizeof(file->path));

	*filep = NULL;	
	*created_flag = 0;

	if(cache_size < MIN_CACHE_SIZE)
		cache_size = MIN_CACHE_SIZE;

	if(stat(path,&stat_buf) < 0)
	{
		TMTRACE(TM_STORAGE_TRACE,"cannot stat %s, (%s) opening with CREAT\n" _
			path _ strerror(errno) );
	
		if( (file->f = open(path,O_RDWR | O_CREAT, S_IRUSR | S_IWUSR)) < 0)
		{
			TMTRACE(TM_STORAGE_TRACE,"create error for %s, %s\n" _
				path _ strerror(errno));
			return TM_ESYS;
		}
		TMTRACE(TM_STORAGE_TRACE,"created %s\n" _ path);
		*created_flag = 1;
	}

	if( (file->f = open(path,O_RDWR, S_IRUSR | S_IWUSR)) < 0)
        {
		assert(0);
		TMTRACE(TM_STORAGE_TRACE,"(1) open error for %s, %s\n" _
				path _ strerror(errno));
		if(errno != EEXIST)
		{
			TMTRACE(TM_STORAGE_TRACE,"open error for %s, %s\n" _
				path _ strerror(errno));
			return TM_ESYS;
		}
		TMTRACE(TM_STORAGE_TRACE,"open error was %s, now creating\n" _
			strerror(errno));
	
		if( (file->f = open(path,O_RDWR | O_CREAT, S_IRUSR | S_IWUSR)) < 0)
		{
			TMTRACE(TM_STORAGE_TRACE,"create error for %s, %s\n" _
				path _ strerror(errno));
			return TM_ESYS;
		}
		TMTRACE(TM_STORAGE_TRACE,"created %s\n" _ path);
		*created_flag = 1;
        }


	assert(sizeof(int) == 4);
	assert(sizeof(off_t) == 4);

	TMTRACE(TM_STORAGE_TRACE,"open: f = %d\n" _ file->f);
	n = lseek(file->f,0,SEEK_END);
	if(n < 0)
	{
		fprintf(stderr,"%s\n",strerror(errno));
		assert(0); /* TBD */
	}

	if(*created_flag) { ; assert(n == 0); }
	if(n == 0) {
		if(initfunc)
			initfunc(p,1);
		else
			gs_page_init(p,1);

		write(file->f,p,GS_PAGE_SIZE);
		/* TBD: check ! */
	}

	file->cache.pmax = cache_size / GS_PAGE_SIZE;

	file->cache.page_items = (struct page_item*)malloc(
			file->cache.pmax * sizeof(struct page_item) );	

	assert(file->cache.page_items);

	for(i = 0; i < file->cache.pmax; i++)
	{
		file->cache.page_items[i].page = (GS_PAGE_T)malloc(
						GS_PAGE_SIZE);	
		assert(file->cache.page_items[i].page);
		file->cache.page_items[i].usecount = 0;
		file->cache.page_items[i].lock = 0;
	}
	file->cache.pcnt = 0;

	*filep = file;
	TMTRACE(TM_STORAGE_TRACE,"gs_storage_open_file(): exit path=%s\n" _
		path);

	return TM_OK;
}
TMError gs_storage_close_file(GS_FILE_T file)
{
	int i;
	assert(file);
	for(i = 0; i < file->cache.pcnt; i++)
	{
		if(gs_page_get_changedflag(file->cache.page_items[i].page))
		{
			assert(0);
			_writepage(file->f,file->cache.page_items[i].page);
		}
	}

	close(file->f);
	
	for(i = 0; i < file->cache.pmax; i++)
	{
		free(file->cache.page_items[i].page);
	}
	free(file->cache.page_items);
	GS_FREE(file);
	return TM_OK;
}




TMError gs_storage_flush_cache(GS_FILE_T file)
{
	int i;
	assert(file);
	
	for(i = 0; i < file->cache.pcnt; i++)
	{
			_writepage(file->f,file->cache.page_items[i].page);
		/*
		if(gs_page_get_changedflag(file->cache.page_items[i].page))
		{
			_writepage(file->f,file->cache.page_items[i].page);
		}
		*/
		/* TBD error check */
	}
	return TM_OK;
}


TMError gs_storage_new_page(GS_FILE_T file,GS_INITFUNC_T initfunc,int *np)
{
	off_t offset;
	int n;
	char empty[GS_PAGE_SIZE];
	GS_PAGE_T p;
	TMTRACE(TM_STORAGE_TRACE,"gs_storage_new_page(): enter\n");

	p = empty;

	assert(file);
	
	assert(sizeof(int) == 4);
	assert(sizeof(off_t) == 4);

	offset = lseek(file->f,0,SEEK_END);
	if(offset < 0)
		fprintf(stderr,"%s\n",strerror(errno));
	TMTRACE(TM_STORAGE_TRACE,"gs_storage_new_page(): end of last page: %ld\n" _ offset);

	n = ( (int)offset / GS_PAGE_SIZE ) + 1;
	TMTRACE(TM_STORAGE_TRACE,"gs_storage_new_page(): number is %d\n" _ n);
	initfunc(p,n);
	TMTRACE(TM_STORAGE_TRACE,"gs_storage_new_page(): called initfunc\n");

	write(file->f,p,GS_PAGE_SIZE);
	*np = n;
	TMTRACE(TM_STORAGE_TRACE,"gs_storage_new_page(): exit with (n=%d)\n" _ *np);
	return TM_OK;
}

	


TMError gs_storage_fetch_page(GS_FILE_T file,GS_PAGE_T *bp, int n)
{
	int i;
	

	assert(file);
	assert(bp);

	TMTRACE(TM_STORAGE_TRACE,"gs_storage_fetch_page(): page %d of %s\n" _ n _ file->path);

	for (i=0; i<file->cache.pcnt; i++)
	{
		if(gs_page_get_number(file->cache.page_items[i].page) == n)
		{
			file->cache.page_items[i].usecount++;
			/*  TBD ???
			assert(file->cache.page_items[ptr->i].lock == 0);
			*/
			/* XXXX */
			/*
			_readpage(file->f,file->cache.page_items[i].page,n);
			*/
			file->cache.page_items[i].lock = 1;
			TMTRACE(TM_STORAGE_TRACE,"locking page %d\n" _ n );
			/* set callers page pointer to the cached page */
			*bp = file->cache.page_items[i].page;
			return TM_OK;
		}
	}

	/* not found */
	

	/* if we still have room in the cache.... */
	if(file->cache.pcnt < file->cache.pmax)
	{
		i = file->cache.pcnt;

		_readpage(file->f,file->cache.page_items[i].page,n);
		/* we should initialize cache !
		assert(file->cache.page_items[i].usecount == 0);
		*/
		file->cache.page_items[i].usecount = 1;
		file->cache.page_items[i].lock = 1;
		TMTRACE(TM_STORAGE_TRACE,"locking page %d\n" _ n );
		*bp = file->cache.page_items[i].page;
		file->cache.pcnt++;
		return TM_OK;
	}


/*
 * ==================================================
 * HERE MUST BE THE FASTEST ALOGORITHM !!!
 * ==================================================
 *
 */

	for(i = 0; i < file->cache.pmax; i++)
	{
		if(!file->cache.page_items[i].lock && file->cache.page_items[i].usecount == 0)
		{
			assert(file->cache.page_items[i].usecount == 0);
			break;
		}
	}

	assert(i < file->cache.pmax); /* or cache too small !! */

	assert(file->cache.page_items[i].usecount == 0);
	assert(file->cache.page_items[i].lock == 0);
	if(gs_page_get_changedflag(file->cache.page_items[i].page))
	{
		_writepage(file->f,file->cache.page_items[i].page);
	}

	_readpage(file->f,file->cache.page_items[i].page,n);

	assert(gs_page_get_number(file->cache.page_items[i].page) == n);
		
	file->cache.page_items[i].usecount = 1;
	file->cache.page_items[i].lock = 1;
	TMTRACE(TM_STORAGE_TRACE,"locking page %d\n" _ n);
	*bp = file->cache.page_items[i].page;


	return TM_OK;
}


TMError gs_storage_drop_page(GS_FILE_T file,GS_PAGE_T page)
{
	int i;

	assert(file);
	assert(page);

	TMTRACE(TM_STORAGE_TRACE,"gs_storage_drop_page(): page %d of %s\n" _ gs_page_get_number(page) _ file->path);
	for(i = 0; i < file->cache.pmax; i++)
	{
		/* FIXME: why was here a '!' ??? */
		if(file->cache.page_items[i].page == page)
		{
			file->cache.page_items[i].usecount--;
			if(file->cache.page_items[i].usecount == 0) {
			file->cache.page_items[i].lock = 0;
			}
			/* XXXX */
			/*
			_writepage(file->f,file->cache.page_items[i].page);
			*/
			return TM_OK;
		}
	}

	return TM_OK;
}




/* ==========================================================================*/


TMError _readpage(int f,GS_PAGE_T p, int n)
{
	off_t offset;
	assert(f);
	assert(p);
	assert(n > 0);
	
	TMTRACE(TM_STORAGE_TRACE,"  _readpage(): page %d\n" _ n);

	offset = GS_PAGE_SIZE * (n-1); 
	if( lseek(f,offset,SEEK_SET) != offset)
	{
		assert(0);
		fprintf(stderr,"cannot lseek to %ld, %s\n",offset,strerror(errno));
		return TM_ESYS;
	}

	if( read(f,p,GS_PAGE_SIZE) != GS_PAGE_SIZE)
	{
		TMTRACE(TM_STORAGE_TRACE,"read: f = %d\n" _ f);
		TMTRACE(TM_STORAGE_TRACE,"cannot read page at offset %d, %s\n"
			_ offset _ strerror(errno));
		/* TBD: remove assertion */
		fprintf(stderr,"cannot read %d bytes at %ld, %s\n",GS_PAGE_SIZE,offset,strerror(errno));
		 assert(0);
		return TM_ESYS;
	}

	/* check the returned page number */
	assert(gs_page_get_number(p) == n);
	return TM_OK;
}
TMError _writepage(int f,GS_PAGE_T p)
{
	off_t offset;
	int n;

	assert(f);
	assert(p);
	TMTRACE(TM_STORAGE_TRACE,"  _writepage() enter\n");

	gs_page_set_changedflag(p,0);
	n = gs_page_get_number(p);

	offset = GS_PAGE_SIZE * (n-1); 
	if(lseek(f,offset,SEEK_SET) != offset)
	{
		assert(0);
		return TM_ESYS;
	}

	if(write(f,p,GS_PAGE_SIZE) != GS_PAGE_SIZE)
	{
		assert(0);
		return TM_ESYS;
	}

	return TM_OK;
}



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
		return TM_ESYS;
	}
	if( chmod(name,S_IRUSR | S_IWUSR | S_IXUSR) )
	{
		/* FIXME: unlink directory ?? */
		TMTRACE(TM_STORAGE_TRACE,"cannot chmod on %s, %s\n" _
			buf _ strerror(errno));
		return TM_ESYS;
	}
	while(files && files[i] != NULL)
	{
		snprintf(buf,sizeof(buf),"%s/%s",name,files[i]);
		if( (fd = open(buf,O_RDWR | O_CREAT, S_IRUSR | S_IWUSR)) < 0)
		{
			/* FIXME: unlink directory ?? */
			TMTRACE(TM_STORAGE_TRACE,"cannot open %s, %s\n" _
				buf _ strerror(errno) );
			return TM_ESYS;
		}
		i++;
	}

	return TM_OK;
}

#if 0
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
		return TM_ESYS;
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
		return TM_ESYS;
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
		return TM_ESYS;
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
		return TM_ESYS;
	}
	return TM_OK;
}

#endif

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
	GS_TEST(test, e == TM_ESYS);
	/*
	printf("errno=%s\n", strerror(errno));
	*/
	e = gs_storage_open_file("/tmp/testfile", NULL, 20000, &file, &created);
	GS_TEST(test, e == TM_OK);

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

