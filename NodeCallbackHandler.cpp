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
	switch(type) {
	case GET_STATUS:
		DPRINTF("node call back: %d -> %s\n", node->getStatus(), nodeStates[node->getStatus()]);
		panel->writeText(nodeStates[node->getStatus()],0,0,16);
		break;
	case GET_IP:
		panel->writeText(node->getIp(),0,8,16);
		break;
	}
}
