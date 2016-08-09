#include "Connection.h"



// Kontruktor
Connection::Connection(Communication* communication, sockaddr_in* destinationAddress, bool isPrimary): channels(4)
{
	this->communication = communication;
	this->destinationAddress = destinationAddress;

	this->priorityChannels = new std::vector<Channel*>[PRIORITY_LEVELS];
	for (int j = 0; j < PRIORITY_LEVELS; j++) this->startInPriorityChannels.push_back(0);

	InitializeSRWLock(&this->LOCK_channels);
	InitializeSRWLock(&this->LOCK_channelNumbers);
	InitializeConditionVariable(&this->CV_sending);
	InitializeConditionVariable(&this->CV_pausing);

	InitializeCriticalSection(&this->CS_pausing);
	InitializeCriticalSection(&this->CS_sending);
	InitializeCriticalSection(&this->CS_reservedChannelNumbers);
	InitializeCriticalSection(&this->CS_listOfRefreshing);
	InitializeCriticalSection(&this->CS_latency);
	InitializeCriticalSection(&this->CS_realSpeed);
	this->connected = true;
	//this->channelIterator = this->channels.begin();
	this->speed = MIN_SPEED;
	this->basicSpeed = this->speed;
	this->startTime = Communication::timeInMiliseconds();
	this->realSpeed = 0;
	this->lengthSum = 0;
	this->areDataAlreadyReady = false;
	//this->speed = 1000;
	this->echoInterval = 100;
	this->isInterrupted = false;
	//this->primary = isPrimary;
	this->nextRefreshId = 0;
	this->nextSentId = 1;
	this->lastReceivedId = 0;
	this->pointerToLatency = 0;
	this->localMinLatency = 0xffff;
	this->globalMinLatency = 0xffff;
	this->isProblem = false;
	//UPPER_LIMIT_RATIO_FOR_SLOWDOWN = 0.9F;
	//SPEED_MULTIPLIER_FOR_SPEEDUP = 1.1F;

	this->thread = (HANDLE) _beginthread(&Connection::start, 0, this);
	
	#ifdef LOG
	this->log.open("latency connection " + std::to_string(Communication::timeInMiliseconds()) + ".txt");
	#endif

	//this->log.open("latency connection " + std::to_string(this->communication->port) + std::to_string(ntohs(this->destinationAddress->sin_port)) + ".txt");
}



Connection::~Connection()
{
	std::cout << "desktruktor Connection" << std::endl;
	this->isInterrupted = true;

	EnterCriticalSection(&this->CS_sending); // LOCK
	WakeConditionVariable(&this->CV_sending);
	LeaveCriticalSection(&this->CS_sending); // UNLOCK

	EnterCriticalSection(&this->CS_pausing); // LOCK
	WakeConditionVariable(&this->CV_pausing);
	LeaveCriticalSection(&this->CS_pausing); // UNLOCK

	WaitForSingleObject(this->thread, INFINITE);

	for (int j = 0; j < PRIORITY_LEVELS; j++)
	{
		for (Channel* channel : this->priorityChannels[j])
		{
			delete channel;
		}
	}
	delete[] this->priorityChannels;

	for (Refreshing* refreshing: this->listOfRefreshing)
	{
		delete refreshing;
	}

	delete destinationAddress;

	#ifdef LOG
	this->log.close();
	#endif

	DeleteCriticalSection(&this->CS_pausing);
	DeleteCriticalSection(&this->CS_sending);
	DeleteCriticalSection(&this->CS_reservedChannelNumbers);
	DeleteCriticalSection(&this->CS_listOfRefreshing);
	DeleteCriticalSection(&this->CS_latency);
	DeleteCriticalSection(&this->CS_realSpeed);
}



Communication* Connection::getCommunication()
{
	return this->communication;
}



// Metoda slouží k odpojení tohoto spojení
void Connection::disconnect()
{
	this->communication->disconnect(this);
}



// Metoda slouží ke zjištìní, jestli je spojení navázáno
bool Connection::isConnected()
{
	return this->connected;
}


/*
bool Connection::isPrimary()
{
	return this->primary;
}
*/


