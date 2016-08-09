#pragma once
#include <time.h>

#include "Data.h"



typedef struct DataForSending
{
public:
	DataForSending()
	{
		this->lastAttempt = 0;
		this->attempts = 0;
	}
	~DataForSending()
	{
		delete data;
	}

	Data* data;
	time_t lastAttempt;
	int attempts;
} DataForSending;