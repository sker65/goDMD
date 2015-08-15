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

class NodeMcu {
public:
	NodeMcu();
	virtual ~NodeMcu();

	bool isNodeMcuDetected() const {
		return nodeMcuDetected;
	}

	void begin();

	Result* getApList();

	void configAp(char* name, char* password);

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

	void checkNodeInfo();

private:
	char lineBuffer[64];
	char* pLine;
	int sendCmd(const char* cmd, int correlation);
	void clearResult();
	Result* lastResult;
	bool nodeMcuDetected;
	uint32_t lasttimeChecked;
	int lastCh;
	ReadState readState;
	CallState callState;
	int correlation;
	uint32_t callSend;
	bool ntpObjectSet;
};

#define NODE_TIMEOUT 15000

// correlations for async calls
#define NODE_INFO 1
#define LIST_AP 2
#define NTP_SYNC 3
#define GET_IP 4

#endif /* NODEMCU_H_ */
