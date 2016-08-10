#include "Channel.h"

#include "Connection.h"



Channel::Channel()
{
	this->connection = NULL;
	this->closing = false;
	InitializeCriticalSection(&this->CS_closing);
}



Channel::~Channel(void)
{
	DeleteCriticalSection(&this->CS_closing);
}



Connection* Channel::getConnection()
{
	return this->connection;
}



sockaddr_in* Channel::getDestinationAddress()
{
	return this->connection->destinationAddress;
}



unsigned short Channel::getSourceChannelNumber()
{
	return this->sourceChannelNumber;
}



unsigned short Channel::getDestinationChannelNumber()
{
	return this->destinationChannelNumber;
}



char Channel::getType()
{
	return this->type;
}



void Channel::close()
{
	getConnection()->closeChannel(this);
}



bool Channel::isInClosingStateAndIfNotSetIt()
{
	bool closing;
	EnterCriticalSection(&this->CS_closing);
	closing = this->closing;
	this->closing = true;
	LeaveCriticalSection(&this->CS_closing);

	return closing;
}



void Channel::notifyDataReady()
{
	if (this->connection != NULL) this->connection->notifyDataReady();
}



bool Channel::getData(Data** data)
{
	return false;
}



void Channel::processReceivedData(Data* data)
{
	delete data;
}