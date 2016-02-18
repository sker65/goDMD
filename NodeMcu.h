/*
 * NodeMcu.h
 *
 *  Created on: 10.08.2015
 *      Author: sr
 */

#ifndef NODEMCU_H_
#define NODEMCU_H_

#include <stdint.h>

class NodeMcu;

class Result {
public:
	char* line;
	Result* next;
};

class NtpCallback {
public:
	virtual ~NtpCallback(){};
	virtual void setUtcTime(uint32_t utcTime) = 0;
};

class NodeNotifyCallback {
public:
	virtual ~NodeNotifyCallback(){};
	virtual void notify(NodeMcu* node, int type) = 0;
};

class NodeMcu {
public:
	bool isNodeMcuDetected() const {
		return nodeMcuDetected;
	}

	NodeMcu(NtpCallback* callback, NodeNotifyCallback* notifyCallback);
	virtual ~NodeMcu();
	void begin();
	void start();

	Result* getApList();

	void configAp(char* name, char* password);

	void requestIp();
	char* syncRequestIp();

	void requestStatus();
	void requestHeap();

	long getUtcFromNtp();

	bool readResponse();
	void waitFor(int correlation);

	enum ReadState {
		READING_UNKNOWN, READING_LF,
		READING_PROMPT_GT, READING_PROMPT_SPC,
		READING_RESULT_TAG, READING_RESULT_SPC,
		READING_RESULT_PAYLOAD, READING_RESULT_END
	};

	enum CallState {
		IDLE, 		// idle, call can be placed
		SENDING,
		PENDING, 	// call is pending, result outstanding
		RESULT_RECEIVED, // result has received, but not yet processed
		TIMEOUT		// call timeout response was too late
	};

	void update(uint32_t now);

	void requestNtpSync(uint32_t when);

//private:

	void setCallState(CallState callState);

	uint8_t getStatus() const {
		return status;
	}

	const char* getIp() const {
		return ip;
	}

	bool isEnableBackgroundCmds() const {
		return enableBackgroundCmds;
	}

	void setEnableBackgroundCmds(bool enableBackgroundCmds) {
		this->enableBackgroundCmds = enableBackgroundCmds;
	}

	bool nodeMcuDetected;

private:
	char lineBuffer[64];
	char* pLine;
	void sendCmd(const char* cmd, int correlation);
	void sendCmdLine();
	void clearResult();
	Result* lastResult;
	uint32_t lasttimeChecked;
	uint32_t nextUpdate;
	uint32_t nextNtpSync;
	int lastCh;
	ReadState readState;
	CallState callState;
	int correlation;

	uint32_t callSend;
	uint32_t timeoutOccured;

	bool ntpObjectSet;
	bool udpServer;
	NtpCallback* callback;
	bool serialDeviceDetected;

	const char* pSendCmd;
	const char* p1SendCmd;
    char* cmdBuffer;

    char ip[16];
    uint8_t status; // see wifi.sta.getStatus
    NodeNotifyCallback* notifyCallback;

    uint32_t lastStatusCheck;
    uint32_t lastHeapCheck;

    bool enableBackgroundCmds;

};

#define NODE_TIMEOUT 5000

// correlations for async calls
#define NODE_INFO 1
#define LIST_AP 2
#define NTP_SYNC 3
#define GET_IP 4
#define GET_STATUS 5
#define GET_HEAP 6

#endif /* NODEMCU_H_ */
