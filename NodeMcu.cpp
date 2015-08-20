/*
 * NodeMcu.cpp
 *
 *  Created on: 10.08.2015
 *      Author: sr
 */

#include "NodeMcu.h"

#include <stddef.h>
#include "WProgram.h"
#include "debug.h"

NodeMcu::NodeMcu(NtpCallback* callback) {
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
}

NodeMcu::~NodeMcu() {
}

void NodeMcu::begin() {
	Serial1.begin(9600);
	Serial1.write('\n');
}

void NodeMcu::start() {
	if( Serial1.available()) {
		serialDeviceDetected = true;
		while( Serial1.available()) Serial1.read();
		DPRINTF("serial device on uart1 detected.\n");
		// cmd echo off

		//sendCmd("node.restart()\r\n", false);

		sendCmd("uart.setup( 0,9600,8,0,1,0 )\r\n", false);
		//sendCmd("uart.setup( 0,115200,8,0,1,0 )\r\n", false);
		//Serial1.begin(115200);
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

Result* NodeMcu::getApList() {
	sendCmd(setModeStation, 0);
	sendCmd(getApListCmd, LIST_AP);
	while ( true) {
		update(millis());
		if (callState == RESULT_RECEIVED && correlation == LIST_AP){
			break;
		}
	}
	callState = IDLE;
	return lastResult;
}

void NodeMcu::configAp(const char* ssid, const char* password) {
	char buf[255];
	sprintf(buf, "wifi.sta.config(\"%s\", \"%s\")\r\n", ssid, password);
	sendCmd(buf, 0);
}

const char* getIpCmd =
		"ip = wifi.sta.getip() print() print(\"# \"..(ip and ip or \"nil\")) print(\"##\")\r\n";

char* NodeMcu::getIp() {
	sendCmd(getIpCmd, GET_IP);
	while ( true ) {
		update(millis());
		//delay(300);
		DPRINTF("wait for result\n");
		if (callState == RESULT_RECEIVED && correlation == GET_IP)
			break;
		if( callState == TIMEOUT) return "error";
	}
	callState = IDLE;
	DPRINTF("getIp result: %s\n", lastResult->line );
	return lastResult->line;
}

const char* getUtcFromNtpCmd = "ntp:sync(function(t) print() print( \"# \"..t.ustamp ) print \"##\" end)\r\n";

long NodeMcu::getUtcFromNtp() {
	sendCmd( getUtcFromNtpCmd, NTP_SYNC);
	return 0;
}

void NodeMcu::sendCmd(const char* cmd, int correlation) {
	const char* p = cmd;
	const char* p1 = cmd;

	if (callState == IDLE && serialDeviceDetected) {
		readState = READING_UNKNOWN;
		if( correlation != 0 ) callState = PENDING;
		clearResult();
		this->correlation = correlation;

		while (*p != 0) {
			callSend = millis();
			Serial1.write(*p);
			if (*p == '\n') { // line completed, wait for ack
				Serial1.flush();
				char line[256];
				memset(line, 0, 256);
				int len = p - p1;
				if( *(p-1)=='\r') len--;
				strncpy(line, p1, len);
				DPRINTF("send line: '%s'\n", line);
				p1 = p+1;
				do {
					readResponse();
					if (millis() > callSend + NODE_TIMEOUT) {
						DPRINTF("sendCmd runs into TIMEOUT\n");
						callState = TIMEOUT;
						return;
					}
				} while (readState != READING_PROMPT_SPC);
				readState = READING_UNKNOWN;
			}
			p++;
		}
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
		if( callState == PENDING ) callState = RESULT_RECEIVED;
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

	if( !ntpObjectSet && nodeMcuDetected && callState == IDLE ) {
		sendCmd(ntp,0);
		ntpObjectSet = true;
	}

	if( now < nextUpdate ) return;

	nextUpdate = now + 400;

	if( callState != IDLE ) DPRINTF("node::update: readState: %d, callState: %d\n", readState, callState);

	if (!nodeMcuDetected) {
		if (now > lasttimeChecked && callState == IDLE) {
			lasttimeChecked = now + 60000;
			sendCmd("majorVer, minorVer, devVer, chipid, flashid, flashsize, "
					"flashmode, flashspeed = node.info() "
					"print() print(\"# \"..minorVer..devVer) print(\"##\")\r\n",
					NODE_INFO);
		}
	}

	if( ntpObjectSet && now > nextNtpSync && callState == IDLE ) {
		nextNtpSync = now + 3600*1000*24; // one day
		sendCmd(getUtcFromNtpCmd,NTP_SYNC);
	}

	if (callState == PENDING) {
		while (readResponse()) {
			if (callState == PENDING && readState == READING_RESULT_END) {
				callState = RESULT_RECEIVED;
			}
		}
		if( now > callSend + 20000 ) {
			callState = TIMEOUT;
			timeoutOccured = now;
			DPRINTF("async timeout occured.\n");
		}
	}

	if (callState == TIMEOUT) {
		while (readResponse())
			;
		if( now > timeoutOccured + 120000 ) {
			DPRINTF("recover from timeout -> IDLE\n");
			callState = IDLE;
		}
	}

	if (callState == RESULT_RECEIVED && correlation == NODE_INFO) {
		if (strncmp(lastResult->line, "96", 2) == 0) {
			nodeMcuDetected = true;
			DPRINTF("node detected. version: %s\r\n",lastResult->line);
		}
		callState = IDLE;
		return;
	}

	if (callState == RESULT_RECEIVED && correlation == NTP_SYNC) {
		DPRINTF("utc timestamp received: %s\r\n", lastResult->line);
		uint32_t utc;
		sscanf(lastResult->line, "%uld",&utc);
		callback->setUtcTime(utc);
		callState = IDLE;
	}
}

void NodeMcu::requestNtpSync(uint32_t when) {
	nextNtpSync = when;
}

void NodeMcu::clearResult() {
	DPRINTF("clearResult\n");
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

// das muss immer gerufen werden können und darf nicht blockieren
// genauso wie das sync per ntp
// es muss im hintergrund passieren

void NodeMcu::checkNodeInfo() {
// unused
}