void Connection::refresh(char* data)
{
	Refreshing* refreshing = new Refreshing;
	refreshing->id = this->nextSentId;
	refreshing->time = Communication::timeInMiliseconds();

	time_t firstPendingRefresh;

	EnterCriticalSection(&this->CS_listOfRefreshing);
	this->listOfRefreshing.push_back(refreshing);
	firstPendingRefresh = (*this->listOfRefreshing.begin())->time;
	LeaveCriticalSection(&this->CS_listOfRefreshing);

	if (Communication::timeInMiliseconds() - firstPendingRefresh > Communication::INACTIVE_TIME)
	{
		std::cout << "disconnect connection z refreshingu" << std::endl;
		disconnect();
	}

	data[3] = this->nextSentId;
	data[4] = this->nextSentId >> 8;
	this->nextSentId++;
	this->communication->send(this->destinationAddress, data, 5);

	/*
	time_t time = Communication::timeInMiliseconds();
	EnterCriticalSection(&this->CS_listOfRefreshing);
	if (!this->listOfRefreshing.empty()) time =- (*this->listOfRefreshing.begin())->time;
	LeaveCriticalSection(&this->CS_listOfRefreshing);
	*/
	//if (time > Communication::INACTIVE_TIME) disconnect();
}



void Connection::processRefresh(unsigned short id)
{
	if ((id > this->lastReceivedId || this->lastReceivedId - id > 30000) && (id <= this->nextSentId || id - this->nextSentId > 30000))
	{
		int length = id - this->lastReceivedId - 1;
		this->lastReceivedId = id;
		if (length < 0) length += USHRT_MAX + 1;

		EnterCriticalSection(&this->CS_listOfRefreshing); // LOCK
		auto iterator = this->listOfRefreshing.begin();
		for (int j = 0; j < length; j++) // Odstraníme všechny starší refreshe, ty už nás stejnì nezajímají
		{
			delete (*iterator);
			iterator = this->listOfRefreshing.erase(iterator);
		}

		time_t time = (*iterator)->time;
		delete (*iterator);
		this->listOfRefreshing.erase(iterator);
		LeaveCriticalSection(&this->CS_listOfRefreshing); // UNLOCK

		time_t currentTime = Communication::timeInMiliseconds();
		unsigned short latency = currentTime - time;

		if (this->localMinLatency > latency) this->localMinLatency = latency;
		if (this->globalMinLatency > latency) this->globalMinLatency = latency;

		/*
		if (currentTime - this->latencyAverageStart > 1024)
		{

			this->latencyAverageStart = currentTime;
		}
		else
		{
			this->newLatencySum += latency;
			this->newLatencyCount++;
		}
		*/

		#ifdef LOG
		this->log << "odezva: " + std::to_string(latency) << std::endl;
		#endif
		

		EnterCriticalSection(&this->CS_latency); // LOCK
		//this->lock_latency.lock(); // LOCK
		this->latency[this->pointerToLatency] = latency;
		this->pointerToLatency = (++this->pointerToLatency) % 3;
		recountSpeed();
		//this->lock_latency.unlock(); // UNLOCK
		LeaveCriticalSection(&this->CS_latency); // UNLOCK
	}
}



time_t Connection::getLastActivity()
{
	return this->lastActivity;
}



void Connection::createChannel(unsigned char channelType, unsigned short dataType)
{
	unsigned short channelNumber = generateChannelNumber();

	DataBuilder data(6, 0);
	data.set((unsigned char) Communication::CHANNEL_ESTABLISHMENT);
	data.set(channelType);
	data.set(channelNumber);
	data.set(dataType);
	this->communication->send(this->destinationAddress, data.getData());

	ChannelEstablishment* channelEstablishment = new ChannelEstablishment;
	channelEstablishment->connection = this;
	channelEstablishment->channelNumber = channelNumber;
	channelEstablishment->channelType = channelType;
	channelEstablishment->dataType = dataType;
	channelEstablishment->attempts = 1;
	channelEstablishment->lastAttempt = Communication::timeInMiliseconds();
	channelEstablishment->data = data.getData();
	this->communication->addChannelEstablishment(channelEstablishment);
}



void Connection::closeChannel(Channel* channel)
{
	if (channel->isInClosingStateAndIfNotSetIt()) return;

	DataBuilder data(3, 0);
	data.set((unsigned char) Communication::CLOSING_CHANNEL);
	data.set(channel->getDestinationChannelNumber());
	this->communication->send(this->destinationAddress, data.getData());

	ClosingChannel* closingChannel = new ClosingChannel;
	closingChannel->channel = channel;
	closingChannel->attempts = 1;
	closingChannel->lastAttempt = Communication::timeInMiliseconds();
	closingChannel->data = data.getData();
	this->communication->addClosingChannel(closingChannel);
}



