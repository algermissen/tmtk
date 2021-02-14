#ifndef DEFAULTFILESTOREDESCRIPTOR_H
#define DEFAULTFILESTOREDESCRIPTOR_H
#include <tmtopicmap.h>

struct data {
	TMTopicMap topicmap;
        int open_count;
        int open_max;
        int cache_size;
        char *path;
	char errstr[1024];
};



#endif
