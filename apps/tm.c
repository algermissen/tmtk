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
#include <ctype.h>

#define PROMPT "tm>"
 
enum {
	ENC_NONE,
	ENC_RAST,
	ENC_TXT,
	ENC_RDF_DC,
	ENC_XTM,
	ENC_XML,
	ENC_HTML
};
static int parse_only           = 0;
static int silent               = 0;
static TMTopicMap topicmap      = NULL;

	/* default encoding is txt */
static int xencoding            = ENC_TXT;
static int comments_in_output	= 1;

static char* base_uri           = NULL;
static char *parse_mode         = "xml";
static int read_stdin           = 0;
static char *query              = NULL;
static int interactive          = 0;




#define MAX_MAPS 	256
#define PARSE_BUF_SIZE 	4096

static TM tm;	

static void tk_set_progname(const char*n);
static void tk_err_exit(const char *fmt,...);
static void qhelp(void);
static void prompt(TMTopicMap topicmap);

static void print_stats(TMTopicMap tm,FILE *f);

static const void* _get_attr(void **atts,const char *name,TMValueType *vp);

typedef struct view_data t_view_data;

static int _find_view_handler(t_view_data *vdp,const char *name);
typedef void(*ViewStartHandler)(t_view_data *,const char *name, void **atts);
typedef void(*ViewEndHandler)(t_view_data *,const char *name);


struct view_data {
	TMTopicMap topicmap;
	FILE *file;
	int level;
	void *data;		/* for specialized TXT handlers */
	ViewStartHandler start;
	ViewEndHandler end;
};

static void hitlist_start(t_view_data *vdp,const char *name, void **atts)
{
	if(strcmp(name,"hitlist") == 0)
	{
		if(comments_in_output)
		{
			fprintf(vdp->file,"# ---------------------------------------\n");
			fprintf(vdp->file,"# Hitlist for %s\n", (char*)_get_attr(atts,"query",NULL) );
			fprintf(vdp->file,"# ---------------------------------------\n");
		}
	}
	else if(strcmp(name,"topic") == 0)
	{
		TMValueType vt;
		char buf[1024];
		const void *bn;
		bn = _get_attr(atts,"basenames",&vt);
		if(bn)
		{
			/*
			TMList lp;
			if(comments_in_output)
			{
				fprintf(vdp->file,"# %s\n" _ buf );
			}
			*/
			tm_value_to_string(vt,bn,buf,sizeof(buf));
			fprintf(vdp->file,"%s\n" _ buf );
			/*
			for(lp = (TMList)bn;lp;lp=lp->next)
			{
				fprintf(vdp->file,"%s\n" _ (char*)lp->content );
			}
			*/
		}
	}
	else
	{
		/*
		assert(0);
		*/
	}
	
}
static void hitlist_end(t_view_data *vdp,const char *name)
{
	if(strcmp(name,"hitlist") == 0)
	{
		;

	}
	else if(strcmp(name,"topic") == 0)
	{
		;
	}
	else
	{
		/*
		assert(0);
		*/
	}
}

static void index_start(t_view_data *vdp,const char *name, void **atts)
{
	/*
	fprintf(vdp->file,"INDEX: start: %s\n",name);
	*/
	if(strcmp(name,"index") == 0)
	{
		if(comments_in_output)
		{
			fprintf(vdp->file,"# ---------------------------------------\n");
			fprintf(vdp->file,"# Index size: %d\n",
				(int)_get_attr(atts,"count",NULL)
			);
			fprintf(vdp->file,"# List-of-basenames => URL (Title)\n");
			fprintf(vdp->file,"# ---------------------------------------\n");
		}
		vdp->data = TM_ALLOC(1024);

	}
	else if(strcmp(name,"entry") == 0)
	{
		TMValueType vt;
		const void *bn;
		bn = _get_attr(atts,"basenames",&vt);
		if(bn)
			tm_value_to_string(vt,bn,(char*)vdp->data,1024);
		else
			snprintf((char*)vdp->data,1024,"no-name");
	}
	else if(strcmp(name,"occurrence") == 0)
	{
		const void *adr, *title;
		adr = _get_attr(atts,"address",NULL);
		if(!adr)
			return;
		title = _get_attr(atts,"title",NULL);
		fprintf(vdp->file,"%s => %s ('%s')\n" , (char*)vdp->data, (char*)adr,
			title ? (char*)title : "no-title");
	}
	else
	{
		/*
		assert(0);
		*/
	}
	
}
static void index_end(t_view_data *vdp,const char *name)
{
	if(strcmp(name,"index") == 0)
	{
		TM_FREE(vdp->data);

	}
	else if(strcmp(name,"entry") == 0)
	{
		;
	}
	else if(strcmp(name,"occurrence") == 0)
	{
		;
	}
	else
	{
		/*
		assert(0);
		*/
	}
}

struct handler_tab {
	const char *view_name;
	ViewStartHandler start;
	ViewEndHandler end;
} handler_tab[] = {
	{ "index", index_start, index_end },
	{ "hitlist", hitlist_start, hitlist_end },
	{ NULL,NULL,NULL }
};