void Connection::processCreateChannel(unsigned char channelType, unsigned short dataType, unsigned short sourceChannelNumber, unsigned short destinationChannelNumber)
{
	Channel* channel = NULL;
	
	bool side = Channel::SENDER;

	std::cout << "processCreateChannel " << (int) destinationChannelNumber << std::endl;
	if (sourceChannelNumber == 0)
	{
		side = Channel::RECEIVER;
		sourceChannelNumber = generateChannelNumber();
		DisposableDataBuilder data(5, 0);
		data.set((unsigned char) Communication::CHANNEL_ESTABLISHMENT_CONFIRMATION);
		data.set(destinationChannelNumber);
		data.set(sourceChannelNumber);
		this->communication->send(this->destinationAddress, data.getData());
		std::cout << "odeslano potvrzeni" << std::endl;
	}

	unsigned short priority = 0;
	channel = creatingChannel(side, channelType, dataType, &priority, sourceChannelNumber, destinationChannelNumber);
	
	// ovìøit rozsah priority
	if (channel != NULL)
	{
		channel->connection = this;
		channel->sourceChannelNumber = sourceChannelNumber;
		channel->destinationChannelNumber = destinationChannelNumber;

		unsigned short index;
		/*unsigned short size;
		unsigned short minIndex = 0;
		unsigned short maxIndex;*/

		AcquireSRWLockExclusive(&this->LOCK_channels); // LOCK
		this->channels.write(sourceChannelNumber, channel);

		auto iterator = findPriorityChannelIterator(priority, sourceChannelNumber);
		index = iterator - this->priorityChannels[priority].begin();
		if (iterator == this->priorityChannels[priority].end()) this->priorityChannels[priority].push_back(channel);
		else this->priorityChannels[priority].insert(iterator, channel);
		if (index <= this->startInPriorityChannels[priority]) this->startInPriorityChannels[priority]++;
		/*
		size = this->priorityChannels[priority].size();
		maxIndex = size - 1;
		if (size == 0) this->priorityChannels[priority].push_back(channel);
		else if (this->priorityChannels[priority][minIndex]->getSourceChannelNumber() > sourceChannelNumber) this->priorityChannels[priority].insert(this->priorityChannels[priority].begin(), channel);
		else if (this->priorityChannels[priority][maxIndex]->getSourceChannelNumber() < sourceChannelNumber) this->priorityChannels[priority].push_back(channel);
		else
		{
			// Binární vyhledávání
			while (true)
			{
				index = (maxIndex + minIndex) / 2;
				if (this->priorityChannels[priority][index]->getSourceChannelNumber() < sourceChannelNumber)
				{
					minIndex = index + 1;
				}
				else if (this->priorityChannels[priority][index]->getSourceChannelNumber() > sourceChannelNumber)
				{
					maxIndex = index - 1;
					if (minIndex >= maxIndex)
					{
						index = minIndex;
						break;
					}
				}
				else break;
			}
			this->priorityChannels[priority].insert(this->priorityChannels[priority].begin() + index, channel);
			if (index < this->startInPriorityChannels[priority]) this->startInPriorityChannels[priority]++;
		}*/
		ReleaseSRWLockExclusive(&this->LOCK_channels); // UNLOCK
		channel->onCreate();
	}
	else
	{
		// Pokud nebyl kanál vytvoøen, tak uvolníme kanálové èíslo
		AcquireSRWLockExclusive(&this->LOCK_channelNumbers); // LOCK
		auto iterator = std::lower_bound(this->channelNumbers.begin(), this->channelNumbers.end(), sourceChannelNumber);
		this->channelNumbers.erase(iterator);
		ReleaseSRWLockExclusive(&this->LOCK_channelNumbers); // UNLOCK
	}
}



