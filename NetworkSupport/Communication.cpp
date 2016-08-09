#include "Communication.h"



// Konstruktor
// Obsazení libovolného volného portu
Communication::Communication()
{
	sockaddr_in sockaddr;
	sockaddr.sin_port = 0;
	init(sockaddr);
}



// Konstruktor
// Obsazení specifikovaného portu
Communication::Communication(unsigned short sourcePort)
{
	sockaddr_in sockaddr;
	sockaddr.sin_port = htons(sourcePort);
	init(sockaddr); // možná zdroj chyb, lepší vytvoøit ukazatel
}



// Inicializace promenných a spuštìní všech vláken
// potøebných pro chod celého síového rozhraní
void Communication::init(sockaddr_in& sockaddr)
{

	int success;
	WSADATA wsadata;
	success = WSAStartup(0x0202, &wsadata);
	if (success != 0)
	{
		char desciption[] = "Nastala chyba u WSAStartup; specifikace: ";
		char* buffer = new char[sizeof(desciption) + 10];
		sprintf_s(buffer, sizeof(desciption) + 10, "%s%d", desciption, success);
		throw NetworkError(NetworkError::INITIALIZATION, buffer);
	}
	
	this->Socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (this->Socket == INVALID_SOCKET)
	{
		char desciption[] = "Nepodaøilo se vytvoøit socket; specifikace: ";
		char* buffer = new char[sizeof(desciption) + 10];
		sprintf_s(buffer, sizeof(desciption) + 10, "%s%d", desciption, WSAGetLastError());
		WSACleanup();
		throw NetworkError(NetworkError::INITIALIZATION, buffer);
	}

	//memset( &(sockaddr.s_addr), '\0', 8 );
	sockaddr.sin_family = AF_INET;
	sockaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	success = bind(this->Socket, (struct sockaddr *) &sockaddr, sizeof(sockaddr));
	if (success == -1)
	{
		char desciption[] = "Nepodaøilo se pøiøadit socketu adresu; specifikace: ";
		char* buffer = new char[sizeof(desciption) + 10];
		sprintf_s(buffer, sizeof(desciption) + 10, "%s%d", desciption, success);
		WSACleanup();
		throw NetworkError(NetworkError::INITIALIZATION, buffer);
	};
	/*
	if (result == (auto) SOCKET_ERROR)
	{
		WSACleanup();
		//throw HTTPError(HTTPError::CONNECTION_PROCESS, "");
	}*/

	this->CV_processing = new CONDITION_VARIABLE;
	InitializeConditionVariable(this->CV_processing);

	InitializeSRWLock(&this->LOCK_connections);

	InitializeCriticalSection(&this->CS_initialReceivedDatagram);
	InitializeCriticalSection(&this->CS_processing);
	InitializeCriticalSection(&this->CS_establishingConnections);
	InitializeCriticalSection(&this->CS_invitations);
	InitializeCriticalSection(&this->CS_closingConnections);
	InitializeCriticalSection(&this->CS_channelEstablishments);
	InitializeCriticalSection(&this->CS_closingChannels);
	InitializeCriticalSection(&this->CS_sending);

	this->isInterrupted = false;
	ReceivedDatagram* initialDatagram = new ReceivedDatagram;
	initialDatagram->next = NULL;
	this->processing = new Processing(this, this->CV_processing, this->CS_processing, initialDatagram, this->CS_initialReceivedDatagram);
	this->receiving = new Receiving(this, this->Socket, this->CV_processing, this->CS_processing, initialDatagram, this->CS_initialReceivedDatagram);

	this->threadSendingInvitations = (HANDLE) _beginthread(&Communication::startSendingInvitations, 0, this);
	this->threadSendingConnections = (HANDLE) _beginthread(&Communication::startSendingConnections, 0, this);
	this->threadSendingDisconnections = (HANDLE) _beginthread(&Communication::startSendingDisconnections, 0, this);
	this->threadRefreshing = (HANDLE) _beginthread(&Communication::startRefreshing, 0, this);
	this->threadSendingChannelEstablishments = (HANDLE) _beginthread(&Communication::startChannelEstablishments, 0, this);
	this->threadSendingClosingChannels = (HANDLE) _beginthread(&Communication::startClosingChannels, 0, this);
}