/* default callbacks */
static int vstart(void* user_data, const char *name, void **atts)
{
	int i;	
	struct view_data *vdp = (struct view_data*)user_data;
	switch(xencoding) {
	case ENC_TXT:
		/* set special TXT handlers if any */
		if( vdp->level == 0)
		{
			_find_view_handler(vdp,name);
		}

		if(vdp->start)
		{
			vdp->start(vdp,name,atts);
		}
		/* TXT default output */
		else
		{
			for(i=0;i<vdp->level;i++)
				fprintf(vdp->file,"  ");
			
			fprintf(vdp->file,"%s\n",name);
			for (i = 0; atts && atts[i]; i += 3)
       			{
				char buf[1024];
				int j;
				tm_value_to_string((TMValueType)atts[i+2],atts[i+1],buf,sizeof(buf));
				for(j=0;j<vdp->level;j++)
					fprintf(vdp->file,"  ");
				fprintf(vdp->file," %s: \"%s\"\n", (char*)atts[i],buf);
			}
			fprintf(vdp->file,"\n");
		}
		break;
	case ENC_RAST:
		fprintf(vdp->file,"(%s\n",name);
		for (i = 0; atts && atts[i]; i += 3)
       		{
			char buf[1024];
			tm_value_to_string((TMValueType)atts[i+2],atts[i+1],buf,sizeof(buf));
			fprintf(vdp->file,"A%s %s\n", (char*)atts[i],buf);
		}
		break;
	case ENC_XML:
		if(vdp->level == 0)
			fprintf(vdp->file,"<?xml version=\"1.0\" encoding=\"utf-8\"?>\n");
		for(i=0;i<vdp->level;i++)
			fprintf(vdp->file,"  ");
			
		fprintf(vdp->file,"<%s",name);
		for (i = 0; atts && atts[i]; i += 3)
       		{
			char buf[1024];
			tm_value_to_string((TMValueType)atts[i+2],atts[i+1],buf,sizeof(buf));
			fprintf(vdp->file," %s=\"%s\"", (char*)atts[i],buf);
		}
		fprintf(vdp->file,">\n");
		break;
	}	
	vdp->level++;
	return (0);
}
static int vend(void* user_data, const char *name)
{
	int i;
	struct view_data *vdp = (struct view_data*)user_data;
	vdp->level--;
	switch(xencoding) {
	case ENC_TXT:
		if(vdp->end)
		{
			vdp->end(vdp,name);
		}
		else
		{
			; /* no default action for TXT type */
		}
		break;
	case ENC_RAST:
		fprintf(vdp->file,")%s\n",name);
		break;
	case ENC_XML:
		for(i=0;i<vdp->level;i++)
			fprintf(vdp->file,"  ");
		fprintf(vdp->file,"</%s>\n",name);
		break;
	}	
	return (0);
}


#ifndef NDEBUG
static char vstr[] = "Version 0.7 (Debug)";
#else
static char vstr[] = "Version 0.7";
#endif

void usage(void)
{
	printf("Usage: tm [-hsvIPLQd] "
	       "[-q query]"
	       "[-p processing model for processing] [-A TMAs to load] "
	       "[-b base-uri] [-i input-syntax] "
#ifndef NDEBUG
	       "[-t tracemask] "
#endif
		"[files...]\n");
}

void version(void)
{
	printf("tm: %s\n\n", vstr);
}
void list_apps(void)
{
	printf("\n\n");
	printf("tm: list of avalable Topic Map Applications\n\n");
	printf("    \n");
	printf("    IS13250\n");
	printf("    sam\n");
	printf("    sx\n");
	printf("    dc\n");
	printf("    rss\n");
	printf("    \n");
	printf("    Please use tmainfo for details about the applications:\n\n");
	printf("      tmainfo appname\n\n");
}
void list_pms(void)
{
	printf("\n\n");
	printf("tm: list of avalable Topic Map Processing Models\n\n");
	printf("    \n");
	printf("    xtm_simple:       a simple PM for XTM (uses TMA 'SAM') \n");
	printf("    rdf_common:       can handle some of the common RDF statements\n");
	printf("                      such as Dublin Core and RSS (uses 'DC' semantics)\n");
	printf("    iso2788:          used for processing  ISO2788 thesauri\n");
	printf("    iso2788namesonly: processes only descriptors of ISO2788 thesauri\n");
}

/*
static void _export(TMTopicMap topicmap, FILE *f);
static void _index(TMTopicMap topicmap, FILE *f, int format);
static void _index_html(TMTopicMap topicmap, FILE *f);
static void _rindex(TMTopicMap topicmap, FILE *f, int format);
static void _namelist1(TMTopicMap topicmap, FILE *f, int format);
static void _iso2788_descriptors(TMTopicMap topicmap, FILE *f, int format);
static void _complete_xtm(TMTopicMap topicmap, FILE *f);
*/
static void _complete(TMTopicMap topicmap, FILE *f, int format);
/*
static void _complete_xml(TMTopicMap topicmap, FILE *f);
*/

