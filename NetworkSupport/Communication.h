#pragma once
#include <winsock2.h>
#include <vector>
#include <list>
#include <deque>
#include <stdlib.h>
#include <time.h>
#include <iostream>

#include "NetworkError.h"
#include "Datagram.h"
#include "Data.h"
#include "DataBuilder.h"
#include "ConfirmatoryDataBuilder.h"
#include "DisposableDataBuilder.h"
#include "FlexibleDataBuilder.h"
#include "DisposableFlexibleDataBuilder.h"
#include "FlexibleConfirmatoryDataBuilder.h"
#include "Connection.h"
#include "Channel.h"
#include "SendingConfirmatoryChannel.h"
#include "ReceivingConfirmatoryChannel.h"
#include "SendingStreamChannel.h"
#include "ReceivingStreamChannel.h"
#include "SendingMassChannel.h"
#include "ReceivingMassChannel.h"
#include "Invitation.h"
#include "EstablishingOrClosingConnection.h"
#include "ChannelEstablishment.h"
#include "ClosingChannel.h"
#include "ReceivedDatagram.h"
#include "Processing.h"
#include "Receiving.h"
#include "AddToRefresh.h"

#pragma comment(lib, "ws2_32.lib")

class Receiving;
class Processing;



// T¯Ìda vytv·¯Ì API, kter˝m lze ¯Ìdit cel˝ sÌùov˝ balÌk
class __declspec(dllexport) Communication
{
public:
	Communication();
	Communication(unsigned short);
	~Communication();
	void close();
	void sendInvitation(sockaddr_in*);
	void stopSendingInvitation(sockaddr_in*);
	void connect(sockaddr_in*);
	void disconnect(Connection*);
	void addChannelEstablishment(ChannelEstablishment* channelEstablishment);
	void addClosingChannel(ClosingChannel* closingChannel);
	virtual void processCreatingConnection(Connection** connection, sockaddr_in* sourceAddress, bool isPrimary) = 0;
	virtual void processDisconnection(Connection*) = 0;
	void processReceivedDatagram(Datagram*);
	void send(sockaddr_in*, char*, unsigned short);
	void send(sockaddr_in*, Data* data);
	void send(Datagram*);
	void sendControlInformation(sockaddr_in*, unsigned char);
	virtual void unableToConnect(sockaddr_in*) = 0;
	static time_t timeInMiliseconds();
	static bool compareAddresses(sockaddr_in*, sockaddr_in*);

	
	// Konstanty pro typy ¯ÌdÌcÌch informacÌ
	static const enum ControlInformationType
	{
		INVITATION,
		CONNECTION,
		CONNECTION_CONFIRMATION,
		CHANNEL_ESTABLISHMENT,
		CHANNEL_ESTABLISHMENT_CONFIRMATION,
		CLOSING_CHANNEL,
		CLOSING_CHANNEL_CONFIRMATION,
		REFRESHING,
		REFRESHING_CONFIRMATION,
		DISCONNECTION,
		DISCONNECTION_CONFIRMATION
	};

	
	const static short DATAGRAM_SIZE = 1444;
	const static short INACTIVE_TIME = 5000;

private:
	void init(sockaddr_in&);
	Connection* findConnection(sockaddr_in*);
	bool addConnection(Connection*);
	Connection* findAndEraseConnection(sockaddr_in*);
	std::vector<Connection*>::iterator findConnectionIterator(sockaddr_in*);
	
	static void startSendingInvitations(void*);
	static void startSendingConnections(void*);
	static void startSendingDisconnections(void*);
	static void startRefreshing(void*);
	static void startChannelEstablishments(void*);
	static void startClosingChannels(void*);
	void runRefreshing();
	void runSendingInvitations();
	void runSendingConnections();
	void runSendingDisconnections();
	void runSendingChannelEstablishments();
	void runSendingClosingChannels();

	SOCKET Socket;
	HANDLE threadRefreshing;
	HANDLE threadSendingInvitations;
	HANDLE threadSendingConnections;
	HANDLE threadSendingDisconnections;
	HANDLE threadSendingChannelEstablishments;
	HANDLE threadSendingClosingChannels;
	Receiving* receiving;
	Processing* processing;
	std::atomic<bool> isInterrupted;
    //ReceivingPackets receivingPackets;
    
    std::vector<Connection*> connections;
	std::list<sockaddr_in*> invitations;
    std::list<EstablishingConnection*> establishingConnections;
    std::list<Connection*> closingConnections;
	std::list<ChannelEstablishment*> channelEstablishments;
	std::list<ClosingChannel*> closingChannels;
	//std::list<Datagram> datagramsToSend;

	CONDITION_VARIABLE* CV_processing;

	SRWLOCK LOCK_connections;

	// Critical sections
	CRITICAL_SECTION CS_initialReceivedDatagram;
	CRITICAL_SECTION CS_processing;
	CRITICAL_SECTION CS_establishingConnections;
	CRITICAL_SECTION CS_invitations;
	CRITICAL_SECTION CS_closingConnections;
	CRITICAL_SECTION CS_channelEstablishments;
	CRITICAL_SECTION CS_closingChannels;
	CRITICAL_SECTION CS_sending;
};

