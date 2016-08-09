#pragma once
#include <WinSock2.h>



typedef struct Invitation
{
public:
	sockaddr_in* destinationAddress;
	time_t lastAttempt;
} Invitation;