Channel* Connection::creatingChannel(bool side, unsigned char channelType, unsigned short dataType, unsigned short* priority, unsigned short sourceChannelNumber, unsigned short destinationChannelNumber)
{
	Channel* channel = NULL;

	switch (channelType)
	{
		case Channel::CONFIRMATORY_CHANNEL:
			if (side == Channel::SENDER) processCreateChannel((SendingConfirmatoryChannel**) &channel, dataType, priority);
			else processCreateChannel((ReceivingConfirmatoryChannel**) &channel, dataType, priority);
			break;

		case Channel::MASS_CHANNEL:
			if (side == Channel::SENDER) processCreateChannel((SendingMassChannel**) &channel, dataType, priority);
			else processCreateChannel((ReceivingMassChannel**) &channel, dataType, priority);
			break;

		case Channel::STREAM_CHANNEL:
			if (side == Channel::SENDER) processCreateChannel((SendingStreamChannel**) &channel, dataType, priority);
			else processCreateChannel((ReceivingStreamChannel**) &channel, dataType, priority);
			break;
	}
	std::cout << (int) channel << std::endl;
	return channel;
}



void Connection::processClosingChannel(unsigned short sourceChannelNumber)
{
	// Pošli potvrzení
	DisposableDataBuilder data(3, 0);
	data.set((unsigned char) Communication::CLOSING_CHANNEL_CONFIRMATION);
	data.set(sourceChannelNumber);
	this->communication->send(this->destinationAddress, data.getData());

	// Najdi pøíslušný kanál a smaž ho
	Channel* channel = NULL;
	AcquireSRWLockShared(&this->LOCK_channels); // LOCK
	channel = this->channels[sourceChannelNumber];
	ReleaseSRWLockShared(&this->LOCK_channels); // UNLOCK

	processClosingChannel(channel);
}



void Connection::processClosingChannel(Channel* channel)
{
	if (channel == NULL) return;

	// Najdi pøíslušný kanál a smaž jej
	unsigned int index;
	/*unsigned int size;
	unsigned int minIndex;
	unsigned int maxIndex;*/
	
	AcquireSRWLockExclusive(&this->LOCK_channels); // LOCK

	for (int level = 0; level < PRIORITY_LEVELS; level++)
	{
		auto iterator = findPriorityChannelIterator(level, channel->getSourceChannelNumber());
		index = this->priorityChannels[level].begin() - iterator;
		if (iterator != this->priorityChannels[level].end() && (*iterator)->getSourceChannelNumber() == channel->getSourceChannelNumber())
		{
			this->priorityChannels[level].erase(iterator);
			if (index <= this->startInPriorityChannels[level]) this->startInPriorityChannels[level]--;

			bool isLast = false;
			AcquireSRWLockExclusive(&this->LOCK_channelNumbers); // LOCK
			auto iterator = std::lower_bound(this->channelNumbers.begin(), this->channelNumbers.end(), channel->getSourceChannelNumber());
			if (iterator == --this->channelNumbers.end()) isLast = true;
			this->channelNumbers.erase(iterator);
			ReleaseSRWLockExclusive(&this->LOCK_channelNumbers); // UNLOCK

			this->channels.erase(channel->getSourceChannelNumber(), isLast);
			delete channel;

			break;
		}
		/*
		size = this->priorityChannels[level].size();
		maxIndex = size - 1;
		minIndex = 0;

		if (size > 0)
		{
			// Binární vyhledávání
			while (true)
			{
				index = (maxIndex + minIndex) / 2;
				if (this->priorityChannels[level][index]->getSourceChannelNumber() < channel->getSourceChannelNumber())
				{
					minIndex = index + 1;
				}
				else if (this->priorityChannels[level][index]->getSourceChannelNumber() > channel->getSourceChannelNumber())
				{
					maxIndex = index - 1;
				}
				else
				{
					channel = this->priorityChannels[level][index];
					break;
				}

				if (minIndex == maxIndex)
				{
					if (this->priorityChannels[level][minIndex]->getSourceChannelNumber() == channel->getSourceChannelNumber())
					{
						index = minIndex;
						channel = this->priorityChannels[level][index];
					}
					break;
				}
			}
			if (channel != NULL)
			{
				this->priorityChannels[level].erase(this->priorityChannels[level].begin() + index);
				if (index < this->startInPriorityChannels[level]) this->startInPriorityChannels[level]--;
				break;
			}
		}
	}

	if (channel != NULL)
	{
		bool isLast = false;
		AcquireSRWLockExclusive(&this->LOCK_channelNumbers); // LOCK
		auto iterator = std::lower_bound(this->channelNumbers.begin(), this->channelNumbers.end(), channel->getSourceChannelNumber());
		if (iterator == --this->channelNumbers.end()) isLast = true;
		this->channelNumbers.erase(iterator);
		ReleaseSRWLockExclusive(&this->LOCK_channelNumbers); // UNLOCK

		this->channels.erase(channel->getSourceChannelNumber(), isLast);
		delete channel;
	}*/
	}
	ReleaseSRWLockExclusive(&this->LOCK_channels); // UNLOCK
}