Communication::~Communication()
{
	delete this->receiving;
	delete this->processing;
	delete this->CV_processing;

	this->isInterrupted = true;
	ResumeThread(this->threadSendingInvitations); // TODO: toto ukonèování vláken není správnì
	ResumeThread(this->threadSendingConnections);
	ResumeThread(this->threadSendingDisconnections);
	ResumeThread(this->threadRefreshing);
	ResumeThread(this->threadSendingChannelEstablishments);
	ResumeThread(this->threadSendingClosingChannels);
	WaitForSingleObject(this->threadSendingInvitations, INFINITE);
	WaitForSingleObject(this->threadSendingConnections, INFINITE);
	WaitForSingleObject(this->threadSendingDisconnections, INFINITE);
	WaitForSingleObject(this->threadRefreshing, INFINITE);
	WaitForSingleObject(this->threadSendingChannelEstablishments, INFINITE);
	WaitForSingleObject(this->threadSendingClosingChannels, INFINITE);
	CloseHandle(this->threadSendingInvitations);
	CloseHandle(this->threadSendingConnections);
	CloseHandle(this->threadSendingDisconnections);
	CloseHandle(this->threadRefreshing);
	CloseHandle(this->threadSendingChannelEstablishments);
	CloseHandle(this->threadSendingClosingChannels);

	closesocket(this->Socket);

	EnterCriticalSection(&this->CS_invitations); // LOCK
	for (sockaddr_in* invitation: this->invitations)
	{
		delete invitation;
	}
	LeaveCriticalSection(&this->CS_invitations); // UNLOCK

	EnterCriticalSection(&this->CS_establishingConnections); // LOCK
	for (EstablishingConnection* establishingConnection: this->establishingConnections)
	{
		delete establishingConnection;
	}
	LeaveCriticalSection(&this->CS_establishingConnections); // UNLOCK

	EnterCriticalSection(&this->CS_closingConnections); // LOCK
	for (Connection* connection: this->closingConnections)
	{
		delete connection;
	}
	LeaveCriticalSection(&this->CS_closingConnections); // UNLOCK

	AcquireSRWLockExclusive(&this->LOCK_connections); // LOCK
	for (Connection* connection : this->connections)
	{
		delete connection;
	}
	ReleaseSRWLockExclusive(&this->LOCK_connections); // UNLOCK

	EnterCriticalSection(&this->CS_channelEstablishments); // LOCK
	for (ChannelEstablishment* channelEstablishment : this->channelEstablishments)
	{
		delete channelEstablishment->data;
		delete channelEstablishment;
	}
	LeaveCriticalSection(&this->CS_channelEstablishments); // UNLOCK

	EnterCriticalSection(&this->CS_closingChannels); // LOCK
	for (ClosingChannel* closingChannel : this->closingChannels)
	{
		delete closingChannel->data;
		delete closingChannel;
	}
	LeaveCriticalSection(&this->CS_closingChannels); // UNLOCK

	DeleteCriticalSection(&this->CS_initialReceivedDatagram);
	DeleteCriticalSection(&this->CS_processing);
	DeleteCriticalSection(&this->CS_establishingConnections);
	DeleteCriticalSection(&this->CS_invitations);
	DeleteCriticalSection(&this->CS_closingConnections);
	DeleteCriticalSection(&this->CS_channelEstablishments);
	DeleteCriticalSection(&this->CS_closingChannels);
	DeleteCriticalSection(&this->CS_sending);
}



// Ukonèení èinnosti síového balíku,
// ukonèení èinnosti všech vláken
// a uvolnìní portu
void Communication::close()
{
	/*
	for (Connection connection: this->connections)
	{
		sendPacket(connection.destinationAddress, null, DISCONNECTION);
	}

	this->receivingPackets.close();
	this->socket.close();
	interrupt();
	*/
}



// Metoda slouží k posílání INVITATION paketù na vybranou adresu
void Communication::sendInvitation(sockaddr_in* destinationAddress)
{
	sendControlInformation(destinationAddress, INVITATION);

	EnterCriticalSection(&this->CS_invitations);
	for (sockaddr_in* address: this->invitations)
	{
		if (compareAddresses(address, destinationAddress))
		{
			LeaveCriticalSection(&this->CS_invitations);
			return;
		}
	}
	LeaveCriticalSection(&this->CS_invitations);

	EnterCriticalSection(&this->CS_invitations);
	this->invitations.push_back(destinationAddress);
	LeaveCriticalSection(&this->CS_invitations);
}