void help(void)
{
	printf("tm: topic map processing tool\n\n");
	usage();

	printf("\nOptions:\n");
	printf("    -h              show this screen\n");
	printf("    -F framework    the name of the disclosure framework to use (defaults to 'www-df')\n");
	printf("    -S storage      the name of the storage implementation to use (defaults to 'MemStore')\n");
	printf("    -i input-type   input is of parse type 'input-type'. This option\n");
	printf("                    is used to choose which parser is to be used.\n");
	printf("                    Recognized types are:\n");
#ifdef HAVE_LIBEXPAT
	printf("                    xml (default) (parser is expat)\n");
#endif
#ifdef HAVE_LIBRAPTOR
	printf("                    rdfxml (parser is raptor)\n");
	printf("                    rdfntriples (parser is raptor) (not implemented)\n");
#endif
	printf("                    lines (build in line-oriented parser\n");
	printf("    -I              read input from stdin\n");
	printf("    -p procmodel    the name of the processing model to use for processing.\n");
	printf("                    This will be used for all given maps.\n");
	printf("    -a argstring    argument to pass to processing model\n");
	printf("    -A app-list     additional Applications to be loaded into topic map\n");
	printf("                    app-list is a comma seperated list of application names\n"); 
	printf("    -o format       use 'format' for the output\n");
	printf("                    Recognized formats are:\n");
	printf("                    txt:       ASCII output\n");
	printf("                    plain:     Plain ASCII output (without comments)\n");
	printf("                    xml:       XML (still needs fixes)\n");
	printf("                    rast:      RAST (still needs some fixes)\n");
/*
	printf("                    html:      HTML\n");
	printf("                    rdfdc:     RDF using Dublin Core namespace\n");
	printf("                    xtm:       XTM\n");
*/
	printf("    -b base-uri     use base-uri base URI\n");
	printf("    -P              only process maps, don't construct topicmap\n");
	printf("    -Q              launch interactive query interface\n");
	printf("    -q query        specify a query to be issued\n");
	printf("    -L loglevel     set loglevel to be used ('debug','info','warn','error' or 'critical')\n");
	printf("    -v              show version information and exit\n");
	printf("    -s              silent mode (no progression bar)\n");
	printf("    -M              do not fully merge the topic map\n");
	printf("    -d              dump topic map\n");
/*
#ifndef NDEBUG
	printf("    -t mask         enable trace output (for debugging) \n");
	printf("                    Recognized flags are:\n");
	printf("                    *: full trace  (use -t \"*\" !)\n");
	printf("                    g: topicmap component\n");
	printf("                    o: omnivore (processing)\n");
	printf("                    s: storage\n");
	printf("                    p: syntax processing model\n");
	printf("                    v: value type functions\n");
	printf("                    a: user application\n");
	printf("                    P: other parsing components (e.g. TMADL parser)\n");
#endif
*/
	/*
	printf("    -m              list available Topic Map Processing Models\n");
	printf("    -l              list available Topic Map Applications\n");
	printf("                \n");
	printf("                \n");
	*/
	printf("                \n");
	printf("                \n");
#if 0
	printf("tm is TmTk's commandline tool for working with topic maps.\n");
	printf("\n");
	printf("Loading data into the topic map:\n");
	printf("  tm takes a list of files as commandline arguments and \n");
	printf("  will apply the processing model specified with the -p option\n");
	printf("  to all the input files. tm -m will list the available\n");
	printf("  processing models with a short description\n");
	printf("  When given the -I option, tm will read data from STDIN.\n");
	printf("\n");
	printf("  Depending on how tm has been built, several parsers are\n");
	printf("  available and the -i option will tell tm which parser to\n");
	printf("  invoke for the input data.\n");
	printf("\n");
	printf("\n");
	printf("Querying the topic map:\n");
	printf("  With the -q option, a query can be executed the result of which\n");
	printf("  will be in the output type that has been specified with the -e\n");
	printf("  option. See above for a list of output types.\n");
	printf("  The option -Q will launch tm's interactive query interface.\n");
	printf("\n");
	printf("Please check http://www.gooseworks.org for extended documentation.\n");
	printf("\n");
	printf("\n");
	printf("tm is a flexible tool for topic map processing. Its primary uses are:\n");
	printf("  - doing a quick parse check for a topic map (use the -P option)\n");
	printf("  - investigate a topic map quickliy by displaying it in several formats\n");
	printf("  - loading a topic map into persistent storage (not until tmtk 1.0)\n");
	printf("  - transforming a topic map to another syntax (propably lossy)\n");
	printf("  - exracting portions of a topic map in formats that can be used for\n");
	printf("    subsequent information retrieval tasks such as query expasnion,\n");
	printf("    text mining, etc.\n");
	/*
	printf("\n");
	printf("\n");
	printf("\n");

	printf("\n");
	*/
	printf("\n");

	/*
	printf("    -u uri      specify URI of a map to import\n");
	printf("    -t type     type of input data\n");
	printf("    -f file     file to open\n");
	printf("                http://www.proxy.com:8080  \n");
	printf("    -e          enable subjectEquivalence callback\n");
	printf("    -u uri      specify URI of a map to import\n");
	printf("    -a          enable association callback\n");
	printf("    -o          export the topicmap as XTM\n");
	*/
#endif
	exit(0);
}

/*
static TMError merge_map_handler(Omnivore ov, const char* loc);
static TMError handler(Omnivore ov,TMAssertion assertion);
static TMError built_in_topic_handler(Omnivore ov, TMSubject subject);
static TMError add_property_handler(Omnivore ov, TMSubject subject, const char *model, const char *prop, const void *value);
*/



