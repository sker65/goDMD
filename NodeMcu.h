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
		IDLE, PENDING, RESULT_RECEIVED
	};

	void update(uint32_t now);

	void checkNodeInfo();

private:
	char lineBuffer[64];
	char* pLine;
	bool sendCmd(const char* cmd, bool parseResult);
	void clearResult();
	Result* lastResult;
	bool nodeMcuDetected;
	long lasttimeChecked;
	int lastCh;
	ReadState readState;
	CallState callState;
};

#endif /* NODEMCU_H_ */
