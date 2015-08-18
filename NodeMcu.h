/*
 * NodeMcu.h
 *
 *  Created on: 10.08.2015
 *      Author: sr
 */

#ifndef NODEMCU_H_
#define NODEMCU_H_

#include <stdint.h>

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

class NodeMcu {
public:
	bool isNodeMcuDetected() const {
		return nodeMcuDetected;
	}

	NodeMcu(NtpCallback* callback);
	virtual ~NodeMcu();
	void begin();

	Result* getApList();

	void configAp(const char* name, const char* password);

	char* getIp();

	long getUtcFromNtp();

	bool readResponse();

	enum ReadState {
		READING_UNKNOWN, READING_LF,
		READING_PROMPT_GT, READING_PROMPT_SPC,
		READING_RESULT_TAG, READING_RESULT_SPC,
		READING_RESULT_PAYLOAD, READING_RESULT_END
	};

	enum CallState {
		IDLE, 		// idle, call can be placed
		PENDING, 	// call is pending, result outstanding
		RESULT_RECEIVED, // result has received, but not yet processed
		TIMEOUT		// call timeout response was too late
	};

	void update(uint32_t now);

//private:
	void checkNodeInfo();
	bool nodeMcuDetected;

private:
	char lineBuffer[64];
	char* pLine;
	void sendCmd(const char* cmd, int correlation);
	void clearResult();
	Result* lastResult;
	uint32_t lasttimeChecked;
	int lastCh;
	ReadState readState;
	CallState callState;
	int correlation;
	uint32_t callSend;
	bool ntpObjectSet;
	NtpCallback* callback;
};

#define NODE_TIMEOUT 15000

// correlations for async calls
#define NODE_INFO 1
#define LIST_AP 2
#define NTP_SYNC 3
#define GET_IP 4

#endif /* NODEMCU_H_ */