bool Connection::getData(Data** data)
{
	unsigned short offset, size;
	AcquireSRWLockShared(&this->LOCK_channels); // LOCK
	for (int level = 0; level < PRIORITY_LEVELS; level++)
	{
		offset = this->startInPriorityChannels[level];
		size = this->priorityChannels[level].size();

		for (auto j = 0; j < size; j++)
		{
			if (this->priorityChannels[level][(j + offset) % size]->getData(data))
			{
				this->startInPriorityChannels[level] = (j + 1) % size;
				ReleaseSRWLockShared(&this->LOCK_channels); // UNLOCK
				return true;
			}
		}
	}
	ReleaseSRWLockShared(&this->LOCK_channels); // UNLOCK

	return false;
}



void Connection::processReceivedData(Data* data, short channelNumber)
{
	Channel* channel = NULL;

	AcquireSRWLockShared(&this->LOCK_channels); // LOCK
	channel = this->channels[channelNumber];
	if (channel != NULL) channel->processReceivedData(data);
	ReleaseSRWLockShared(&this->LOCK_channels); // UNLOCK

	if (channel == NULL) delete data;
}



void Connection::notifyDataReady()
{
	EnterCriticalSection(&this->CS_sending); // LOCK
	this->areDataAlreadyReady = true;
	WakeConditionVariable(&this->CV_sending);
	LeaveCriticalSection(&this->CS_sending); // UNLOCK
}



unsigned int Connection::getAndRecountRealSpeed()
{
	unsigned int realSpeed = 0;

	EnterCriticalSection(&this->CS_realSpeed); // LOCK
	//if (this->realSpeed > 0)
	{
		time_t time = Communication::timeInMiliseconds();

		if (time - this->startTime >= 250)
		{
			this->realSpeed = 1000 * this->lengthSum / (time - this->startTime);
			std::cout << "skutecna rychlost je: " << this->realSpeed << std::endl;
			//std::cout << "time: " << time << ", sT: " << this->startTime << ", r: " << time - this->startTime << ", real: " << this->realSpeed << "l: " << this->lengthSum << std::endl;
			//std::cout << "skutecna rychlost: " << this->realSpeed << std::endl;
			this->startTime = time;
			this->lengthSum = 0;
		}
		realSpeed = this->realSpeed;
	}
	LeaveCriticalSection(&this->CS_realSpeed); // UNLOCK

	return realSpeed;
}