// Metoda slouží k zastavení posílá Invitation paketù
void Communication::stopSendingInvitation(sockaddr_in* destinationAddress)
{
	EnterCriticalSection(&this->CS_invitations);
	for (auto iterator = this->invitations.begin(); iterator != this->invitations.end(); iterator++)
	{
		if (compareAddresses((*iterator), destinationAddress))
		{
			this->invitations.erase(iterator);
			break;
		}
	}
	LeaveCriticalSection(&this->CS_invitations);
}



// Metoda slouží k navázání spojení
// Po úspìšném spojení je uživateli pomocí abstraktní
// metody processConnection pøedán objekt Connection
// V pøípadì neúspìchu je uživatel informován
// abstraktní metodou unableToConnect
void Communication::connect(sockaddr_in* destinationAddress)
{
	std::cout << " chci se pripojit" << std::endl;
	//sendControlInformation(destinationAddress, CONNECTION);
	stopSendingInvitation(destinationAddress);

	EstablishingConnection* establishingConnection = new EstablishingConnection;
	establishingConnection->destinationAddress = destinationAddress;
	establishingConnection->attempts = 1;
	time_t time = timeInMiliseconds();
	establishingConnection->lastAttempt = time;
	establishingConnection->session = time;

	EnterCriticalSection(&CS_establishingConnections);
	this->establishingConnections.push_back(establishingConnection);
	LeaveCriticalSection(&CS_establishingConnections);

	DisposableDataBuilder data(1 + 8, 0);
	data.set((unsigned char) CONNECTION);
	data.set(time); // session
	Communication::send(destinationAddress, data.getData());
}



// Metoda slouží k ukonèení aktivního spojení
void Communication::disconnect(Connection* connection)
{
	bool found = false;

	EnterCriticalSection(&CS_closingConnections); // LOCK
	for (Connection* c : this->closingConnections)
	{
		if (compareAddresses(c->destinationAddress, connection->destinationAddress))
		{
			found = true;
			break;
		}
	}
	if (!found)
	{
		this->closingConnections.push_back(connection);
	}
	LeaveCriticalSection(&CS_closingConnections); // UNLOCK
}



void Communication::addChannelEstablishment(ChannelEstablishment* channelEstablishment)
{
	EnterCriticalSection(&this->CS_channelEstablishments);
	this->channelEstablishments.push_back(channelEstablishment);
	LeaveCriticalSection(&this->CS_channelEstablishments);
}



void Communication::addClosingChannel(ClosingChannel* closingChannel)
{
	EnterCriticalSection(&this->CS_closingChannels);
	this->closingChannels.push_back(closingChannel);
	LeaveCriticalSection(&this->CS_closingChannels);
}



void Communication::send(sockaddr_in* address, char* data, unsigned short length)
{
	EnterCriticalSection(&this->CS_sending); // LOCK
	int bytes = sendto(this->Socket, data, length, 0, (sockaddr*) address, sizeof(*address));
	//std::cout << "poslano " << (int) bytes << " bytu" << std::endl;
	LeaveCriticalSection(&this->CS_sending); // UNLOCK
}



void Communication::send(sockaddr_in* address, Data* data)
{
	Communication::send(address, data->bytes, data->length);
}



void Communication::send(Datagram* datagram)
{
	Communication::send(datagram->address, datagram->data->bytes, datagram->data->length);
}



void Communication::sendControlInformation(sockaddr_in* destinationAddress, unsigned char controlInformation)
{
	std::cout << " odeslana ridice informace: " << (int) controlInformation << " ";
	DisposableDataBuilder data(1, 0);
	data.set(controlInformation);
	Communication::send(destinationAddress, data.getData());
}



