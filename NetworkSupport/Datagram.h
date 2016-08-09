#pragma once
#include <WinSock2.h>
#include <iostream>

#include "Data.h"


typedef struct Datagram
{
public:
	Datagram() { }
	Datagram(sockaddr_in* address, Data* data): address(address), data(data) { }
	Datagram(sockaddr_in* address, char* data, unsigned short length): address(address)
	{
		this->data = new Data(data, length);
	}
	~Datagram()
	{
		//delete address;
	}
	void clear()
	{
		delete address;
		delete data;
	}

	sockaddr_in* address;
	Data* data;
} Datagram;