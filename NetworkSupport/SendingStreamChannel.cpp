#include "SendingStreamChannel.h"



SendingStreamChannel::SendingStreamChannel()
{
	this->type = STREAM_CHANNEL;
	this->nextId = 0;
	this->last = std::chrono::system_clock::now();
}



SendingStreamChannel::~SendingStreamChannel()
{
	while (!this->queue.empty())
	{
		delete this->queue.front();
		this->queue.pop();
	}
}



bool SendingStreamChannel::getData(Data** data)
{
	std::chrono::system_clock::time_point start = std::chrono::system_clock::now();

	if (!this->queue.empty())
	{
		delete this->queue.front();
		this->queue.pop();
	}

	if (this->queue.empty())
	{
		char* bytes;
		unsigned int size = onGetData(&bytes);
		if (size == 0)
		{
			notifyDataReady();
			return false;
		}

		unsigned short length;
		bool isThisFirstFragment = true;
		bool isThisLastFragment;
		unsigned int position = 0;
		Data* d;
		do
		{
			if (size - position > Communication::DATAGRAM_SIZE - 4)
			{
				length = Communication::DATAGRAM_SIZE - 4;
				isThisLastFragment = false;
			}
			else
			{
				length = size - position;
				isThisLastFragment = true;
			}
			d = new Data(length + 4);
			d->bytes[0] = getDestinationChannelNumber();
			d->bytes[1] = getDestinationChannelNumber() >> 8;
			d->bytes[2] = this->nextId;
			d->bytes[3] = (this->nextId >> 8) & 0x3f;
			this->nextId++;
			if (isThisFirstFragment) d->bytes[3] |= 0x80;
			if (isThisLastFragment) d->bytes[3] |= 0x40;
			isThisFirstFragment = false;
			memcpy(d->bytes + 4, bytes + position, length);
			position += length;
			this->queue.push(d);
		} while (!isThisLastFragment);
	}
	
	*data = this->queue.front();
	//this->queue.pop();
	//std::cout << "ssc: " << std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - start).count() << std::endl;
	
	//std::cout << "cas: " << std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - this->last).count() << std::endl;
	this->last = std::chrono::system_clock::now();
	return true;
}
