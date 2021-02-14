#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>


#include <topicmaps.h>

#include "tk.h"
static void print_model_ascii(TMModel m,FILE *f);

void usage(void)
{
	printf("Usage: tmainfo\n\n");
	exit(1);
}

void help(void)
{
	printf("tmainfo:\n");

	printf("Options:\n");
	printf("    -h          show this screen\n");
	/*
	printf("    -u uri      specify URI of a map to import\n");
	printf("    -t type     type of input data\n");
	printf("    -f file     file to open\n");
	printf("                http://www.proxy.com:8080  \n");
	printf("    -e          enable subjectEquivalence callback\n");
	printf("    -u uri      specify URI of a map to import\n");
	printf("    -q query    specify a query to be issued\n");
	printf("    -a          enable association callback\n");
	printf("    -v          enable verbose mode\n");
	printf("    -s          show the list of scopes in the map\n");
	printf("    -o          export the topicmap as XTM\n");
	*/
	printf("\n\n\n");
	exit(0);
}

enum { TXT_FMT, XML_FMT, HTML_FMT };

int main(int argc, char **args)
{
	TMError e;
	TMModel model;
	int format = TXT_FMT;
	char option;

	tk_set_progname(args[0]);
	/*
	tm_set_trace_mask("*");
	*/

	opterr = 0;
	
	while( (option = getopt(argc,args, "-hf:")) != EOF)
	{
		switch(option) {
		case 'h':
			help();
		case 'f':
			if(strcmp(optarg,"txt") == 0)
				format = TXT_FMT;
			else
				tk_err_exit("unknown format: %s\n",optarg);
			break;
		case   1:
			if( (e = tmtk_default_model_lookup_function(optarg,&model)) != TM_OK)
				tk_err_exit("cannot lookup model %s, %s\n",
					optarg, tm_strerror(e));
			break;
		case '?':
			usage(); 
		}
	}

	switch(format) {
	case TXT_FMT:
		print_model_ascii(model,stdout);
		break;
	default:
		assert(!"unknown format code");
	}
	return 0;
}