int main(int argc, char **args)
{
	const char *df_name = NULL;
	const char *storage_name = NULL;
	const char *loglevel = NULL;

	char *topicmap_name = NULL;
	char *maps[MAX_MAPS];
	int i,map_cnt = 0;
	char cwd_buf[256];
	int dump = 0;
	int fully_merge = 1;
	const char *config_file = NULL;
	/*
	TMError e;
	*/
	char *pm_arg = NULL;
	TMList  lp,add_model_list = NULL;
	/*
	TMModel model;
	TMProcModel proc_model = NULL;
	*/
	char *pm_name = NULL;

	char option;



	tk_set_progname(args[0]);

	if( getcwd(cwd_buf,sizeof(cwd_buf)) == NULL)
		tk_err_exit("unable to get cwd, %s\n",strerror(errno));
	
	opterr = 0;
	
	while( (option = getopt(argc,args, "-QIhMsvPdL:F:S:t:C:a:i:o:a:A:b:p:q:")) != EOF)
	{
		switch(option) {
		case 'h':
			help();
			exit(0);
			break;
		case 'F':
			df_name = tm_strdup(optarg);
			break;
		case 'S':
			storage_name = tm_strdup(optarg);
			break;
		case 'I':
			read_stdin = 1;
			break;
		case 'd':
			dump = 1;
			break;
		case 'P':
			parse_only = 1;
			break;
		case 's':
			silent = 1;
			break;
		case 'L':
			loglevel = tm_strdup(optarg);
			break;
		case 'v':
			version();
			exit(0);
		case 't':
			tm_set_trace_mask(optarg);	
			break;
		case 'b':
			base_uri = tm_strdup(optarg);
			break;
		case 'M':
			fully_merge = 0;
			break;
		case 'q':
			query = tm_strdup(optarg);
			break;
		case 'C':
			config_file = tm_strdup(optarg);
			break;
		case 'a':
			pm_arg = tm_strdup(optarg);
			break;
		case 'Q':
			interactive = 1;
			break;
		case 'i':
			parse_mode = tm_strdup(optarg);
			break;
		case 'p':
			pm_name = tm_strdup(optarg);
			break;
		case 'A': 
			add_model_list = tm_list_push(add_model_list,tm_strdup(optarg));
			break; 
		case 'o':
			if(strcmp(optarg,"plain") == 0)
			{
				comments_in_output = 0;
				xencoding = ENC_TXT;
			}
			else if(strcmp(optarg,"txt") == 0)
			{
				comments_in_output = 1;
				xencoding = ENC_TXT;
			}
			else if(strcmp(optarg,"rast") == 0)
				xencoding = ENC_RAST;
			else if(strcmp(optarg,"rdfdc") == 0)
				xencoding = ENC_RDF_DC;
			else if(strcmp(optarg,"xtm") == 0)
				xencoding = ENC_XTM;
			else if(strcmp(optarg,"xml") == 0)
				xencoding = ENC_XML;
			else if(strcmp(optarg,"html") == 0)
				xencoding = ENC_HTML;
			else
				tk_err_exit("unknow output encoding %s",optarg);
			break;
		case 'g':
			if(topicmap_name)
			{
				usage();
				exit(1);
			}
			if( (topicmap_name = (char*)malloc(strlen(optarg)+1)) == NULL)
				tk_err_exit("out of memory");
			strcpy(topicmap_name,(const char*)optarg);	
			break;
		case   1:
			if(map_cnt >= MAX_MAPS)
				tk_err_exit("maximum number of topic maps "
					    "exceeded\n");

			if( tm_uri_is_abs(optarg))
			{
				maps[map_cnt]=(char*)malloc(strlen(optarg)+1);
				if( ! maps[map_cnt])
					tk_err_exit("out of memory");
				strcpy(maps[map_cnt],optarg);
			}
			else
			{
				maps[map_cnt]=(char*)malloc(
					strlen(cwd_buf)+1+strlen(optarg)+1);
				if( ! maps[map_cnt])
					tk_err_exit("out of memory");
				sprintf(maps[map_cnt],"%s/%s",cwd_buf,optarg);
			}
			map_cnt++;
			break;
		case '?':
			usage(); 
			exit(1);
		}
	}


	if(config_file)
	{
		assert(0);
	}
	
	if(df_name == NULL)
	{
		df_name = tm_strdup("www-df");
	}

	if(tm_init(&tm,df_name,NULL) != TM_OK)
        {
                fprintf(stderr,"ERROR: %s\n",tm_get_error(tm));
		exit(1);
        }

	if( loglevel == NULL )
		tm_set_loglevel(tm,TM_LOG_WARNING);
	else if( strcmp(loglevel,"debug") == 0)
		tm_set_loglevel(tm,TM_LOG_DEBUG);
	else if( strcmp(loglevel,"info") == 0)
		tm_set_loglevel(tm,TM_LOG_INFO);
	else if( strcmp(loglevel,"warning") == 0)
		tm_set_loglevel(tm,TM_LOG_WARNING);
	else if( strcmp(loglevel,"error") == 0)
		tm_set_loglevel(tm,TM_LOG_ERROR);
	else if( strcmp(loglevel,"critical") == 0)
		tm_set_loglevel(tm,TM_LOG_CRITICAL);
	else
		tk_err_exit("unknown log level %s" , loglevel);
	

	if(storage_name == NULL)
	{
		storage_name = tm_strdup("MemStore");
	}




	if(! parse_only) 
	{
		/* FIXME: the follwoing stuff is actually stuff to choose the storage type!! */
		if(topicmap_name == NULL)
		{
			topicmap = tm_topicmap_new(tm);
			if( tm_topicmap_set_storage(topicmap,storage_name) != TM_OK)
				tk_err_exit("unable to set storage, %s", tm_get_error(tm) );
		} 
		else
		{
			/*
			topicmap = tm_topicmap_new(tm,&default_file_storage_descriptor);
			*/
			tk_err_exit("named topicmaps not supported now");
		}


		if( tm_topicmap_open(topicmap,topicmap_name) != TM_OK)
			tk_err_exit("unable to open topicmap %s, %s\n",
				topicmap_name, tm_get_error(tm) );

		/* -- old code (when model was specifed via commandline
		if(tm_topicmap_require_model(topicmap,model) != TM_OK)
			tk_err_exit("unable to open topicmap %s, %s\n",
				topicmap_name, tm_get_error(tm) );
		*/

		for(lp= add_model_list;lp;lp=lp->next)
		{
			if(tm_topicmap_require_model(topicmap,(char*)lp->content) != TM_OK)
				tk_err_exit("unable to load model %s, %s\n",
					(char*)lp->content, tm_get_error(tm) );
	
		}
		/*
		if( tm_topicmap_fully_merge(topicmap) != TM_OK)
			tk_err_exit("unable to merge topicmap %s\n",
				tm_get_error(tm) );
		*/
		/*	
		if(do_log)
			tm_topicmap_set_log_function(topicmap,tm_log);
		*/

	}

	if(!pm_name && (read_stdin || map_cnt > 0) )
	{
		pm_name = tm_strdup("xtm_simple");
	}
	/*
		tk_err_exit("cannot process input: no processing model specified\n");
	*/




	if(! read_stdin)
	{

	for(i=0;i<map_cnt;i++) {
		Omnivore o;
		int fd;
		double file_size;
		double percent_save = 0;
		double percent_done = 0;
		double percent_inc = 2;		/* 100% / 50 char bar len = 2 % increment */
		struct stat stbuf;
		int N  = 50;

		

		if(strstr(maps[i],"://") != NULL) {
			fprintf(stderr,"skipping %s\nonly file names are "
			   	       "currently supported\n",maps[i]);
			continue;
		}
		if( (fd = open(maps[i],O_RDONLY,0)) < 0)
			tk_err_exit("cannot open %s, %s\n",
				maps[i], strerror(errno));
	
		if( fstat(fd,&stbuf) < 0)
			tk_err_exit("cannot stat %s, %s\n",
				maps[i], strerror(errno));

		file_size = stbuf.st_size;


		o = Omnivore_new(tm);
		if(!o)
		{
			fprintf(stderr,"skipping %s\n"
					"%s\n",maps[i],tm_get_error(tm));
			continue;
		}
		if( tm_omnivore_prepare(o,parse_mode,pm_name,pm_arg,topicmap) != TM_OK)
			tk_err_exit("cannot prepare omnivore, %s\n",
				tm_get_error(tm));


		/* FIXME: allow combination of both*/
		if(!base_uri)
			Omnivore_setDocUri(o,maps[i]);
		else
			Omnivore_setDocUri(o,base_uri);


		/*
		if(do_log)
			tm_omnivore_set_log_function(o,tm_log);
		*/

		if(! parse_only)
		{
			/*
			Omnivore_setAssertionHandler(o,handler);
			Omnivore_setMergeMapHandler(o,merge_map_handler);
			Omnivore_setBuiltInNodeHandler(o,built_in_topic_handler);
			Omnivore_setAddPropertyHandler(o,add_property_handler);
			*/
		}

		if(! silent)
		{
			printf("%s:\n",maps[i]);
			printf("|%50s|\r|"," ");
			fflush(stdout);                    
		}


		percent_save = percent_done;
		if(! parse_only)
			tm_topicmap_start_transaction(topicmap);
		for (;;)
		{
			char buf[PARSE_BUF_SIZE];
			int len;
			double p;

			

			if( (len = read(fd, buf, sizeof(buf))) < 0)
				tk_err_exit("error reading %s, %s\n", maps[i], strerror(errno));

			p = (((double)len * (double)100) / (double)file_size);
			percent_done += p;
			if( (!silent) && (percent_done >= (percent_save + percent_inc)) ) {
				double diff,n;
				double i;
				diff = percent_done - (percent_save + percent_inc);
				n = diff / percent_inc;
				for(i=0;i<n;i++)
				{
					N--;
					printf("*");
				}
				fflush(stdout);                    
				
				percent_save = percent_done;
			
			}
		
			if (! Omnivore_parse(o, buf, len, len == 0))
			{
				tk_err_exit("Parse error: %s\n",
					tm_get_error(tm) );
			}
			if (len == 0)
				break;
		}
		if(! silent)
		{
			for(;N>0;N--)
				printf("*");
			fflush(stdout);                    
			printf("\n%.0f %% done\n",percent_done);
			fflush(stdout);                    
		}
		Omnivore_delete( &o );
		if(! parse_only)
		{
			if(fully_merge)
			{	
			if( tm_topicmap_fully_merge(topicmap) != TM_OK)
				tk_err_exit("after processing %s: unable to merge topicmap %s\n",
					maps[i],tm_get_error(tm) );
			}
			tm_topicmap_end_transaction(topicmap);
		}

	} /* while more maps */

	}

	else	/* read stdin */

	{
		Omnivore o;
		int fd;

		fd = 0;	/* stdin, FIXME */
	
		o = Omnivore_new(tm);
		if(!o)
		{
			fprintf(stderr,"cannot read STDIN, %s\n",tm_get_error(tm));
			/* FIXME cleanup mem */
			exit(1);
		}
		if( tm_omnivore_prepare(o,parse_mode,pm_name,pm_arg,topicmap) != TM_OK)
			tk_err_exit("cannot prepare omnivore, %s\n",
				tm_get_error(tm));
	

		if(base_uri)
			Omnivore_setDocUri(o,base_uri);

		/*
		if(do_log)
			tm_omnivore_set_log_function(o,tm_log);
		*/

		if(! parse_only)
		{
			/*
			Omnivore_setAssertionHandler(o,handler);
			Omnivore_setMergeMapHandler(o,merge_map_handler);
			Omnivore_setBuiltInNodeHandler(o,built_in_topic_handler);
			Omnivore_setAddPropertyHandler(o,add_property_handler);
			*/
		}

		if(! parse_only)
			tm_topicmap_start_transaction(topicmap);
		for (;;)
		{
			char buf[PARSE_BUF_SIZE];
			int len;
			

			if( (len = read(fd, buf, sizeof(buf))) < 0)
				tk_err_exit("error reading %s, %s\n", maps[i], strerror(errno));

			if (! Omnivore_parse(o, buf, len, len == 0))
			{
				tk_err_exit("Parse error: %s\n",
					tm_get_error(tm) );
			}
			if (len == 0)
				break;
		}

		Omnivore_delete( &o );
		if(! parse_only)
		{
			/*
			tm_topicmap_fully_merge(topicmap);
			*/
			if(fully_merge)
			{	
			tm_topicmap_end_transaction(topicmap);
			}
		}


	}


	if(! parse_only)
	{	
		if(interactive)
		{
			prompt(topicmap);
		}

		if(query != NULL)
		{
			struct view_data vdata;
                        bzero(&vdata,sizeof(vdata));
                        vdata.topicmap = topicmap;
                        vdata.file = stdout;
			/*	 
                        tm_topicmap_set_view_user_data(topicmap,&vdata);
                        tm_topicmap_set_view_pre_event_handler(topicmap,vpre);
			*/
			/*
			printf("%s\n",query);
			tm_topicmap_set_view_start_event_handler(topicmap,vstart);
			tm_topicmap_set_view_end_event_handler(topicmap,vend);
			tm_topicmap_set_view_user_data(topicmap,NULL);
			*/


			/*
			if( tm_topicmap_query(topicmap,&vdata,topic_start_html,topic_end_html,query) != TM_OK)
			*/
			if( tm_topicmap_query(topicmap,&vdata,vstart,vend,query) != TM_OK)
			{
				fprintf(stderr,"could not execute query '%s', %s\n",
				query, tm_get_error(tm) );
			}
		}
		/*
		TMError e;
		if( (e = init_topics(topicmap)) != TM_OK)
		{
			tm_topicmap_close(topicmap);
			tm_topicmap_delete(&topicmap);
			tk_err_exit("cannot initialize topics");
			
		}
		*/
		if(dump)
		{
			_complete(topicmap,stdout,xencoding);
		}
#if 0
		if(xformat != FMT_NONE)
		{
			switch(xformat) {
			case FMT_NAMELIST1: 
				_namelist1(topicmap,stdout,xencoding);
				break;
			case FMT_ISO2788_DESCRIPTORS: 
				_iso2788_descriptors(topicmap,stdout,xencoding);
				break;
			case FMT_INDEX: 
				if(xencoding == ENC_HTML)
					_index_html(topicmap,stdout);
				else
					_index(topicmap,stdout,xencoding);
				break;
			case FMT_RINDEX: 
				_rindex(topicmap,stdout,xencoding);
				break;
			case FMT_COMPLETE: 
				_complete(topicmap,stdout,xencoding);
				break;
			default:
				/* can't happen */
				assert(!"unknown xformat");
			}
		}
#endif
		/*
		_test(topicmap,stdout,xencoding);
		*/
		tm_topicmap_close(topicmap);
		tm_topicmap_delete(&topicmap);
	}
	tm_cleanup(&tm);
	return 0;
} 


