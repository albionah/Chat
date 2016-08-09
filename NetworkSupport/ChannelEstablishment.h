#pragma once
#include <time.h>

class Connection;
class Data;



typedef struct ChannelEstablishment
{
	Connection* connection;
	unsigned short channelNumber;
	unsigned char channelType;
	unsigned short dataType;
	Data* data;

	time_t lastAttempt;
	int attempts;
} ChannelEstablishment;