// Metoda slouží ke zpracovávání pøijatých paketù
void Communication::processReceivedDatagram(Datagram* datagram)
{
	// Zjištìní channelu
	unsigned short channelNumber = *((short*) datagram->data->bytes);

	// Jedná se o øídící informaci?
	if (channelNumber == 0)
	{
		if (datagram->data->length < 3)
		{
			datagram->clear();
			delete datagram;
		}

		switch (datagram->data->bytes[2]) // Typ kontrolní informace
		{
			case INVITATION:
				break;

			case CONNECTION:
			{
				std::cout << " zadost o spojeni" << std::endl;
				sendControlInformation(datagram->address, CONNECTION_CONFIRMATION);
				Connection* connection = findConnection(datagram->address);
				if (connection != NULL)
				{
					DataParser parser(datagram->data, 3);
					if (connection->session == parser.getUint64())
					{
						datagram->clear();
						break;
					}
					else
					{
						findAndEraseConnection(connection->destinationAddress);
						processDisconnection(connection);
						delete connection;
					}
				}

				EnterCriticalSection(&this->CS_invitations);
				for (auto iterator = this->invitations.begin(); iterator != this->invitations.end(); iterator++)
				{
					if (compareAddresses((*iterator), datagram->address))
					{
						delete *iterator;
						this->invitations.erase(iterator);
						break;
					}
				}
				LeaveCriticalSection(&this->CS_invitations);

				processCreatingConnection(&connection, datagram->address, false);
				if (connection != NULL)
				{
					DataParser parser(datagram->data, 3);
					connection->session = parser.getUint64();
					bool added = addConnection(connection);
					if (added) connection->onCreate();
					else delete connection;
				}
				delete datagram->data;
				break;
			}

			case CONNECTION_CONFIRMATION:
			{
				std::cout << " potvrzeni spojeni" << std::endl;
				bool found = false;
				EnterCriticalSection(&this->CS_establishingConnections); // LOCK
				for (auto iterator = this->establishingConnections.begin(); iterator != this->establishingConnections.end(); iterator++)
				{
					if (compareAddresses((*iterator)->destinationAddress, datagram->address))
					{
						found = true;
						this->establishingConnections.erase(iterator);
						break;
					}
				}
				LeaveCriticalSection(&this->CS_establishingConnections); // UNLOCK
				if (!found)
				{
					datagram->clear();
					break;
				}

				// Ještì se procházela všechna spojení a pokud by se tam už adresa vyskytla, tak se to ukonèilo.
				// Já ale myslím, že je to zbyteèné, ovšem nutno ovìøit.
				Connection* connection = NULL;
				processCreatingConnection(&connection, datagram->address, true);
				if (connection != NULL)
				{
					bool added = addConnection(connection);
					if (added) connection->onCreate();
					else delete connection;
					delete datagram->data;
				}
				else datagram->clear();
				break;
			}

			case DISCONNECTION:
			{
				//sendControlInformation(datagram->address, DISCONNECTION_CONFIRMATION);

				Connection* connection = findAndEraseConnection(datagram->address);
				if (connection != NULL)
				{
					processDisconnection(connection);
					delete connection;
				}

				datagram->clear();

				break;
			}

			/*
			case DISCONNECTION_CONFIRMATION:
			{
				ClosingConnection* closingConnection = NULL;
				EnterCriticalSection(&this->CS_closingConnections);
				for (auto iterator = this->closingConnections.begin(); iterator != this->closingConnections.end(); iterator++)
				{
					if (compareAddresses((*iterator)->destinationAddress, datagram->address))
					{
						closingConnection = (*iterator);
						this->closingConnections.erase(iterator);
						break;
					}
				}
				LeaveCriticalSection(&this->CS_closingConnections);

				if (closingConnection != NULL)
				{
					Connection* connection = findAndEraseConnection(datagram->address);
					processDisconnection(connection);
					delete connection;
				}
				datagram->clear();
				break;
			}
			*/

			case CHANNEL_ESTABLISHMENT:
			{
				Connection* connection = findConnection(datagram->address);
				if (connection != NULL && datagram->data->length >= 6)
				{
					DataParser data(datagram->data, 3);
					unsigned char channelType = data.getChar();
					unsigned short destinationChannelNumber = data.getShort();
					unsigned short dataType = data.getShort();
					connection->processCreateChannel(channelType, dataType, 0, destinationChannelNumber);
				}
				datagram->clear();
				break;
			}

			case CHANNEL_ESTABLISHMENT_CONFIRMATION:
			{
				Connection* connection = findConnection(datagram->address);
				if (connection != NULL && datagram->data->length >= 7)
				{
					DataParser data(datagram->data, 3);
					unsigned short sourceChannelNumber = data.getShort();
					unsigned short destinationChannelNumber = data.getShort();
					unsigned char channelType;
					unsigned short dataType;
					ChannelEstablishment* channelEstablishment = NULL;
					EnterCriticalSection(&this->CS_channelEstablishments);
					for (auto iterator = this->channelEstablishments.begin(); iterator != this->channelEstablishments.end(); iterator++)
					{
						if ((*iterator)->channelNumber == sourceChannelNumber)
						{
							channelEstablishment = *iterator;
							this->channelEstablishments.erase(iterator);
							break;
						}
					}
					LeaveCriticalSection(&this->CS_channelEstablishments);

					if (channelEstablishment != NULL)
					{
						channelType = channelEstablishment->channelType;
						dataType = channelEstablishment->dataType;
						connection->processCreateChannel(channelType, dataType, sourceChannelNumber, destinationChannelNumber);
						delete channelEstablishment->data;
					}
				}
				datagram->clear();
				break;
			}

			case CLOSING_CHANNEL:
			{
				Connection* connection = findConnection(datagram->address);
				if (connection != NULL)
				{
					DataParser data(datagram->data, 3);
					unsigned short sourceChannelNumber = data.getShort();
					connection->processClosingChannel(sourceChannelNumber);
				}
				datagram->clear();
				break;
			}

			case CLOSING_CHANNEL_CONFIRMATION:
			{
				Connection* connection = findConnection(datagram->address);
				if (connection != NULL)
				{
					DataParser data(datagram->data, 3);
					unsigned short destinationChannelNumber = data.getShort();
					ClosingChannel* closingChannel = NULL;
					Channel* channel = NULL;
					EnterCriticalSection(&this->CS_closingChannels);
					for (auto iterator = this->closingChannels.begin(); iterator != this->closingChannels.end(); iterator++)
					{
						if ((*iterator)->channel->getDestinationChannelNumber() == destinationChannelNumber)
						{
							closingChannel = *iterator;
							channel = closingChannel->channel;
							this->closingChannels.erase(iterator);
							break;
						}
					}
					LeaveCriticalSection(&this->CS_closingChannels);
					if (closingChannel != NULL)
					{
						delete closingChannel->data;
						delete closingChannel;
						connection->processClosingChannel(channel);
					}
				}
				datagram->clear();
				break;
			}
				
			case REFRESHING:
			{
				datagram->data->bytes[2] = REFRESHING_CONFIRMATION;
				send(datagram->address, datagram->data);
				datagram->clear();
				break;
			}

			case REFRESHING_CONFIRMATION:
			{
				Connection* connection = findConnection(datagram->address);
				if (connection != NULL)
				{
					DataParser data(datagram->data, 3);
					connection->processRefresh(data.getUshort());
				}
				datagram->clear();
				break;
			}
		}
	}
	else
	{
		Connection* connection = findConnection(datagram->address);
		if (connection != NULL) connection->processReceivedData(datagram->data, channelNumber);
		else datagram->clear();
	}

	/*
	Vymazání datagramu.
	Data by už mìla být smazána.
	Adresa taky, kromì CONNECTION a CONNECTION_CONFIRMATION, kde se ukládá ke Connection.
	*/
	delete datagram;
}