void _complete(TMTopicMap topicmap, FILE *f, int encoding)
{
	switch(encoding) {
	case ENC_XTM:
		/*
		_complete_xtm(topicmap,f);
		*/
		break;
	case ENC_XML:
		/*
		_complete_xml(topicmap,f);
		*/
		tm_topicmap_xml_export(topicmap,f);
		break;
	case ENC_TXT:
		tm_topicmap_dump(topicmap,f);
		break;
	default:
		tk_err_exit("encoding not supported for 'complete'\n");
	}
}
	
#if 0
void _test(TMTopicMap topicmap, FILE *f, int encoding)
{
	TMTopicScan scan;
	TMTopic at_basenamed_basename_topic;
        TMTopic role_basenamed_topic;
        TMTopic role_basename_topic;
	TMTopic at_occurring_occurrence_topic;
        TMTopic role_occurring_topic;
        TMTopic role_occurrence_topic;
	TMModel m;
	
	TMProperty prop_descriptor;
	TMProperty prop_asidp;
	TMProperty prop_sbjadr;
	TMProperty prop_sbjdata;
	TMProperty prop_inddata;

	tmtk_default_model_lookup_function("ISO2788",&m);

	prop_descriptor = tm_model_get_property(m,"Descriptor");
	assert(prop_descriptor);
#if 0
	prop_sbjadr = tm_model_get_property(m,"SubjectAddress");
	prop_sbjdata = tm_model_get_property(m,"SubjectData");
	prop_inddata = tm_model_get_property(m,"IndicatorData");
	assert(prop_asidp);
	assert(prop_sbjadr);
	assert(prop_sbjdata);
	assert(prop_inddata);

	if( tm_topicmap_get_topic(topicmap,"IS13250","UniqueName","AtBaseNamedBaseName",
                                &(at_basenamed_basename_topic)) != TM_OK)
        {
                assert(0);
        }
        if( tm_topicmap_get_topic(topicmap,"IS13250","UniqueName","RoleBaseNamed",
                                &(role_basenamed_topic)) != TM_OK )
        {
                assert(0);
        }
        if( tm_topicmap_get_topic(topicmap,"IS13250","UniqueName","RoleBaseName",
                                &(role_basename_topic)) != TM_OK )
        {
                assert(0);
        }
	if( tm_topicmap_get_topic(topicmap,"IS13250","UniqueName","AtOccurringOccurrence",
                                &(at_occurring_occurrence_topic)) != TM_OK)
        {
                assert(0);
        }
        if( tm_topicmap_get_topic(topicmap,"IS13250","UniqueName","RoleOccurring",
                                &(role_occurring_topic)) != TM_OK )
        {
                assert(0);
        }
        if( tm_topicmap_get_topic(topicmap,"IS13250","UniqueName","RoleOccurrence",
                                &(role_occurrence_topic)) != TM_OK )
        {
                assert(0);
        }
	assert(at_basenamed_basename_topic);
        assert(role_basenamed_topic);
        assert(role_basename_topic);
	assert(at_occurring_occurrence_topic);
        assert(role_occurring_topic);
        assert(role_occurrence_topic);

#endif
	/*
	if( tm_topicmap_topic_scan_open_with_single_condition(topicmap,
			"ISO2788","Descriptor","LIKE","ate",&scan) != TM_OK)
	{
		fprintf(stderr,"cannot open scan, %s\n", tm_get_error(tm));
		return;
	}
	*/
	if( tm_topicmap_topic_scan_open_with_double_condition(topicmap,
			"ISO2788","Descriptor","=","soda water",
			"OR",
			"ISO2788","Descriptor","=","water",
			&scan) != TM_OK)
	{
		fprintf(stderr,"cannot open scan, %s\n", tm_get_error(tm));
		return;
	}

	fprintf(f,"Testoutpt\n\n");
	while(! tm_topicmap_topic_scan_finished(topicmap,scan) )
	{
		TMTopic t;
		TMTopic b_topic;
		TMTopic t_topic;
		/*
		char buf[1024];
		*/
		void *v = NULL,*v2 = NULL, *v3 = NULL;
		tm_topicmap_topic_scan_fetch(topicmap,scan,&t);
		tm_topicmap_get_property(topicmap,t,"ISO2788","Descriptor",&v);

		fprintf(f,"%s\n",v);

		tm_value_delete(prop_descriptor->value_type,&v);

	}


	tm_topicmap_topic_scan_close(topicmap,scan);

}
void _test2(TMTopicMap topicmap, FILE *f, int encoding)
{
	TMTopicScan scan;
	TMTopic at_basenamed_basename_topic;
        TMTopic role_basenamed_topic;
        TMTopic role_basename_topic;
	TMTopic at_occurring_occurrence_topic;
        TMTopic role_occurring_topic;
        TMTopic role_occurrence_topic;
	TMModel m;
	
	TMProperty prop_descriptor;
	TMProperty prop_asidp;
	TMProperty prop_sbjadr;
	TMProperty prop_sbjdata;
	TMProperty prop_inddata;

	tmtk_default_model_lookup_function("ISO2788",&m);

	prop_descriptor = tm_model_get_property(m,"Descriptor");
	assert(prop_descriptor);
#if 0
	prop_sbjadr = tm_model_get_property(m,"SubjectAddress");
	prop_sbjdata = tm_model_get_property(m,"SubjectData");
	prop_inddata = tm_model_get_property(m,"IndicatorData");
	assert(prop_asidp);
	assert(prop_sbjadr);
	assert(prop_sbjdata);
	assert(prop_inddata);

	if( tm_topicmap_get_topic(topicmap,"IS13250","UniqueName","AtBaseNamedBaseName",
                                &(at_basenamed_basename_topic)) != TM_OK)
        {
                assert(0);
        }
        if( tm_topicmap_get_topic(topicmap,"IS13250","UniqueName","RoleBaseNamed",
                                &(role_basenamed_topic)) != TM_OK )
        {
                assert(0);
        }
        if( tm_topicmap_get_topic(topicmap,"IS13250","UniqueName","RoleBaseName",
                                &(role_basename_topic)) != TM_OK )
        {
                assert(0);
        }
	if( tm_topicmap_get_topic(topicmap,"IS13250","UniqueName","AtOccurringOccurrence",
                                &(at_occurring_occurrence_topic)) != TM_OK)
        {
                assert(0);
        }
        if( tm_topicmap_get_topic(topicmap,"IS13250","UniqueName","RoleOccurring",
                                &(role_occurring_topic)) != TM_OK )
        {
                assert(0);
        }
        if( tm_topicmap_get_topic(topicmap,"IS13250","UniqueName","RoleOccurrence",
                                &(role_occurrence_topic)) != TM_OK )
        {
                assert(0);
        }
	assert(at_basenamed_basename_topic);
        assert(role_basenamed_topic);
        assert(role_basename_topic);
	assert(at_occurring_occurrence_topic);
        assert(role_occurring_topic);
        assert(role_occurrence_topic);

#endif
	/*
	if( tm_topicmap_topic_scan_open_with_single_condition(topicmap,
			"ISO2788","Descriptor","LIKE","ate",&scan) != TM_OK)
	{
		fprintf(stderr,"cannot open scan, %s\n", tm_get_error(tm));
		return;
	}
	*/
	if( tm_topicmap_topic_scan_open_with_double_condition(topicmap,
			"ISO2788","Descriptor","=","soda water",
			"OR",
			"ISO2788","Descriptor","=","water",
			&scan) != TM_OK)
	{
		fprintf(stderr,"cannot open scan, %s\n", tm_get_error(tm));
		return;
	}

	fprintf(f,"Testoutpt\n\n");
	while(! tm_topicmap_topic_scan_finished(topicmap,scan) )
	{
		TMTopic t;
		TMTopic b_topic;
		TMTopic t_topic;
		/*
		char buf[1024];
		*/
		void *v = NULL,*v2 = NULL, *v3 = NULL;
		tm_topicmap_topic_scan_fetch(topicmap,scan,&t);
		tm_topicmap_get_property(topicmap,t,"ISO2788","Descriptor",&v);

		fprintf(f,"%s\n",v);

		tm_value_delete(prop_descriptor->value_type,&v);

	}


	tm_topicmap_topic_scan_close(topicmap,scan);

}
#endif
 
