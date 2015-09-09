/*
 * NodeMcu.cpp
 *
 *  Created on: 10.08.2015
 *      Author: sr
 */

#include "NodeMcu.h"

#include <stddef.h>
#include <string.h>

#include "WProgram.h"
#include "debug.h"

NodeMcu::NodeMcu(NtpCallback* callback, NodeNotifyCallback* notifyCallback) {
	lasttimeChecked = 0L;
	nodeMcuDetected = false;
	lastResult = NULL;
	lastCh = 0;
	readState = READING_UNKNOWN;
	pLine = lineBuffer;
	callState = IDLE;
	correlation = 0;
	callSend = 0;
	ntpObjectSet = false;
	this->callback = callback;
	serialDeviceDetected = false;
	nextNtpSync = 1<<31;
	nextUpdate = 0;
	cmdBuffer = NULL;
	ip[0]='\0';
	this->notifyCallback = notifyCallback;
	lastStatusCheck = millis() + 15000;
	enableBackgroundCmds = true;
}

NodeMcu::~NodeMcu() {
}

void NodeMcu::begin() {
	DPRINTF2("node::begin: starting serial1\n");
	Serial1.begin(9600);
	Serial1.write('\n');
}

void NodeMcu::start() {
	delay(200);
	DPRINTF2("node::start: serial1 probing\n");
	if( Serial1.available()) {
		serialDeviceDetected = true;
		while( Serial1.available()) Serial1.read();
		DPRINTF2("serial device on uart1 detected.\n");
		// cmd echo off

		//sendCmd("node.restart()\r\n", false);

		sendCmd("uart.setup( 0,9600,8,0,1,0 )\r\n", 0);
		//Serial1.begin(115200);
	} else {
		DPRINTF2("NO serial device detected.\n");
	}
}

const char* setModeStation = "wifi.setmode(wifi.STATION)\r\n";

const char* ntp =
		    "ntp = ({\n"
		    "hour=0,\n"
			"minute=0,\n"
			"second=0,\n"
			"lastsync=0,\n"
			"ustamp=0,\n"
			"tz=0,\n"
			"udptimer=2,\n"
			"udptimeout=1000,\n"
			"ntpserver=\"130.149.17.8\",\n"
			"sk=nil,\n"
			"sync=function(self,callback)\n"
			"	local request=string.char( 227, 0, 6, 236, 0,0,0,0,0,0,0,0, 49, 78, 49, 52,\n"
			"	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0\n"
			"	)\n"
			"	self.sk=net.createConnection(net.UDP, 0)\n"
			"	self.sk:on(\"receive\",function(sck,payload)\n"
			"		sck:close()\n"
			"		self.lastsync=self:calc_stamp(payload:sub(41,44))\n"
			"		self:set_time()\n"
			"		if callback and type(callback) == \"function\" then\n"
			"			callback(self)\n"
			"		end\n"
			"		collectgarbage() collectgarbage()\n"
			"	end)\n"
			"	self.sk:connect(123,self.ntpserver)\n"
			"	tmr.alarm(self.udptimer,self.udptimeout,0,function() self.sk:close() end)\n"
			"	self.sk:send(request)\n"
			"end,\n"
			"calc_stamp=function(self,bytes)\n"
			"	local highw,loww,ntpstamp\n"
			"	highw = bytes:byte(1) * 256 + bytes:byte(2)\n"
			"	loww = bytes:byte(3) * 256 + bytes:byte(4)\n"
			"	ntpstamp=( highw * 65536 + loww ) + ( self.tz * 3600)\n"
			"	self.ustamp=ntpstamp - 1104494400 - 1104494400\n"
			"	return(self.ustamp)\n"
			"end,\n"
			"set_time=function(self)\n"
			"	self.hour = self.ustamp % 86400 / 3600\n"
			"	self.minute = self.ustamp % 3600 / 60\n"
			"	self.second = self.ustamp % 60\n"
			"end,\n"
		"})\n";

const char* getApListCmd =
		"wifi.sta.getap(function(t) print() for k,v in pairs(t) do print(\"# \"..k) end print(\"##\") end)\r\n";

void NodeMcu::waitFor(int corr) {
	while ( true) {
		update(millis());
		if (callState == IDLE && this->correlation == corr){
			break;
		}
	}
}

Result* NodeMcu::getApList() {
	sendCmd(setModeStation, 0);
	sendCmd(getApListCmd, LIST_AP);
	waitFor(LIST_AP);
	return lastResult;
}

void NodeMcu::configAp(char* ssid, char* password) {
	char buf[255];
	sendCmd(setModeStation, 0);
	sprintf(buf, "wifi.sta.config(\"%s\", \"%s\")\r\n", ssid, password);
	sendCmd(buf, 0);
}