void Communication::startSendingInvitations(void* parameter)
{
	((Communication*) parameter)->runSendingInvitations();
}



void Communication::startSendingConnections(void* parameter)
{
	((Communication*) parameter)->runSendingConnections();
}



void Communication::startSendingDisconnections(void* parameter)
{
	((Communication*) parameter)->runSendingDisconnections();
}



void Communication::startRefreshing(void* parameter)
{
	((Communication*) parameter)->runRefreshing();
}



void Communication::startChannelEstablishments(void* parameter)
{
	((Communication*) parameter)->runSendingChannelEstablishments();
}



void Communication::startClosingChannels(void* parameter)
{
	((Communication*) parameter)->runSendingClosingChannels();
}



void Communication::runSendingInvitations()
{
	while (!this->isInterrupted)
	{
		EnterCriticalSection(&this->CS_invitations);
		for (sockaddr_in* address: this->invitations)
		{
			sendControlInformation(address, INVITATION);
		}
		LeaveCriticalSection(&this->CS_invitations);

		Sleep(500);
	}
}



void Communication::runSendingConnections()
{
	time_t timeDifference, minimumTimeDifference;

	while (!this->isInterrupted)
	{
		minimumTimeDifference = 1000;
		EnterCriticalSection(&this->CS_establishingConnections);

		EstablishingConnection* establishingConnection;
		for (auto iterator = this->establishingConnections.begin(); iterator != this->establishingConnections.end();)
		{
			establishingConnection = *iterator;
			timeDifference = timeInMiliseconds() - establishingConnection->lastAttempt;
			if (timeDifference < 1000)
			{
				if ((1000 - timeDifference) < minimumTimeDifference) minimumTimeDifference = 1000 - timeDifference;
				iterator++;
			}
			else
			{
				if (establishingConnection->attempts >= 5) // KONSTANTA
				{
					unableToConnect((*iterator)->destinationAddress);
					iterator = this->establishingConnections.erase(iterator);
				}
				else
				{
					std::cout << " chci se pripojit" << std::endl;
					//sendControlInformation(establishingConnection->destinationAddress, CONNECTION);
					DisposableDataBuilder data(1 + 8, 0);
					data.set((unsigned char) CONNECTION);
					data.set(establishingConnection->session);
					Communication::send(establishingConnection->destinationAddress, data.getData());
					establishingConnection->lastAttempt = timeInMiliseconds();
					establishingConnection->attempts++;
					iterator++;
				}
			}
		}

		LeaveCriticalSection(&this->CS_establishingConnections);

		Sleep(minimumTimeDifference);
	}
}



