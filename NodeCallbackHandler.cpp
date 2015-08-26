/*
 * NodeCallbackHandler.cpp
 *
 *  Created on: 25.08.2015
 *      Author: sr
 */

#include <stdio.h>
#include "NodeCallbackHandler.h"
#include "LEDMatrixPanel.h"
#include "debug.h"

NodeCallbackHandler::NodeCallbackHandler(LEDMatrixPanel* panel)
	: panel(panel) {
}

const char* nodeStates[] = {
	//   0123456789012345
		"IDLE",
		"CONNECTING",
		"WRONG_PASSWORD",
		"NO_AP_FOUND",
		"CONNECT_FAIL",
		"GOT_IP"
};


void NodeCallbackHandler::notify(NodeMcu* node, int type) {
	uint8_t status;
	switch(type) {
	case GET_STATUS:
		status = node->getStatus();
		if( status > 5 ) status = 0;
		DPRINTF("node call back: %d -> %s\n", status, nodeStates[status]);
		panel->writeText(nodeStates[status],0,0,16);
		break;
	case GET_IP:
		panel->writeText(node->getIp(),0,8,16);
		break;
	}
}
