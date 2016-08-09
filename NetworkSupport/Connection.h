#pragma once
#include <list>
#include <atomic>
#include <winsock2.h>
#include <fstream>
#include <string>
#include <climits>
#include <chrono>

#include "Channel.h"
#include "Communication.h"
#include "Refreshing.h"
#include "DynamicArray.h"

#include "SendingConfirmatoryChannel.h"
#include "ReceivingConfirmatoryChannel.h"
#include "SendingMassChannel.h"
#include "ReceivingMassChannel.h"
#include "SendingStreamChannel.h"
#include "ReceivingStreamChannel.h"
class Communication;
class SendingConfirmatoryChannel;
class ReceivingConfirmatoryChannel;
class SendingMassChannel;
class ReceivingMassChannel;
class SendingStreamChannel;
class ReceivingStreamChannel;

//#define LOG


// Tøída vytváøí API aktivního spojení
class __declspec(dllexport) Connection
{
	friend class Communication;

public:
	Connection(Communication*, sockaddr_in*, bool isPrimary);
	~Connection();
	Communication* getCommunication();
	void disconnect();
	bool isConnected();
	void refresh(char* data);
	void processRefresh(unsigned short id);
	time_t getLastActivity();

	virtual void onCreate() = 0;
	void createChannel(unsigned char channelType, unsigned short dataType);
	void closeChannel(Channel* channel);
	void processClosingChannel(unsigned short sourceChannelNumber);
	void processClosingChannel(Channel* channel);
	void processCreateChannel(unsigned char channelType, unsigned short dataType, unsigned short sourceChannelNumber, unsigned short destinationChannelNumber);
	virtual Channel* creatingChannel(bool side, unsigned char channelType, unsigned short dataType, unsigned short* priority, unsigned short sourceChannelNumber, unsigned short destinationChannelNumber);
	virtual void processCreateChannel(SendingConfirmatoryChannel** channel, unsigned short dataType, unsigned short* priority) { };
	virtual void processCreateChannel(ReceivingConfirmatoryChannel** channel, unsigned short dataType, unsigned short* priority) { };
	virtual void processCreateChannel(SendingMassChannel** channel, unsigned short dataType, unsigned short* priority) { };
	virtual void processCreateChannel(ReceivingMassChannel** channel, unsigned short dataType, unsigned short* priority) { };
	virtual void processCreateChannel(SendingStreamChannel** channel, unsigned short dataType, unsigned short* priority) { };
	virtual void processCreateChannel(ReceivingStreamChannel** channel, unsigned short dataType, unsigned short* priority) { };
	bool getData(Data** data);
	void processReceivedData(Data* data, short channelNumber);
	void notifyDataReady();
    
	sockaddr_in* destinationAddress;
    int attemptNumberForReviving;

	#ifdef LOG
	LOG std::ofstream log;
	#endif
protected:
	std::vector<Channel*>::iterator findPriorityChannelIterator(int priority, unsigned short channelNumber);
	unsigned short generateChannelNumber();
	void recountSpeed();
	static void start(void*);
	void run();
	unsigned int getAndRecountRealSpeed();

	HANDLE thread;
	//std::vector<Channel*> channels;
	DynamicArray<Channel*> channels;
	unsigned short channelCount;
	std::vector<Channel*>* priorityChannels;
	std::vector<unsigned short> startInPriorityChannels;
	std::vector<unsigned short> channelNumbers;

	Communication* communication;
	std::list<Refreshing*> listOfRefreshing;
	SRWLOCK LOCK_channels;
	SRWLOCK LOCK_channelNumbers;
	CONDITION_VARIABLE CV_sending;
	CONDITION_VARIABLE CV_pausing;
	CRITICAL_SECTION CS_pausing;
	CRITICAL_SECTION CS_sending;
	CRITICAL_SECTION CS_reservedChannelNumbers;
	CRITICAL_SECTION CS_listOfRefreshing;
	CRITICAL_SECTION CS_latency;
	CRITICAL_SECTION CS_realSpeed;
	//std::mutex lock_latency;
	std::atomic<unsigned int> speed;
	std::atomic<bool> areDataAlreadyReady;
	time_t lastActivity;
    bool connected;
	//bool primary; // pokud zrušit, tak i metodu
	unsigned short nextRefreshId;

	unsigned short echoInterval;
	std::atomic<unsigned short> nextSentId;
	std::atomic<unsigned short> lastReceivedId;
	time_t lastSendingTime;
	unsigned short latency[3];
	short pointerToLatency;
	unsigned short newestLatency;
	unsigned int basicSpeed;
	std::atomic<bool> isLatencyRecountRequired;

	// Mechanismus záchrany
	unsigned short localMinLatency;

	// Mechanismus poslední záchrany
	unsigned short globalMinLatency;
	std::atomic<bool> isProblem;
	time_t timeOfProblem;

	/*
	unsigned int oldLatencySum;
	unsigned int newLatencySum;
	unsigned int oldLatencyCount;
	unsigned int newLatencyCount;
	time_t latencyAverageStart;
	*/

	unsigned int lengthSum;
	time_t startTime;
	unsigned int realSpeed;

	std::atomic<bool> isInterrupted;

	// KONSTANTY NASTAVENÍ
	float UPPER_LIMIT_RATIO_FOR_SLOWDOWN = 0.9F;
	float SPEED_MULTIPLIER_FOR_SPEEDUP = 1.1F;
	unsigned int MIN_SPEED = 20000;
	time_t MAX_TIME_TO_WAIT = 5000;
	unsigned short PRIORITY_LEVELS = 3;

private:
	time_t session;
};