const char *default_topicmap = "/tmp/.topicmap";
 
static char progname[256];
 
 
void tk_set_progname(const char*n)
{
        assert(n);
        memset(progname,0,sizeof(progname));
        strncpy(progname,n,sizeof(progname));
}
 
void tk_err_exit(const char *fmt,...)
{
        char b[2048], c[2048];
        va_list args;
        assert(fmt);
        va_start(args,fmt);
        vsnprintf(b,sizeof(b),fmt,args);
        va_end(args);
        snprintf(c,sizeof(c),"\n%s: %s\n", progname,b);
        fprintf(stderr,c);
        exit(1);
}
void list_tmas(TMTopicMap topicmap,FILE *f)
{
	TMList lp;
	fprintf(f,"these TMAs are loaded in the topic map:\n");

	tm_topicmap_get_models(topicmap,&lp);
	/* FIXME: error check or not??? */
	for(;lp;lp=lp->next)
	{
		TMModel m = (TMModel)lp->content;
		fprintf(f,"%s\n",m->name);
	}
}

void prompt(TMTopicMap topicmap)
{
	int quit;
	char qbuf[4096];
	char *ptr;

	setbuf(stdout,NULL);

	printf("\n\n");
	printf("Welcome to tm's interactive mode!\n");
	printf("\n");
	printf("Type \\h for help\n");
	printf("\n");
	printf("\n");

	quit = 0;
	bzero(qbuf,sizeof(qbuf));
	ptr = qbuf;
	while(!quit)
	{
		char input[1024];
		char *p;

		printf(PROMPT" ");
		p = fgets(input,sizeof(input),stdin);
		if(!p)
			break;

		p[strlen(p)-1] = '\0';	/* chomp */

		


		if(*p == '\0')
			continue;

		if(*p != '\\')
		{
			struct view_data vdata;
			strcpy(ptr,p);
			if(p[strlen(p)-1] != ';')
			{
				ptr += strlen(p);
				continue;
			}
			qbuf[strlen(qbuf)-1] = '\0';
			bzero(&vdata,sizeof(vdata));
			vdata.topicmap = topicmap;
			vdata.file = stdout;
			/*	
			tm_topicmap_set_view_user_data(topicmap,&vdata);
			tm_topicmap_set_view_pre_event_handler(topicmap,vpre);
			*/

			if( tm_topicmap_query(topicmap,&vdata,vstart,vend,qbuf) != TM_OK)
			{
				printf("could not execute query '%s', %s\n",
				qbuf, tm_get_error(tm) );
			}
			/*
			printf("_%s_\n",qbuf);
			*/

			bzero(qbuf,sizeof(qbuf));
			ptr = qbuf;
			continue;
		}
		

		switch(*++p) {
		case '\0':
			continue;
		case 'q' : 
			quit = 1;
			break;
		case 'h' :
			qhelp();
			break;
		case 'l' :
			list_tmas(topicmap,stdout);
			break;
		case 's' :
			print_stats(topicmap,stdout);
			break;
		default:
			continue;
		}
		


	}
	return;
}



