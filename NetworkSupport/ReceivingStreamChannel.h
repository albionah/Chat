#pragma once
#include <deque>

#include "Channel.h"
#include "Data.h"
#include "FragmentedData.h"
#include "DataForSending.h"
#include "Connection.h"
class Connection;



class __declspec(dllexport) ReceivingStreamChannel : public Channel
{
	friend class Connection;

public:
	ReceivingStreamChannel();
	virtual ~ReceivingStreamChannel();

	virtual void giveData(Data* data) = 0;

private:
	void processReceivedData(Data* data);

	int getDifference(unsigned short id);

	/*
		Pro zachycen� ID k dispozici jen 14 bit�.
		Prvn� bit ud�v�, zdali je dan� datagram prvn�m z fragmentovan�ch dat.
		Druh� bit ud�v�, zdali je dan� datagram posledn�m z fragmentovan�ch dat.
		Tak�e:
		00 - st�ed fragmentu
		10 - po��te�n� datagram fragmentovan�ch dat
		01 - koncov� datagram fragmentovan�ch dat
		11 - nefragmentov�n� data
	*/
	unsigned short expectedId;
	std::vector<FragmentedData*> dataBuffer;
	/*
	int waitingTime;
	unsigned short lastDatagramIdentification;
	*/
};