const char* getIpCmd =
		"ip = wifi.sta.getip() print() print(\"# \"..(ip and ip or \"nil\")) print(\"##\")\r\n";

const char* getStatusCmd =
		"status = wifi.sta.status() print() print(\"# \"..status) print(\"##\")\r\n";

void NodeMcu::requestStatus() {
	sendCmd(getStatusCmd, GET_STATUS);
}

void NodeMcu::requestIp() {
	sendCmd(getIpCmd, GET_IP);
}


char* NodeMcu::syncRequestIp() {
	requestIp();
	waitFor(GET_IP);
	return ip;
}

const char* getUtcFromNtpCmd = "ntp:sync(function(t) print() print( \"# \"..t.ustamp ) print \"##\" end)\r\n";

long NodeMcu::getUtcFromNtp() {
	sendCmd( getUtcFromNtpCmd, NTP_SYNC);
	return 0;
}

void NodeMcu::sendCmdLine() {
	while (*pSendCmd != 0) {
		callSend = millis();
		Serial1.write(*pSendCmd);
		if (*pSendCmd == '\n') { // line completed, wait for ack
			Serial1.flush();
			char line[256];
			memset(line, 0, 256);
			int len = pSendCmd - p1SendCmd;
			if( *(pSendCmd-1)=='\r') len--;
			strncpy(line, p1SendCmd, len);
			DPRINTF("send line: '%s'\n", line);
			p1SendCmd = pSendCmd+1;
			do {
				readResponse();
				if (millis() > callSend + NODE_TIMEOUT) {
					DPRINTF2("sendCmd runs into TIMEOUT\n");
					setCallState(TIMEOUT);
					return;
				}
			} while (readState != READING_PROMPT_SPC);
			if( *(pSendCmd+1) != 0 ) {
				DPRINTF2("multiline cmd continue ...\n");
				pSendCmd++;
				readState = READING_UNKNOWN;
				return;  // more to come
			}
			break;
		} // line end
		pSendCmd++;
	} // while (cmd end)
	if( cmdBuffer != NULL ) {
		free( (void*)cmdBuffer );
		cmdBuffer = NULL;
	}
	if (callState == SENDING) {
		if (correlation != 0) {
			setCallState(PENDING);
		} else {
			setCallState(IDLE);
		}
	}
}


void NodeMcu::sendCmd(const char* cmd, int correlation) {

	if (callState == IDLE && serialDeviceDetected) {

		if( cmdBuffer != NULL ) free( (void*)cmdBuffer );
		cmdBuffer =  (char*) malloc( strlen(cmd) + 1);
		strcpy( cmdBuffer, cmd);

		pSendCmd = cmdBuffer;
		p1SendCmd = cmdBuffer;

		readState = READING_UNKNOWN;
		setCallState(SENDING);
		clearResult();
		this->correlation = correlation;
		sendCmdLine();
	} else {
		this->correlation = 0;
	}
}

bool NodeMcu::readResponse() {
	if (!Serial1.available())
		return false;
	int ch = Serial1.read();
	ReadState old = readState;

	switch (readState) {
	case READING_UNKNOWN:
		if (ch == '\n' || ch == '\r')
			readState = READING_LF;
		if (ch == '>')
			readState = READING_PROMPT_GT;
		break;
	case READING_LF:
		if (ch == '\n' || ch == '\r')
			break;
		if (ch == '>')
			readState = READING_PROMPT_GT;
		if (ch == '#')
			readState = READING_RESULT_TAG;
		break;
	case READING_PROMPT_GT:
		if (ch == ' ')
			readState = READING_PROMPT_SPC;
		break;
	case READING_PROMPT_SPC:
		readState = READING_UNKNOWN;
		if (ch == '\n' || ch == '\r')
			readState = READING_LF;
		break;
	case READING_RESULT_TAG:
		if (ch == ' ')
			readState = READING_RESULT_SPC;
		if (ch == '#')
			readState = READING_RESULT_END;
		break;
	case READING_RESULT_SPC:
		readState = READING_RESULT_PAYLOAD;
		pLine = lineBuffer;
		*pLine++ = ch;
		break;
	case READING_RESULT_PAYLOAD:
		if (ch == '\n' || ch == '\r') {
			readState = READING_LF;
			*pLine = 0;
			// construct new element
			Result* r = (Result*) malloc(sizeof(Result));
			r->line = (char*) malloc(strlen(lineBuffer)+1);
			strcpy(r->line, lineBuffer);
			r->next = lastResult;
			lastResult = r;
			break;
		}
		if (pLine - lineBuffer < 63) {
			*pLine++ = ch;
		}
		break;
	case READING_RESULT_END:
		if( callState == PENDING || callState == SENDING ) setCallState(RESULT_RECEIVED);
		if (ch == '\n' || ch == '\r') {
			readState = READING_LF;
		} else {
			readState = READING_UNKNOWN;
		}
		break;
	}
	if( ch>=32 ) DPRINTF("read: '%c', %i->%i\n", ch, old, readState);
	else DPRINTF("read: 0x%02x, %i->%i\n", (unsigned char)ch, old, readState);

	return true;
}