void Connection::recountSpeed()
{
	//unsigned short olderLatency = min(this->latency[(this->pointerToLatency + 1) % 3], this->latency[(this->pointerToLatency + 2) % 3]);
	//this->newestLatency = min(this->latency[this->pointerToLatency], this->latency[(this->pointerToLatency + 2) % 3]);
	this->newestLatency = min(this->latency[(this->pointerToLatency + 1) % 3], this->latency[(this->pointerToLatency + 2) % 3]);
	unsigned short olderLatency = min(this->latency[this->pointerToLatency], this->latency[(this->pointerToLatency + 1) % 3]);

	if (this->isProblem)
	{
		if ((this->newestLatency + 100) < 3 * (this->globalMinLatency + 100) || Communication::timeInMiliseconds() - this->timeOfProblem >= MAX_TIME_TO_WAIT) this->isProblem = false;
	}
	else if ((this->newestLatency + 100) > 7 * (this->globalMinLatency + 100)) // Mechanismus poslední záchrany
	{
		this->speed = MIN_SPEED;
		this->timeOfProblem = Communication::timeInMiliseconds();
		this->isProblem = true;
		#ifdef LOG
		this->log << "posledni zachrana" << std::endl;
		#endif
	}
	else if ((this->newestLatency + 100) > 2.5 * (this->localMinLatency + 100)) // Mechanismus záchrany
	{
		this->speed = this->basicSpeed / 2;
		if (this->speed < 20000) this->speed = 20000;
		this->localMinLatency = this->newestLatency;
		#ifdef LOG
		this->log << "zachrana: rychlost = " + std::to_string(this->speed) << " basic speed = " << this->basicSpeed << std::endl;
		#endif
	}
	else
	{
		float ratio = (float) (olderLatency + 10) / (this->newestLatency + 10);
		unsigned int real = getAndRecountRealSpeed();
		#ifdef LOG
		this->log << "ratio: " + std::to_string(ratio) << std::endl;
		#endif
		if (ratio < UPPER_LIMIT_RATIO_FOR_SLOWDOWN)
		{
			this->speed = this->basicSpeed * ratio;
			if (this->speed < 20000) this->speed = 20000;
			//this->speed = this->basicSpeed * sqrt(ratio);
			//this->speed = this->basicSpeed * 0.5; // zpomalení
			std::cout << "srychlost: " << this->speed << std::endl;
			#ifdef LOG
			this->log << "snizeni: " + std::to_string(this->speed) + " skutecna rychlost: " + std::to_string(real) << " pomer: " << std::to_string((float) real / this->speed) << std::endl;
			#endif
		}
		else
		{
			#ifdef LOG
			this->log << "zvyseni: " + std::to_string(this->speed) + " skutecna rychlost: " + std::to_string(real) << " pomer: " << std::to_string((float) real / this->speed) << std::endl;
			#endif

			if ((float) real / (this->speed + 1) < 0.25F)
			{
				return;
			}

			unsigned int speed = this->basicSpeed * SPEED_MULTIPLIER_FOR_SPEEDUP; // zrychlení
			if (speed < this->speed) this->speed = UINT32_MAX;
			else this->speed = speed;
			std::cout << "zrychlost: " << this->speed << std::endl;
		}
	}
	this->basicSpeed = this->speed;
	#ifdef LOG
	this->log << "rychlost: " + std::to_string(this->speed) + " ( " + std::to_string(olderLatency) + " + 10 / " + std::to_string(this->newestLatency) + " + 10 = " + std::to_string((float) (olderLatency + 10) / (this->newestLatency + 10)) + " )" << std::endl;
	#endif
}



// Spustí run
void Connection::start(void* parameter)
{
	((Connection*) parameter)->run();
}



