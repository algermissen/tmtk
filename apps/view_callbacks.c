#if 0
static const void* tm_view_event_get_attr(void **atts,const char *name,TMValueType *vp)
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





static int index_start_text(void* user_data, const char *name, void **atts)
{
	const char *label,*href,*data;
	void* indicators;
	TMValueType vt;
	char buf[1024];
	struct view_data *vdp = (struct view_data*)user_data;
	if(strcmp(name,"entry") == 0) {
		label = tm_view_event_get_attr(atts,"label",NULL);
		indicators = (void*)tm_view_event_get_attr(atts,"indicators",&vt);
		fprintf(vdp->file,"Name: %s\n",label);
		if(indicators)
		{
			tm_value_to_string(vt,indicators,buf,sizeof(buf));
			fprintf(vdp->file,"   %s\n",buf);
		}
	} else if(strcmp(name,"occurrence") == 0) {
		data = tm_view_event_get_attr(atts,"data",NULL);
		if(data)
		{
			fprintf(vdp->file,"   Data:");
			tm_fmtprint(vdp->file, data, 9, 40, 1);

		}
		else
		{
			href = tm_view_event_get_attr(atts,"address",NULL);
			fprintf(vdp->file,"    URL: %s\n",href);
		}

	}
	
	return (0);
}
static int index_end_text(void* user_data, const char *name)
{
	struct view_data *vdp = (struct view_data*)user_data;
	if(strcmp(name,"entry") == 0) {
		fprintf(vdp->file,"\n");
#if 0
	} else if(strcmp(name,"occurrence") == 0) {
		data = tm_view_event_get_attr(atts,"data",NULL);
		if(data)
		{
			tm_fmtprint(vdp->file, data, 4, 30, 4);

		}
		else
		{
			href = tm_view_event_get_attr(atts,"address",NULL);
			fprintf(vdp->file,"   -> %s\n",href);
		}
#endif
	}

	
	return (0);
}
static int index_start_html(void* user_data, const char *name, void **atts)
{
	const char *label,*href,*data;
	TMTopic topic;
	void* indicators;
	TMValueType vt;
	/*
	char buf[1024];
	*/
	struct view_data *vdp = (struct view_data*)user_data;
	if(strcmp(name,"entry") == 0) {
		label = tm_view_event_get_attr(atts,"label",NULL);
		topic = (TMTopic)tm_view_event_get_attr(atts,"topic",NULL);
		indicators = (void*)tm_view_event_get_attr(atts,"indicators",&vt);
		fprintf(vdp->file,"<div class=\"entry\">\n");
		fprintf(vdp->file,"<b><a href=\"topic%d.html\">%s</a></b><br />\n",topic,label);
		if(indicators)
		{
			TMList lp;
			fprintf(vdp->file,"<div><small>\n");
			for(lp=(TMList)indicators;lp;lp=lp->next)
			{
				char *p = (char*)lp->content;
				fprintf(vdp->file,"&nbsp;&nbsp;<a href=\"%s\">%s</a><br />\n",p,p);
			}
			fprintf(vdp->file,"</small></div>\n");
		}
		fprintf(vdp->file,"<ul>\n");
	} else if(strcmp(name,"occurrence") == 0) {
		data = tm_view_event_get_attr(atts,"data",NULL);
		if(data)
		{
			fprintf(vdp->file,"<li>%s</li>",data);
		}
		else
		{
			href = tm_view_event_get_attr(atts,"address",NULL);
			fprintf(vdp->file,"<li><a href=\"%s\">%s</a></li>",href,href);
		}

	} else if(strcmp(name,"index") == 0) {
		fprintf(vdp->file,"<html>\n<body>\n");
	}

	
	return (0);
}
static int index_end_html(void* user_data, const char *name)
{
	struct view_data *vdp = (struct view_data*)user_data;
	if(strcmp(name,"entry") == 0) {
		fprintf(vdp->file,"</ul>\n");
		fprintf(vdp->file,"</div>\n\n");
#if 0
	} else if(strcmp(name,"occurrence") == 0) {
		data = tm_view_event_get_attr(atts,"data",NULL);
		if(data)
		{
			tm_fmtprint(vdp->file, data, 4, 30, 4);

		}
		else
		{
			href = tm_view_event_get_attr(atts,"address",NULL);
			fprintf(vdp->file,"   -> %s\n",href);
		}
#endif
	} else if(strcmp(name,"index") == 0) {
		fprintf(vdp->file,"</body>\n</html>\n");

	}

	
	return (0);
}

