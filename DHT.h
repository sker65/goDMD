/*
 * DHT.h
 *
 *  Created on: 07.05.2016
 *      Author: sr
 */

#ifndef DHT_H_
#define DHT_H_

class DHT {
public:
	DHT(int pin);
	virtual ~DHT();

	enum ReturnCode { DHTLIB_ERROR_TIMEOUT, DHT_ERROR_CHECKSUM, DHT_OK };

	int pin;
	ReturnCode readDHT();
	int humidity;
	int temperature;
	bool isConnected;

};

#endif /* DHT_H_ */