// Èinnost vlánka
void Connection::run()
{
	std::chrono::system_clock::time_point last = std::chrono::system_clock::now();
	Data* data = NULL;
	float preciseWait = 0;
	int wait = 0;
	time_t time;
	time_t sleepTime;
	time_t sleepFor = 0;
	unsigned short latency;
	unsigned short difference;
	//this->startTime = Communication::timeInMiliseconds();
	while (!this->isInterrupted)
	{
		last = std::chrono::system_clock::now();
		if (!Connection::getData(&data))
		{
			EnterCriticalSection(&this->CS_sending); // LOCK
			if (!this->areDataAlreadyReady)
			{
				sleepTime = Communication::timeInMiliseconds();
				SleepConditionVariableCS(&this->CV_sending, &this->CS_sending, INFINITE);
				sleepFor = Communication::timeInMiliseconds() - sleepTime;
			}
			this->areDataAlreadyReady = false;
			LeaveCriticalSection(&this->CS_sending); // UNLOCK

			if (sleepFor > 100)
			{
				EnterCriticalSection(&this->CS_latency); // LOCK
				this->basicSpeed = 2 * this->basicSpeed / log10(sleepFor);
				std::cout << "spinkal " << sleepFor << " a rychlost je " << this->basicSpeed << std::endl;
				if (this->basicSpeed < MIN_SPEED) this->basicSpeed = MIN_SPEED;
				this->speed = this->basicSpeed;
				LeaveCriticalSection(&this->CS_latency); // UNLOCK
				sleepFor = 0;
			}
		}
		else
		{
			this->communication->send(this->destinationAddress, data->bytes, data->length);

			EnterCriticalSection(&this->CS_realSpeed); // LOCK
			this->lengthSum += data->length;
			LeaveCriticalSection(&this->CS_realSpeed); // UNLOCK

			/* 
				Pokud ping datagramy nechodí, pak se snažíme situaci
				øešit tímto kodem. Díky tomu zrychlíme reakci systému
				na zahlcení.
			*/
			//std::cout << "rychlost: " << this->speed << std::endl;
			//if (this->speed / (getAndRecountRealSpeed() + 1) <= 1.25F)
			if (getAndRecountRealSpeed() / (this->speed + 1) >= 0.25F)
			{
				difference = this->nextSentId - this->lastReceivedId - 1;
				if (difference < 0) difference += USHRT_MAX + 1;
				if (difference > 0)
				{
					//std::cout << "rychla uprava rychlosti" << std::endl;
					EnterCriticalSection(&this->CS_listOfRefreshing);
					if (this->listOfRefreshing.empty()) time = Communication::timeInMiliseconds();
					else time = (*this->listOfRefreshing.begin())->time;
					LeaveCriticalSection(&this->CS_listOfRefreshing);

					latency = Communication::timeInMiliseconds() - time;

					EnterCriticalSection(&this->CS_latency); // LOCK
					//this->lock_latency.lock(); // LOCK
					float ratio = (float) (this->newestLatency + 10) / (latency + 10);
					if (ratio < UPPER_LIMIT_RATIO_FOR_SLOWDOWN)
					{
						this->speed = this->basicSpeed * ratio;
						if (this->speed < 20000) this->speed = 20000;
						std::cout << "snizeni rychlosti na " << this->speed << std::endl;
					}
					//this->lock_latency.unlock(); // UNLOCK
					LeaveCriticalSection(&this->CS_latency); // UNLOCK
				}
			}
			//getAndRecountRealSpeed();

			preciseWait += 1000 * data->length / this->speed;
			wait = preciseWait;
			if (wait >= 2)
			{
				preciseWait -= wait;
				EnterCriticalSection(&this->CS_pausing); // LOCK
				SleepConditionVariableCS(&this->CV_pausing, &this->CS_pausing, wait);
				LeaveCriticalSection(&this->CS_pausing); // UNLOCK
			}
			
			while (this->isProblem)
			{
				EnterCriticalSection(&this->CS_pausing); // LOCK
				SleepConditionVariableCS(&this->CV_pausing, &this->CS_pausing, 100);
				LeaveCriticalSection(&this->CS_pausing); // UNLOCK
			}

			EnterCriticalSection(&this->CS_sending); // LOCK
			this->areDataAlreadyReady = false;
			LeaveCriticalSection(&this->CS_sending); // UNLOCK
		}
		//std::cout << "ca2: " << std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - last).count() << std::endl;

		data = NULL;
	}
}



std::vector<Channel*>::iterator Connection::findPriorityChannelIterator(int priority, unsigned short channelNumber)
{
	return std::lower_bound(this->priorityChannels[priority].begin(), this->priorityChannels[priority].end(), channelNumber,
		[](Channel* channel, unsigned short number)
		{
			return channel->getSourceChannelNumber() < number;
		});
}



/*
Nula by mohla být vrácena, když už by nebyl žádné kanálové èíslo.
*/
unsigned short Connection::generateChannelNumber()
{
	unsigned short index;
	unsigned short size;
	unsigned short minIndex = 0;
	unsigned short maxIndex;
	unsigned short channelNumber;

	AcquireSRWLockExclusive(&this->LOCK_channelNumbers); // LOCK
	size = this->channelNumbers.size();
	maxIndex = size;
	
	if (size == 0)
	{
		channelNumber = 1;
		this->channelNumbers.push_back(channelNumber);
	}
	else if (this->channelNumbers[size - 1] == size)
	{
		channelNumber = this->channelNumbers[size - 1] + 1;
		this->channelNumbers.push_back(channelNumber);
	}
	else if (this->channelNumbers[0] != 1)
	{
		channelNumber = 1;
		this->channelNumbers.insert(this->channelNumbers.begin(), channelNumber);
	}
	else
	{
		// Binární vyhledávání
		while (true)
		{
			index = (maxIndex + minIndex) / 2;
			if (this->channelNumbers[index] == index + 1)
			{
				if (this->channelNumbers[index + 1] == index + 2) minIndex = index + 1;
				else break;
			}
			else
			{
				if (this->channelNumbers[index - 1] != index) maxIndex = index - 1;
				else
				{
					index--;
					break;
				}
			}
		}
		channelNumber = this->channelNumbers[index] + 1;
		this->channelNumbers.insert(this->channelNumbers.begin() + index + 1, channelNumber);
	}
	ReleaseSRWLockExclusive(&this->LOCK_channelNumbers); // UNLOCK

	return channelNumber;
}