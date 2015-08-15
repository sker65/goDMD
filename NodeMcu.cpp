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

NodeMcu::NodeMcu() {
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
}

NodeMcu::~NodeMcu() {
}

void NodeMcu::begin() {
	Serial1.begin(9600);
	// cmd echo off
	sendCmd("setup(0,9600,8,0,1,0)\r\n", false);
}

const char* setModeStation = "wifi.setmode(wifi.STATION)\r\n";

const char* ntp =
		"ntp = ({\n"
				"    hour=0,\n"
				"    minute=0,\n"
				"    second=0,\n"
				"    lastsync=0,\n"
				"    ustamp=0,\n"
				"    tz=1,\n"
				"    udptimer=2,\n"
				"    udptimeout=1000,\n"
				"    ntpserver=\"194.109.22.18\",\n"
				"    sk=nil,\n"
				"    sync=function(self)\n"
				"        local request=string.char( 227, 0, 6, 236, 0,0,0,0,0,0,0,0, 49, 78, 49, 52,\n"
				"        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0\n"
				"        )\n"
				"        self.sk=net.createConnection(net.UDP, 0)\n"
				"        self.sk:on(\"receive\",function(sck,payload) \n"
				"            sck:close()\n"
				"            self.lastsync=self:calc_stamp(payload:sub(41,44))\n"
				"            self:set_time()\n"
				"            collectgarbage() collectgarbage()\n"
				"        end)\n"
				"        self.sk:connect(123,self.ntpserver)\n"
				"        tmr.alarm(self.udptimer,self.udptimeout,0,function() self.sk:close() end)\n"
				"        self.sk:send(request)\n"
				"    end,\n"
				"    calc_stamp=function(self,bytes)\n"
				"        local highw,loww,ntpstamp\n"
				"        highw = bytes:byte(1) * 256 bytes:byte(2)\n"
				"        loww = bytes:byte(3) * 256 bytes:byte(4)\n"
				"        ntpstamp=( highw * 65536 loww ) ( self.tz * 3600)\n"
				"        self.ustamp=ntpstamp - 1104494400 - 1104494400\n"
				"        return(self.ustamp)\n"
				"    end,\n"
				"    set_time=function(self)\n"
				"        self.hour = self.ustamp % 86400 / 3600\n"
				"        self.minute = self.ustamp % 3600 / 60\n"
				"        self.second = self.ustamp % 60\n"
				"    end,\n"
				"})\n";

const char* getApListCmd =
		"wifi.sta.getap(function(t) for k,v in pairs(t) do print(\"# \"..k) end print(\"##\") end)\r\n";

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

void NodeMcu::configAp(char* ssid, char* password) {
	char buf[255];
	sprintf(buf, "wifi.sta.config(\"%s\", \"%s\")\r\n", ssid, password);
	sendCmd(buf, 0);
}

const char* getIpCmd =
		"ip = wifi.sta.getip() print(\"# \"..ip) print(\"##\")\r\n";

char* NodeMcu::getIp() {
	sendCmd(getIpCmd, GET_IP);
	while ( true) {
		update(millis());
		if (callState == RESULT_RECEIVED && correlation == GET_IP)
			break;
	}
	callState = IDLE;
	return lastResult->line;
}

const char* getUtcFromNtpCmd = "ntp:sync(function(t) print( \"# \" .. t.ustamp ) print \"##\" end)\r\n";

long NodeMcu::getUtcFromNtp() {
	sendCmd( getUtcFromNtpCmd, NTP_SYNC);
	return 0;
}

int NodeMcu::sendCmd(const char* cmd, int correlation) {
	const char* p = cmd;
	const char* p1 = cmd;

	if (callState == IDLE) {
		clearResult();
		while (*p != 0) {
			callSend = millis();
			Serial1.write(*p);
			if (*p == '\n') { // line completed, wait for ack
				char line[64];
				memset(line, 0, 64);
				strncpy(line, p1, p - p1);
				DPRINTF("send line: '%s'", line);
				p1 = p + 1;
				do {
					readResponse();
					if (millis() > callSend + NODE_TIMEOUT) {
						callState = TIMEOUT;
						return 0;
					}
				} while (readState != READING_PROMPT_SPC);
			}
			p++;
		}
		if( correlation != 0 ) callState = PENDING;
		return correlation;
	} else {
		return 0;
	}
}

bool NodeMcu::readResponse() {
	if (!Serial1.available())
		return false;
	int ch = Serial1.read();
	DPRINTF("read: %c, %i\n", ch, readState);
	switch (readState) {
	case READING_UNKNOWN:
		if (ch == '\n' || ch == '\r')
			readState = READING_LF;
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
		break;
	case READING_RESULT_PAYLOAD:
		if (ch == '\n' || ch == '\r') {
			readState = READING_LF;
			*pLine = 0;
			Result* p = lastResult;
			while (p != NULL)
				p = p->next;
			p = (Result*) malloc(sizeof(Result));
			p->line = (char*) malloc(strlen(lineBuffer));
			strcpy(p->line, lineBuffer);
			break;
		}
		if (pLine - lineBuffer < 63) {
			*pLine++ = ch;
		}
		break;
	case READING_RESULT_END:
		break;
	}
	return true;
}

void NodeMcu::update(uint32_t now) {
	if( !ntpObjectSet && nodeMcuDetected && callState == IDLE ) {
		sendCmd(ntp,0);
		ntpObjectSet = true;
	}

	if (callState == PENDING) {
		while (readResponse()) {
			if (callState == PENDING && readState == READING_RESULT_END) {
				callState = RESULT_RECEIVED;
			}
		}
	}

	if (callState == TIMEOUT) {
		while (readResponse())
			;
	}

	if (callState == RESULT_RECEIVED && correlation == NODE_INFO) {
		if (strncmp(lastResult->line, "96", 2) == 0) {
			nodeMcuDetected = true;
			DPRINTF("node detected. version: %s\r\n",lastResult->line);
		}
		callState = IDLE;
	}

	if (callState == RESULT_RECEIVED && correlation == NTP_SYNC) {
		DPRINTF("utc timestamp received: %s\r\n", lastResult->line);
		callState = IDLE;
	}
}

void NodeMcu::clearResult() {
	DPRINTF("clearResult\n");
	Result* p = lastResult;
	while (p != NULL) {
		Result* current = p;
		p = p->next;
		DPRINTF("free line: %s\n", current->line);
		free(current->line);
		free(current);
	}
}

// das muss immer gerufen werden können und darf nicht blockieren
// genauso wie das sync per ntp
// es muss im hintergrund passieren

void NodeMcu::checkNodeInfo() {
	unsigned long now = millis();
	if (!nodeMcuDetected) {
		if (now > lasttimeChecked && callState == IDLE) {
			lasttimeChecked = now + 60000;
			sendCmd("majorVer, minorVer, devVer, chipid, flashid, flashsize, "
					"flashmode, flashspeed = node.info() "
					"print(\"# \"..minorVer..devVer) print(\"##\"\r\n",
					NODE_INFO);
		}
	}
}
