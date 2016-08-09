#pragma once
#include <WinSock2.h>



typedef struct EstablishingConnection
{
public:
	sockaddr_in* destinationAddress;
	time_t lastAttempt;
	int attempts;
	time_t session;
} EstablishingConnection;


// alias
typedef struct EstablishingConnection ClosingConnection;