/*
 * NodeCallbackHandler.h
 *
 *  Created on: 25.08.2015
 *      Author: sr
 */

#ifndef NODECALLBACKHANDLER_H_
#define NODECALLBACKHANDLER_H_

#include "NodeMcu.h"

class LEDMatrixPanel;

class NodeCallbackHandler : public NodeNotifyCallback {
public:
	NodeCallbackHandler(LEDMatrixPanel* panel);
	virtual ~NodeCallbackHandler() {};
	virtual void notify(NodeMcu* node, int type);

private:
	LEDMatrixPanel* panel;

};

#endif /* NODECALLBACKHANDLER_H_ */
