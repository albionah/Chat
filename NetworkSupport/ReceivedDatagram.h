#pragma once
#include <winsock2.h>
#include "Datagram.h"



typedef struct ReceivedDatagram
{
	Datagram* datagram;

	ReceivedDatagram* next;
} ReceivedDatagram;