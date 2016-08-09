#pragma once
#include "Data.h"
#include <winsock2.h>
class Connection;


class __declspec(dllexport) Channel
{
	friend class Connection;

public:
	Channel(); //(Connection* connection, unsigned short sourceChannelNumber, unsigned short destinationChannelNumber);
	virtual ~Channel();
	virtual void onCreate() { }

	Connection* getConnection();
	sockaddr_in* getDestinationAddress();
	unsigned short getSourceChannelNumber();
	unsigned short getDestinationChannelNumber();
	char getType();
	void close();
	bool isInClosingStateAndIfNotSetIt();

	virtual bool getData(Data** data);
	virtual void processReceivedData(Data* data);

	// Konstanty pro typy kanálù
	static const enum ChannelType
	{
		CONFIRMATORY_CHANNEL,
		MASS_CHANNEL,
		STREAM_CHANNEL
	};

	static const enum Side
	{
		RECEIVER,
		SENDER
	};

protected:
	void notifyDataReady();

	char type;

private:
	unsigned short destinationChannelNumber;
	unsigned short sourceChannelNumber;
	bool isReceiver;
	Connection* connection;
	bool closing;
	CRITICAL_SECTION CS_closing;
};