void Communication::runSendingDisconnections()
{
	while (!this->isInterrupted)
	{
		EnterCriticalSection(&this->CS_closingConnections); // LOCK
		for (auto iterator = this->closingConnections.begin(); iterator != this->closingConnections.end(); )
		{
			sendControlInformation((*iterator)->destinationAddress, DISCONNECTION);
			Connection* connection = findAndEraseConnection((*iterator)->destinationAddress);
			if (connection != NULL)
			{
				processDisconnection(connection);
				delete connection;
			}
			iterator = this->closingConnections.erase(iterator);
		}
		LeaveCriticalSection(&this->CS_closingConnections); // UNLOCK

		Sleep(1000);
	}
	/*
	time_t timeDifference, minimumTimeDifference;

	while (!this->isInterrupted)
	{
		minimumTimeDifference = 1000;
		EnterCriticalSection(&this->CS_closingConnections);

		ClosingConnection* closingConnection;
		for (auto iterator = this->closingConnections.begin(); iterator != this->closingConnections.end(); )
		{
			closingConnection = *iterator;
			timeDifference = timeInMiliseconds() - closingConnection->lastAttempt;
			if (timeDifference < 1000)
			{
				if ((1000 - timeDifference) < minimumTimeDifference) minimumTimeDifference = 1000 - timeDifference;
				iterator++;
			}
			else
			{
				if (closingConnection->attempts >= 3)
				{
					disconnect(findConnection((*iterator)->destinationAddress));
					iterator = this->closingConnections.erase(iterator);
				}
				else
				{
					sendControlInformation(closingConnection->destinationAddress, DISCONNECTION);
					closingConnection->lastAttempt = timeInMiliseconds();
					closingConnection->attempts++;
					iterator++;
				}
			}
		}

		LeaveCriticalSection(&this->CS_closingConnections);

		Sleep(minimumTimeDifference);
	}
	*/
}



void Communication::runSendingChannelEstablishments()
{
	time_t timeDifference, minimumTimeDifference;
	ChannelEstablishment* channelEstablishment;

	while (!this->isInterrupted)
	{
		minimumTimeDifference = 1000;
		EnterCriticalSection(&this->CS_channelEstablishments);
		for (auto iterator = this->channelEstablishments.begin(); iterator != this->channelEstablishments.end(); )
		{
			std::cout << "pocet: " << this->channelEstablishments.size() << std::endl;
			channelEstablishment = *iterator;
			timeDifference = timeInMiliseconds() - channelEstablishment->lastAttempt;
			if (timeDifference < 1000)
			{
				if ((1000 - timeDifference) < minimumTimeDifference) minimumTimeDifference = 1000 - timeDifference;
				iterator++;
			}
			else
			{
				if (channelEstablishment->attempts >= 3)
				{
					iterator = this->channelEstablishments.erase(iterator);
					std::cout << "odstraneni" << std::endl;
					if (iterator == this->channelEstablishments.end()) std::cout << "rovna se" << std::endl;
					else std::cout << "nerovna se" << std::endl;
				}
				else
				{
					send(channelEstablishment->connection->destinationAddress, channelEstablishment->data);
					channelEstablishment->lastAttempt = timeInMiliseconds();
					channelEstablishment->attempts++;
					iterator++;
				}
			}
		}

		LeaveCriticalSection(&this->CS_channelEstablishments);

		Sleep(minimumTimeDifference);
	}
}