void qhelp(void)
{
        printf("\n\nList of commands:\n");
 
        printf("  \\q           Quit\n");
        printf("  \\h           Show this screen\n");
        printf("  \\l           list the loaded TMAs\n");
#if 0
        printf("        \\X               Show the list of all tenplates\n");
        printf("        \\N               Show the list of all basenames\n");
        printf("        \\O               Show the list of all occurrences\n");
        printf("        \\C               Show the list of all classes\n");
        printf("        \\I               Show the list of all instances\n");
        printf("        \\R               Show the list of all played roles\n");
        printf("        \\S               Show the list of all scopes\n");
        printf("\n");
        printf("        \\a               Show the templates and their constraints\n");
        printf("        \\n               Show a list of subjects and their names\n");
        printf("        \\o               Show a list of subjects and their occurrences\n");
        printf("        \\s               Show the basenames by scopes\n");
#endif
        printf("\n\n\n");
}
void print_stats(TMTopicMap tm,FILE *f)
{
	TMList models,lp;
	fprintf(f,"Topic Map Statistics\n");
	fprintf(f,"--------------------\n");
	tm_topicmap_get_models(tm,&models);
	for(lp=models;lp;lp=lp->next)
	{
		fprintf(f,"%s\n",((TMModel)lp->content)->name);
	
	}	


}

