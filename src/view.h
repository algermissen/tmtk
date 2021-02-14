#ifndef TM_VIEW_H
#define TM_VIEW_H

struct view
{
	TM tm;
	TMTopicMap topicmap;
	void *user_data;
	TMViewStartEventHandler start;
	TMViewEndEventHandler end;
};

#endif