void NodeMcu::update(uint32_t now) {

	// complete sending a multi line commmand
	if( callState == SENDING ) {
		sendCmdLine();
		return;
	}

	if( now < nextUpdate ) return;

	nextUpdate = now + 400;

	if( isEnableBackgroundCmds() ) {
		if( !ntpObjectSet && nodeMcuDetected && callState == IDLE ) {
			sendCmd(ntp,0);
			// send not completed here
			ntpObjectSet = true;
		}

		if( callState == IDLE && nodeMcuDetected && now > lastStatusCheck ) {
			requestStatus();
			lastStatusCheck = now + 60000;
		}

		if( callState == IDLE && nodeMcuDetected && now > lastHeapCheck ) {
			requestHeap();
			lastHeapCheck = now + 120000;
		}

		if( callState == IDLE && nodeMcuDetected && status == 5 && strlen(ip)==0 ) {
			requestIp();
		}

		if( ntpObjectSet && now > nextNtpSync && callState == IDLE && status == 5 ) {
			nextNtpSync = now + 3600*1000*24; // one day
			sendCmd(getUtcFromNtpCmd,NTP_SYNC);
		}
	}

	if( callState != IDLE ) DPRINTF("readState: %d, callState: %d\n", readState, callState);

	if (!nodeMcuDetected) {
		if (now > lasttimeChecked && callState == IDLE) {
			lasttimeChecked = now + 60000;
			sendCmd("majorVer, minorVer, devVer, chipid, flashid, flashsize, "
					"flashmode, flashspeed = node.info() "
					"print() print(\"# \"..minorVer..devVer) print(\"##\")\r\n",
					NODE_INFO);
		}
	}

	if (callState == PENDING) {
		while (readResponse()) {
			if (callState == PENDING && readState == READING_RESULT_END) {
				setCallState(RESULT_RECEIVED);
			}
		}
		if( now > callSend + 20000 ) {
			setCallState(TIMEOUT);
			timeoutOccured = now;
			DPRINTF2("async timeout occured.\n");
		}
	}

	if (callState == TIMEOUT) {
		while (readResponse())
			;
		if( now > timeoutOccured + 120000 ) {
			DPRINTF2("recover from timeout -> IDLE\n");
			setCallState(IDLE);
		}
	}

	if (callState == RESULT_RECEIVED ) {
		uint8_t newStatus = 0;
		switch( correlation ) {
		case NODE_INFO:
			if (strncmp(lastResult->line, "96", 2) == 0) {
				nodeMcuDetected = true;
				DPRINTF("node detected. version: %s\r\n",lastResult->line);
			}
			if( notifyCallback ) notifyCallback->notify(this, correlation);
			break;
		case NTP_SYNC:
			DPRINTF("utc timestamp received: %s\r\n", lastResult->line);
			uint32_t utc;
			sscanf(lastResult->line, "%lu",&utc);
			callback->setUtcTime(utc);
			break;
		case GET_IP:
			DPRINTF("getip received: %s\n", lastResult->line);
			strncpy(ip,lastResult->line,16);
			if( notifyCallback ) notifyCallback->notify(this, correlation);
			break;
		case GET_STATUS:
			sscanf(lastResult->line, "%hhu",&newStatus);
			if( newStatus != status ) {
				status = newStatus;
				if( notifyCallback ) notifyCallback->notify(this, correlation);
			}
			DPRINTF("received status: %d\n",status);
			break;
		case GET_HEAP:
			DPRINTF("node heap: %s\n", lastResult->line);
			break;
		}
		setCallState(IDLE);
		return;
	}
}

void NodeMcu::requestNtpSync(uint32_t when) {
	nextNtpSync = when;
}

const char* CALLSTATES[] = { "IDLE", "SENDING", "PENDING", "RESULT_RECEIVED", "TIMEOUT" };

void NodeMcu::setCallState(CallState callState) {
	DPRINTF("callState: %s -> %s\n", CALLSTATES[this->callState],
			CALLSTATES[callState]);
	this->callState = callState;
}

const char* getHeapCmd =
		"heap = node.heap() print() print(\"# \"..heap) print(\"##\")\r\n";

void NodeMcu::requestHeap() {
	sendCmd(getHeapCmd, GET_HEAP);
}

void NodeMcu::clearResult() {
	DPRINTF2("clearResult\n");
	Result* p = lastResult;
	while (p != NULL) {
		Result* current = p;
		p = current->next;
		DPRINTF("free line: %s\n", current->line);
		free(current->line);
		free(current);
	}
	lastResult = NULL;
}

