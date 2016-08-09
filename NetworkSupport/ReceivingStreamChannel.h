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
		Pro zachycení ID k dispozici jen 14 bitù.
		První bit udává, zdali je daný datagram prvním z fragmentovaných dat.
		Druhý bit udává, zdali je daný datagram posledním z fragmentovaných dat.
		Takže:
		00 - støed fragmentu
		10 - poèáteèní datagram fragmentovaných dat
		01 - koncový datagram fragmentovaných dat
		11 - nefragmentováná data
	*/
	unsigned short expectedId;
	std::vector<FragmentedData*> dataBuffer;
	/*
	int waitingTime;
	unsigned short lastDatagramIdentification;
	*/
};