void print_model_ascii(TMModel m,FILE *f)
{
	TMList pp;
	TMSubject *sp;
	TMList mp;
	TMList ap;
	TMList lp;
	/*
	TMProcModel *xp;
	*/
	fprintf(f,"Topic Map Model\n===============\n\n");
	fprintf(f," Name: %s\n",m->name);
	fprintf(f,"About: ");
	tm_fmtprint(f,m->description,7,60, 0);

	fprintf(f,"\n\n");
	fprintf(f,"Required Models: ");
	
	for(mp=m->require; mp; mp=mp->next)
	{
		fprintf(f,"%s ",((TMModel)mp->content)->name);
	}	
	fprintf(f,"\n\n");
	

	fprintf(f,"Properties:\n\n");
	for(pp = m->properties; pp; pp = pp->next )
	{
		TMProperty p = (TMProperty)pp->content;
	 	fprintf(f,"          Name: %s\n",p->name);
	 	fprintf(f,"    Value Type: %s\n",p->value_type->name);
	 	fprintf(f,"          Type: %s\n",p->type == TM_SIDP ? "SIDP" : "OP" );
	 	fprintf(f,"   Combination: %s\n",p->combination_code ? p->combination_code : "---" );
	 	fprintf(f,"      Equality: %s\n",p->equality_code ? p->equality_code : "---" );
		/*
	 	fprintf(f,"  Assign. Type: ");
		switch(p->assignment_type) {
		case TM_CONTEXTUAL_PROPERTY:
			fprintf(f,"Contextual\n");
			break;
		case TM_BUILT_IN_PROPERTY:
			fprintf(f,"Built-In\n");
			break;
		case TM_CONFERRED_PROPERTY:
			fprintf(f,"Conferred\n");
			break;
		case TM_CONFERRED_PROPERTY_IMMEDIATELY:
			fprintf(f,"Conferred-Immediately\n");
			break;
		default:
			assert(0);
		}
		*/
	 	fprintf(f,"         About: ");
		tm_fmtprint(f,p->description,16,50, 0);

		fprintf(f,"\n\n");
	}
	fprintf(f,"Assertion Types:\n\n");
	for(ap = m->atypes; ap; ap = ap->next)
	{
		int i;
		TMList rp;
		TMSubject s;
		TMAssertionType a = (TMAssertionType)ap->content;
		fprintf(f,"     Name: %s\n", a->name);
		s = a->subject;

		fprintf(f,"  Subject:\n");
		s = a->subject;
		for(i = 0; i<s->N;i++)
		{
			char buf[1024];
			TMProperty p;
			/*
			assert( strcmp(s->props[i].model,m->name) == 0);
			*/
			p = tm_model_get_property(m, s->props[i].name);
			assert(p);

			tm_value_to_string(p->value_type,s->props[i].value,buf,sizeof(buf));

			fprintf(f,"           %s::%s=%s\n",s->props[i].model, s->props[i].name, buf);
		}
	 	fprintf(f,"    About: ");
		tm_fmtprint(f,a->description,11,50, 0);

	 	fprintf(f,"    Roles:\n");
		for(rp = a->roles; rp; rp=rp->next)
		{
			TMRoleType r = (TMRoleType)rp->content;
			fprintf(f,"              Name: %s\n",r->name);
			s = r->subject;

	 		fprintf(f,"           Subject:\n");
			for(i = 0; i<s->N;i++)
			{
				char buf[1024];
				TMProperty p;
				/* 
				assert( strcmp(s->props[i].model,m->name) == 0);
				*/
				p = tm_model_get_property(m, s->props[i].name);
				assert(p);
 
				tm_value_to_string(p->value_type,s->props[i].value,buf,sizeof(buf));
 
				fprintf(f,"                    %s::%s=%s\n",s->props[i].model, s->props[i].name, buf);
			}
	 		fprintf(f,"             About: ");
			tm_fmtprint(f,r->description,20,40, 0);


		}
		/*
		for(i = 0; i<s->N;i++)
		{
			char buf[1024];
			TMProperty p;
			
			assert( strcmp(s->props[i].model,m->name) == 0);
			p = tm_model_get_property(m, s->props[i].name);
			assert(p);

			tm_value_to_string(p->value_type,s->props[i].value,buf,sizeof(buf));
		
			fprintf(f,"%s::%s=%s\n",s->props[i].model, s->props[i].name, buf);
		}
		*/
		fprintf(f,"\n");
	}
	fprintf(f,"Built-In Subjects\n\n");
	for(lp = m->subjects; lp; lp = lp->next)
	{
		int i;
		TMSubject s;

		fprintf(f,"  Subject:\n");
		s = (TMSubject)lp->content;
		for(i = 0; i<s->N;i++)
		{
			char buf[1024];
			TMProperty p;
			/*
			assert( strcmp(s->props[i].model,m->name) == 0);
			*/
			p = tm_model_get_property(m, s->props[i].name);
			assert(p);

			tm_value_to_string(p->value_type,s->props[i].value,buf,sizeof(buf));

			fprintf(f,"    %s::%s=%s\n",s->props[i].model, s->props[i].name, buf);
		}
	}
	
#if 0
	fprintf(f,"Built-In-Nodes:\n\n");
	for(sp = m->built_in_topics; sp && *sp; sp++)
	{
		int i;
		TMSubject s = *sp;
		for(i = 0; i<s->N;i++)
		{
			char buf[1024];
			TMProperty p;
			
			/* FIXME: why did I assert this? 
			assert( strcmp(s->props[i].model,m->name) == 0);
			*/
			p = tm_model_get_property(m, s->props[i].name);
			assert(p);

			tm_value_to_string(p->value_type,s->props[i].value,buf,sizeof(buf));
		
			fprintf(f,"  %s::%s=%s\n",s->props[i].model, s->props[i].name, buf);
		}
		fprintf(f,"\n");
	}
	fprintf(f,"Processing Models:\n");
#endif
#if 0	
	for(xp=m->proc_models; xp && *xp; xp++)
	{
		fprintf(f,"  %s\n",(*xp)->name );
		fprintf(f,"    Root element name: <%s>\n",(*xp)->root_element_name ? (*xp)->root_element_name : "-");
		fprintf(f,"        Namespace URI: %s\n",(*xp)->namespace ? (*xp)->namespace : "-");
		fprintf(f,"        DTD Public ID: %s\n",(*xp)->public_id ? (*xp)->public_id : "-" );
		/*
		fprintf(f,"        %s\n",(*xp)->system_id );
		*/
	}	
	fprintf(f,"\n\n");
#endif

	fprintf(f,"END MODEL\n\n");
}


TM_API(TMProperty) tm_model_get_property(TMModel self, const char *name);