void Communication::runSendingClosingChannels()
{
	time_t timeDifference, minimumTimeDifference;
	ClosingChannel* closingChannel;

	while (!this->isInterrupted)
	{
		minimumTimeDifference = 1000;
		EnterCriticalSection(&this->CS_closingChannels);
		for (auto iterator = this->closingChannels.begin(); iterator != this->closingChannels.end(); )
		{
			closingChannel = *iterator;
			timeDifference = timeInMiliseconds() - closingChannel->lastAttempt;
			if (timeDifference < 1000)
			{
				if ((1000 - timeDifference) < minimumTimeDifference) minimumTimeDifference = 1000 - timeDifference;
				iterator++;
			}
			else
			{
				if (closingChannel->attempts >= 3)
				{

					iterator = this->closingChannels.erase(iterator);
				}
				else
				{
					send(closingChannel->channel->getDestinationAddress(), closingChannel->data);
					closingChannel->lastAttempt = timeInMiliseconds();
					closingChannel->attempts++;
					iterator++;
				}
			}
		}

		LeaveCriticalSection(&this->CS_closingChannels);

		Sleep(minimumTimeDifference);
	}
}



void Communication::runRefreshing()
{
	Connection* connection;
	char data[5];
	data[0] = 0;
	data[1] = 0;
	data[2] = REFRESHING;
	data[3] = 0;
	data[4] = 0;
	while (!this->isInterrupted)
	{
		AcquireSRWLockShared(&this->LOCK_connections); // LOCK
		for (auto iterator = this->connections.begin(); iterator != this->connections.end(); iterator++)
		{
			(*iterator)->refresh(data);
		}
		ReleaseSRWLockShared(&this->LOCK_connections); // UNLOCK

		Sleep(100);
	}
}



time_t Communication::timeInMiliseconds()
{
	SYSTEMTIME t;
	GetSystemTime(&t);
	return (time(0) * 1000 + t.wMilliseconds);
}


bool Communication::compareAddresses(sockaddr_in* first, sockaddr_in* second)
{
	return (first->sin_addr.S_un.S_addr == second->sin_addr.S_un.S_addr && first->sin_port == second->sin_port);
}



Connection* Communication::findConnection(sockaddr_in* destinationAddress)
{
	Connection* connection = NULL;
	/*unsigned int index;
	unsigned int size;
	unsigned int minIndex = 0;
	int maxIndex;*/

	AcquireSRWLockShared(&this->LOCK_connections); // LOCK
	auto iterator = findConnectionIterator(destinationAddress);
	if (iterator != this->connections.end() && compareAddresses((*iterator)->destinationAddress, destinationAddress)) connection = *iterator;
	/*
	size = this->connections.size();
	maxIndex = size - 1;

	if (size > 0)
	{
		while (minIndex < maxIndex)
		{
			index = (maxIndex + minIndex) / 2;
			if (this->connections[index]->destinationAddress->sin_addr.S_un.S_addr < destinationAddress->sin_addr.S_un.S_addr || (this->connections[index]->destinationAddress->sin_addr.S_un.S_addr == destinationAddress->sin_addr.S_un.S_addr && this->connections[index]->destinationAddress->sin_port < destinationAddress->sin_port))
			{
				minIndex = index + 1;
			}
			else if (this->connections[index]->destinationAddress->sin_addr.S_un.S_addr > destinationAddress->sin_addr.S_un.S_addr || (this->connections[index]->destinationAddress->sin_addr.S_un.S_addr == destinationAddress->sin_addr.S_un.S_addr && this->connections[index]->destinationAddress->sin_port > destinationAddress->sin_port))
			{
				maxIndex = index - 1;
			}
			else
			{
				connection = this->connections[index];
				break;
			}

			if (minIndex == maxIndex)
			{
				if (compareAddresses(this->connections[minIndex]->destinationAddress, destinationAddress)) connection = this->connections[minIndex];
				break;
			}
		}
	}*/

	/*for (auto iterator = this->connections.begin(); iterator != this->connections.end(); iterator++)
	{
		if (compareAddresses((*iterator)->destinationAddress, destinationAddress))
		{
			connection = *iterator;
			break;
		}
	}*/

	ReleaseSRWLockShared(&this->LOCK_connections); // UNLOCK

	return connection;
}



