/*
 * NodeUpload.h
 *
 *  Created on: 30.08.2015
 *      Author: md
 */

#ifndef NODEUPLOAD_H_
#define NODEUPLOAD_H_

#include <stdint.h>

class NodeUpload;

class NodeUpload {
public:

	NodeUpload();
	virtual ~NodeUpload();
	void main();


private:
    boolean read_timed_out;
	boolean user_abort;

    void setspeed();
    void resetspeed();
    void ServerStartup();
    void ServerShutdown();
    void clearESP8266SerialBuffer();
    void sendESP8266Cmdln(const char* cmd, uint16_t waitTime);
    char getCharESPBlocking(uint32_t timeout);
    char getPreamble(char filename[], uint16_t * blocksize);
    uint16_t getDataLine(uint16_t blocksize, char payload[], uint32_t * blockno);

};
//#define ESP_SPEED 9600lu
//#define ESP_SPEED 74880lu
//#define ESP_SPEED 115200lu
//#define ESP_SPEED 230400lu
//#define ESP_SPEED 460800lu
#define ESP_SPEED 921600lu
//#define ESP_SPEED 1843200lu

#define GETCHAR_TIMEOUT 1000
#define CMDTIMEOUT 500
#define MAXLEN 20



#endif /* NODEUPLOAD_H_ */