int _find_view_handler(struct view_data *vdp,const char *name)
{
	struct handler_tab *tp;
	vdp->start = NULL;
	vdp->end = NULL;
	for(tp = handler_tab; tp && tp->view_name; tp++)
	{
		if( strcmp(name,tp->view_name) == 0)
		{
			vdp->start = tp->start;
			vdp->end = tp->end;
			return 1;
		}
	}
	
	return 0;
}

const void* _get_attr(void **atts,const char *name,TMValueType *vp)
{
        int i=0;
        for (i = 0; atts && atts[i]; i += 3)
        {
                if(strcmp((char*)atts[i],name) == 0)
                {
                        if(vp)
                                *vp = (TMValueType)atts[i+2];
                        return (atts[i+1]);
                }
        }
        return NULL;
}


struct load_map {
	char *map;
	char *mode;
	char *pm;
	char *arg;
};
#if 0
/*
LoadMap /home/jan/foo xml xtm_simple -
*/
TMError _read_config(TM tm, const char *filename, TMList *listp)
{
        char buf[1024];
        char *p;
        FILE *file;
	TMList list = NULL;

	
 
        if( (file = fopen(filename,"r")) == NULL)
        {
                tm_set_error(o,"unable to open config file %s, %s",
                        filename, strerror(errno));
                return TM_FAIL;
        }
 
        while( (p = fgets(buf,sizeof(buf), file)) != NULL)
        {
                if(tm_is_whitespace_string(p))
                        continue;
 
                if(tm_is_comment_string(p,'#'))
                        continue;

                p = (char*)tm_lstrip(p);
                if( strstr(p,"LoadMap") == p)
                {
			struct load_map *lmp;
			char *q;
                        TMError e;
			TM_NEW(lmp);
			mlp->map = NULL;
			mlp->mode = NULL;
			mlp->pm = NULL;
			mlp->arg = NULL;

			p += 7;
			while( *p && isspace(*p)) p++;
			q = p;
			while( *p && !isspace(*p)) p++;
			*p = "\0";
			mlp->map = tm_strdup(q); 
			
			
		
		}
	}
	*listp = list;
}	
#endif
