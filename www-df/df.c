#include <tmdisclosureframework.h>
#include <tmassert.h>

#include <errno.h>

static TMError _from_file(TMDisclosureFramework self,TM tm, const char *filename,TMModel *mp);

extern TMModel tm_create_tma_from_file(TMDisclosureFramework df,TM tm,FILE *file);

static TMError lookup_model(TMDisclosureFramework self,TM tm,const char *name,TMModel *mp)
{
	TMError e;
	char filename[1024]; /* FIXME */
	if( strcmp(name,"http://www.gooseworks.org/disclosures/SAM.xml") == 0) {
		snprintf(filename,sizeof(filename),"%s/tmtk/www-df/disclosures/SAM.xml",PREFIX);
	} else if( strcmp(name,"http://www.gooseworks.org/disclosures/HIERARCHY.xml") == 0) {
		snprintf(filename,sizeof(filename),"%s/tmtk/www-df/disclosures/HIERARCHY.xml",PREFIX);
	} else if( strcmp(name,"http://www.gooseworks.org/disclosures/DC.xml") == 0) {
		snprintf(filename,sizeof(filename),"%s/tmtk/www-df/disclosures/DC.xml",PREFIX);
	} else if( strstr(name,"file://") == name) {
		snprintf(filename,sizeof(filename),"%s",name+7);
	} else {
		tm_set_error(tm,"cannot find model named %s" ,name);
		return TM_FAIL;
	}

	if( (e = _from_file(self,tm,filename,mp)) != TM_OK)
		return e;

	return TM_OK;
}

extern struct TMProcModel iso2788procmodel;
extern struct TMProcModel xtm_simple;
extern struct TMProcModel rdf_common;

static TMError lookup_procmodel(TMDisclosureFramework self,TM tm,const char *name,TMProcModel *pmp)
{
	*pmp = NULL;
        if(strcmp(name,"iso2788") == 0)
                *pmp = &iso2788procmodel;
        else if(strcmp(name,"xtm_simple") == 0)
                *pmp = &xtm_simple;
        else if(strcmp(name,"rdf_common") == 0)
                *pmp = &rdf_common;
        else
        {
		tm_set_error(tm,"unkown processing model %s",name);
		return TM_FAIL;
        }
	return TM_OK;
}

extern TMError (tm_view_sam_index)(struct view *vp, TMParams params);
extern TMError (tm_view_sam_topic)(struct view *vp, TMParams params);
extern TMError (tm_view_role_playings)(struct view *vp, TMParams params);
extern TMError (tm_view_hitlist)(struct view *vp, TMParams params);

static TMError lookup_viewhandler(TMDisclosureFramework self,TM tm,const char *name,TMViewHandler *pmp)
{

	if(strcmp(name,"index") == 0)
        {
                *pmp = &tm_view_sam_index;
        }
        else if(strcmp(name,"hitlist") == 0)
        {
                *pmp = &tm_view_hitlist;
        }
#if 0
        else if(strcmp(name,"namelist") == 0)
        {
                *pmp = &tm_view_sam_namelist;
        }
        else if(strcmp(name,"tops") == 0)
        {
                *pmp = &tm_view_sam_tops;
        }
#endif
        else if(strcmp(name,"topic") == 0)
        {
                *pmp = &tm_view_sam_topic;
        }
        else if(strcmp(name,"role_playings") == 0)
        {
                *pmp = &tm_view_role_playings;
        }
        else
        {
                tm_set_error(tm,"unknown view '%s'", name);
                        return TM_E_SYNTAX;
	}
	return TM_OK;
}
        

struct TMDisclosureFramework www_df = {
	"www-df",
	lookup_model,
	lookup_procmodel,
	lookup_viewhandler
};

TMError _from_file(TMDisclosureFramework self,TM tm, const char *filename,TMModel *mp)
{
     TMModel model;
        FILE *f;
        model = NULL;
#if 0
        char buf[1024];
        char name_with_slashes[1024];
        char *tmtk_home;
        char *p;
        tmtk_home = getenv("TMTK_HOME");

        assert(sizeof(name_with_slashes) > strlen(name));  /* FIXME need a lot of work on all this here! */
        strcpy(name_with_slashes,name);
        for(p=name_with_slashes;*p;p++)
        {
                if(*p == '.')
                        *p = '/';
        }


        /* FIXME: handle possible / at end of path although it is no problem */
        /* FIXME: check TMTK envvar also, propably make PATH */
        /*
        sprintf(buf,"%s/%s.xml", tmtk_home ? tmtk_home : "./" , name_with_slashes);
        */

        /* PREFIX is determined by configure and compiled in */
        sprintf(buf,"%s/tmtk/tma/%s.xml", PREFIX , name_with_slashes);
#endif
        if( (f = fopen(filename,"r")) == NULL)
        {
                tm_set_error(tm, "cannot open %s, %s\n",filename,strerror(errno));
                return TM_FAIL;
        }
        if( (model = tm_create_tma_from_file(self,tm,f)) == NULL)
        {
                return TM_FAIL;
        }
        fclose(f);

        *mp = model;
	return TM_OK;
}