std::vector<Connection*>::iterator Communication::findConnectionIterator(sockaddr_in* destinationAddress)
{
	return std::lower_bound(this->connections.begin(), this->connections.end(), destinationAddress,
		[](const Connection* connection, sockaddr_in* address)
		{
			return (connection->destinationAddress->sin_addr.S_un.S_addr < address->sin_addr.S_un.S_addr || (connection->destinationAddress->sin_addr.S_un.S_addr == address->sin_addr.S_un.S_addr && connection->destinationAddress->sin_port < address->sin_port));
		});
}



bool Communication::addConnection(Connection* connection)
{
	AcquireSRWLockExclusive(&this->LOCK_connections); // LOCK
	auto iterator = findConnectionIterator(connection->destinationAddress);
	if (iterator == this->connections.end()) this->connections.push_back(connection);
	else if ((*iterator) == connection)
	{
		ReleaseSRWLockExclusive(&this->LOCK_connections); // UNLOCK
		return false;
	}
	else this->connections.insert(iterator, connection);
	ReleaseSRWLockExclusive(&this->LOCK_connections); // UNLOCK

	return true;
}



Connection* Communication::findAndEraseConnection(sockaddr_in* destinationAddress)
{
	Connection* connection = NULL;

	AcquireSRWLockExclusive(&this->LOCK_connections); // LOCK
	auto iterator = findConnectionIterator(destinationAddress);
	if (iterator != this->connections.end() && compareAddresses((*iterator)->destinationAddress, destinationAddress))
	{
		connection = *iterator;
		this->connections.erase(iterator);
	}
	/*
	size = this->connections.size();
	maxIndex = size - 1;

	if (size > 0)
	{
		while (minIndex < maxIndex)
		{
			index = (maxIndex + minIndex) / 2;
			if (this->connections[index]->destinationAddress->sin_addr.S_un.S_addr < destinationAddress->sin_addr.S_un.S_addr || (this->connections[index]->destinationAddress->sin_addr.S_un.S_addr == connection->destinationAddress->sin_addr.S_un.S_addr && this->connections[index]->destinationAddress->sin_port < connection->destinationAddress->sin_port))
			{
				minIndex = index + 1;
			}
			else if (this->connections[index]->destinationAddress->sin_addr.S_un.S_addr > destinationAddress->sin_addr.S_un.S_addr || (this->connections[index]->destinationAddress->sin_addr.S_un.S_addr == connection->destinationAddress->sin_addr.S_un.S_addr && this->connections[index]->destinationAddress->sin_port > connection->destinationAddress->sin_port))
			{
				maxIndex = index - 1;
			}
			else
			{
				connection = this->connections[index];
				break;
			}

			if (minIndex == maxIndex)
			{
				if (compareAddresses(this->connections[minIndex]->destinationAddress, destinationAddress)) connection = this->connections[minIndex];
				break;
			}

			if (minIndex == maxIndex)
			{
				if (compareAddresses(this->connections[minIndex]->destinationAddress, destinationAddress))
				{
					index = minIndex;
					connection = this->connections[index];
				}
				break;
			}
		}
		if (connection != NULL) this->connections.erase(this->connections.begin() + index);
	}*/

	/*
	for (auto iterator = this->connections.begin(); iterator != this->connections.end(); iterator++)
	{
		if (compareAddresses((*iterator)->destinationAddress, destinationAddress))
		{
			connection = *iterator;
			this->connections.erase(iterator);
			break;
		}
	}*/
	ReleaseSRWLockExclusive(&this->LOCK_connections); // UNLOCK

	return connection;
}