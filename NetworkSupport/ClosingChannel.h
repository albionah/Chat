#pragma once
#include <time.h>

class Channel;
class Data;



typedef struct ClosingChannel
{
	Channel* channel;
	Data* data;

	time_t lastAttempt;
	int attempts;
} ClosingChannel;