/* ========================================================================
static int index_start_xml(void* user_data, const char *name, void **atts)
{
	int i;	
	printf("<%s",name);
	for (i = 0; atts && atts[i]; i += 3)
        {
		printf(" %s=\"%s\"", (char*)atts[i],(char*)atts[i + 1]);
	}
	printf(">\n");
	
	return (0);
}
static int index_end_xml(void* user_data, const char *name)
{
	printf("</%s>\n",name);
	return (0);
}
*/

static int hitlist_start_text(void* user_data, const char *name, void **atts)
{
	const char *basename /*,*href,*data*/;
	TMTopic topic;
	void* indicators;
	TMValueType vt;
	char buf[1024];
	struct view_data *vdp = (struct view_data*)user_data;
	if(strcmp(name,"hit") == 0) {
		basename = tm_view_event_get_attr(atts,"basename",NULL);
		indicators = (void*)tm_view_event_get_attr(atts,"indicators",&vt);
		topic = (TMTopic)tm_view_event_get_attr(atts,"topic",NULL);
		fprintf(vdp->file,"%s (%d)\n",basename,topic);
		if(indicators)
		{
			tm_value_to_string(vt,indicators,buf,sizeof(buf));
			fprintf(vdp->file,"   %s\n",buf);
		}
#if 0
	} else if(strcmp(name,"occurrence") == 0) {
		data = tm_view_event_get_attr(atts,"data",NULL);
		if(data)
		{
			fprintf(vdp->file,"   Data:");
			tm_fmtprint(vdp->file, data, 9, 40, 1);

		}
		else
		{
			href = tm_view_event_get_attr(atts,"address",NULL);
			fprintf(vdp->file,"    URL: %s\n",href);
		}
#endif
	}
	
	return (0);
}
static int hitlist_end_text(void* user_data, const char *name)
{
	struct view_data *vdp = (struct view_data*)user_data;
	if(strcmp(name,"hit") == 0) {
		fprintf(vdp->file,"\n");
#if 0
	} else if(strcmp(name,"occurrence") == 0) {
		data = tm_view_event_get_attr(atts,"data",NULL);
		if(data)
		{
			tm_fmtprint(vdp->file, data, 4, 30, 4);

		}
		else
		{
			href = tm_view_event_get_attr(atts,"address",NULL);
			fprintf(vdp->file,"   -> %s\n",href);
		}
#endif
	}

	
	return (0);
}



static int topic_start_html(void* user_data, const char *name, void **atts)
{
        struct view_data *vdp = (struct view_data*)user_data;
	void *indicators;
	TMValueType vt;

	if( strcmp(name,"topic") == 0)
	{
		fprintf(vdp->file,
			"<html>\n"
			"<head>\n"
			"  <title>Topic Page</title>\n"
			"</head>\n"
			"<body>\n"
		);


		indicators = (void*)tm_view_event_get_attr(atts,"indicators",&vt);
		if(indicators)
		{
			char buf[1024];
			tm_value_to_string(vt,indicators,buf,sizeof(buf));
			fprintf(vdp->file,"<p>%s</p>\n",buf);
		}
	}
	else if( strcmp(name,"basename") == 0)
	{
		char buf[1024];
		void *string;
		string = (void*)tm_view_event_get_attr(atts,"string",&vt);
		assert(string);
		tm_value_to_string(vt,string,buf,sizeof(buf));

		fprintf(vdp->file,"<div class=\"basenames\">\n");
		fprintf(vdp->file,"%s\n", buf);

	}
	else if( strcmp(name,"occurrence") == 0)
	{
		char buf[1024];
		void *data, *address;
		fprintf(vdp->file,"<div class=\"occurrence\">\n");
		address = (void*)tm_view_event_get_attr(atts,"address",&vt);
		if(address)
		{
			tm_value_to_string(vt,address,buf,sizeof(buf));
			fprintf(vdp->file,"<a href=\"%s\">%s</a>\n",
				buf,buf);

		}
		data = (void*)tm_view_event_get_attr(atts,"data",&vt);
		if(data)
		{
			tm_value_to_string(vt,data,buf,sizeof(buf));
			fprintf(vdp->file,"<p>%s</p>\n",
				buf);

		}


	}

#if 0
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
#endif
        return (0);
}
static int topic_end_html(void* user_data, const char *name)
{
        struct view_data *vdp = (struct view_data*)user_data;

	if( strcmp(name,"topic") == 0)
	{
		fprintf(vdp->file,"</body>\n</html>\n");
	}
	else if( strcmp(name,"basename") == 0)
	{
		fprintf(vdp->file,"</div>\n");

	}
	else if( strcmp(name,"occurrence") == 0)
	{
		fprintf(vdp->file,"</div>\n");

	}
        return (0);
}
#endif
