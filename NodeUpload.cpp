/*
 * NodeUpload.cpp
 *
 *  Created on: 30.08.2015
 *      Author: md
 */


#include <stddef.h>
#include <string.h>

#include <SD.h>
#include "WProgram.h"
#include "debug.h"
#include <Stream.h>

#include "NodeUpload.h"

#define ESP8266 Serial1

NodeUpload::NodeUpload() {
	read_timed_out = false;
}

NodeUpload::~NodeUpload() {
}

void NodeUpload::setspeed() {
//	DPRINTF2("resetting ESP8266");
//	sendESP8266Cmdln ("node.restart()",500);
//	delay (5000);


	DPRINTF(" setting speed ESP8266 to %lu\n",ESP_SPEED);
//	Serial.write("__PIC32_pbClk: ");
//	Serial.print(__PIC32_pbClk);
//	Serial.write("\r\n");
//	Serial.write("Baud Rate Reg Value: ");
//	unsigned long tempLong = ((__PIC32_pbClk / 4 / ESP_SPEED) - 1);
//	Serial.print(tempLong,16);
//	Serial.write("\r\n");
	DPRINTF(" setting speed ESP8266 to %lu\n",ESP_SPEED);

	char cmd[32];
	sprintf(cmd,"uart.setup( 0,%lu,8,0,1,0 )", ESP_SPEED);
	ESP8266.println (cmd);
	delay (500);
	ESP8266.flush();
	ESP8266.purge();
	ESP8266.end();
	delay (500);


	ESP8266.begin(ESP_SPEED);
	ESP8266.flush();
	ESP8266.purge();
	sendESP8266Cmdln ("",500);
	sendESP8266Cmdln ("wifi.sta.connect()",500);
	sendESP8266Cmdln ("=node.heap()",500);
	sendESP8266Cmdln ("=wifi.sta.getip()",500);
}

void NodeUpload::resetspeed() {
	DPRINTF2(" resetting speed ESP8266\n");

	ESP8266.flush();
	ESP8266.purge();
	ESP8266.println ("uart.setup( 0,9600,8,0,1,0 )");
	delay (500);
	ESP8266.purge();
	ESP8266.flush();
	ESP8266.begin(9600);
	ESP8266.write('\n');

	sendESP8266Cmdln ("",500);
	sendESP8266Cmdln ("=node.heap()",500);
}


const char* tftpd =
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

void NodeUpload::main() {

	ServerStartup();

	// panel notify user to start upload

	user_abort = false;
	read_timed_out = false;

	SdFile f;
	char OPcode;
	char filename[MAXLEN];
	uint16_t blocksize;
	char * payload = NULL;

	// get line terminated by CRNL. This could be a "B"egin line indicating a transfer to SD
	// It could also be an "E"nd line indicating that a transfer was done to local ESPs file system

	OPcode = getPreamble(filename, &blocksize);
	DPRINTF("blocksize %u",blocksize);

	if (user_abort || read_timed_out ) {
		//timeout waiting for transfer start or user abort
		// print error & clear buffers
		DPRINTF2("Transfer aborted !\n");
		ServerShutdown();
		return;
	}

	if (OPcode == 'B') {
		// get filename and open file
		DPRINTF("SD card filename %s\n",filename);
		if (! f.open(SD.root, filename, O_CREAT|O_WRITE)) {
			DPRINTF2("fopen error!");
			ServerShutdown();
			return;
		}
		payload = (char*) malloc (blocksize);
		if (payload == NULL) {
			//malloc error
			DPRINTF2("malloc error !");
			ServerShutdown();
			return;
		}
	}
	else if (OPcode == 'F') {
		// get ESP filename and print on panel
		DPRINTF("ESP filename %s",filename);
	}
	else {
		// protocol error
		DPRINTF2("Wrong start marker received");
		ServerShutdown();
		return;
	}

	uint32_t rblockno = 1;
	uint32_t lblockno = 1;
	uint16_t numbytes = blocksize;
	int retries = 0;
	while (numbytes == blocksize && retries < 20) {
		numbytes = getDataLine(blocksize, payload, &rblockno);

		if (read_timed_out) {
			retries++;
//			read_timed_out = false;
			numbytes = blocksize;
			DPRINTF("retry %i\n", retries);
		}
		else {
			if (rblockno == lblockno) {
				if (! f.write(payload,numbytes)) {
					DPRINTF2("file write error!\n");
					read_timed_out = true;
					break;
				}
				// print blockno progress to panel ?
			}

			if (numbytes != blocksize ) { // last block received successfully
				break;
			}

			lblockno++;
			retries = 0;
		}
	}

	f.close();
	if (payload != NULL) {
		free(payload);
		payload = NULL;
	}

	if (read_timed_out) {
		// panel error write
		DPRINTF2 ("error in transfer \n");
	}
	else {
		// success
		DPRINTF2 ("Done successfully !\n");
	}

	ServerShutdown();
}

void NodeUpload::ServerStartup()
{
	NodeUpload::setspeed();

	// get tftpd-lua code to ESP
//	NodeUpload::prepareServer();
	sendESP8266Cmdln ("=node.heap()",CMDTIMEOUT);

	DPRINTF2(" starting tftp server\n");
	sendESP8266Cmdln ("if tftpd then tftpd:stop() tftpd=nil end",CMDTIMEOUT);

	sendESP8266Cmdln ("dofile(\"tftpd.lc\")",CMDTIMEOUT);
	sendESP8266Cmdln ("tftpd:start()",CMDTIMEOUT);

}

void NodeUpload::ServerShutdown()
{
	//clearESP8266SerialBuffer();

	DPRINTF2(" stopping tftp server");
	ESP8266.write("quit\n"); // tell ack-handler to shutdown
	sendESP8266Cmdln ("=node.heap()",CMDTIMEOUT);

	sendESP8266Cmdln ("if tftpd then tftpd:stop() tftpd=nil end",CMDTIMEOUT);

	NodeUpload::resetspeed();
}

//Send command to ESP8266, assume OK, no error check
//wait some time and print debug response
void NodeUpload::sendESP8266Cmdln(const char*  cmd, uint16_t waitTime)
{
  ESP8266.println(cmd);
  long _startMillis = millis();
   do {
   } while(millis() - _startMillis < waitTime);
  clearESP8266SerialBuffer();
}

//Clear and display Serial Buffer for ESP8266
void NodeUpload::clearESP8266SerialBuffer()
{
  while (ESP8266.available() > 0) {
    char a = ESP8266.read();
    printf("'%c'",a);
  }
}

//Get a char from ESP with timeout (in milliseconds)
// return char if within timeout
// if not, just return 00x and set global timeout var to true
char NodeUpload::getCharESPBlocking(uint32_t timeout)
{
	char ch;
	uint32_t start = millis();
	while (true) {
		if (ESP8266.available()) {
			ch = ESP8266.read();
			return ch;
		}
		if ((start + timeout) < millis()) {
			read_timed_out = true;
			break;
		}
	}
	return 0;

}

// read preamble line
// B<filename>|<blksize>
// F<filename>|<blksize>  // indicate transfer to ESPs flash
// notify user file waiting
// read IR to abort
char NodeUpload::getPreamble(char filename[], uint16_t * blocksize) {

	read_timed_out = false;
	char opCode = ' ';
	int secs = 0;
	while ( secs < 300 ) {
		secs++;

		if ((opCode = getCharESPBlocking(GETCHAR_TIMEOUT))) {
			DPRINTF("->%c\n",opCode);
			if (opCode == 'B' || opCode == 'F') {
				break;
			}
		}
	}
	if (opCode == 'B') {
		read_timed_out=false;
		int i = ESP8266.readBytesUntil('|',filename,MAXLEN-1);
		filename[i] = '\0';
		uint16_t block = ESP8266.parseInt();
		printf ("Block %u ",block);
		*blocksize = block;
		if (i==0 || block ==0){
			//something went wrong so set error
			read_timed_out = true;
		}

		int ch = getCharESPBlocking(GETCHAR_TIMEOUT); //consume newline
	}
	// just fall thru even in error
	return opCode;
}

// read data line
// D<5digit blockno>|<4digit size>|<blocksize bytes payload>
// sets global read_timed_out if invalid block read
//
uint16_t NodeUpload::getDataLine(uint16_t blocksize, char payload[], uint32_t * blockno) {

	read_timed_out = false;
	uint32_t rblock = 0;
	uint16_t numbytes = 0;
	char opCode = getCharESPBlocking( 1000L * 2); // wait max 2 secs for the first data byte to arrive
	printf("\nopcode=%c:%02x",opCode,opCode);
	if (!read_timed_out && opCode == 'D') {

		rblock = ESP8266.parseInt();
		numbytes = ESP8266.parseInt();

		int ch = getCharESPBlocking(GETCHAR_TIMEOUT); // consume delimiter

		printf (" Block:%5d",rblock);
		printf (" Expect:%4d",numbytes);
		printf (" blksize:%4d",blocksize);
		//only read numbytes if no timeout
		if (!read_timed_out && rblock && numbytes) {

			uint16_t readbytes = ESP8266.readBytes(payload,blocksize);
			printf (" Read:%4d",readbytes);

#ifdef DUMPPAYLOAD
			printf ("payload:");
			for(uint16_t i = 0; i < readbytes; i++) {
				printf ("%02x", payload[i]);
			}
			printf("\n");
#endif
			// if we read a complete blocksize, then all good
			// last block is padded from numbytes to blocksize
			// send ack to tftpd so it can reply to sender
			if (readbytes == blocksize) {
				char ackbuf[5];
				const char ackbuf_format[] = "%5d";
				printf (" Sending ack %5d",rblock);
				sprintf (ackbuf, ackbuf_format, rblock);
				ESP8266.write(ackbuf);

				*blockno = rblock;
				return numbytes;

			}

			// we read either nothing at all or an incomplete block
			read_timed_out = true;
			printf ("\nTIMEOUT while reading data-line. Only got %d", readbytes);

			// not clear what to do. Sender will re-send but will we be able to get to the next block
			// start ?
			ESP8266.purge();
			//clearESP8266SerialBuffer();
		}
	}
	else {
		// read garbage or nonsense ...
		printf("\n read nonsense for opcode");
		ESP8266.purge();
//		clearESP8266SerialBuffer();
		read_timed_out = true;
		numbytes = 0;
		rblock = 0;
	}

	*blockno = rblock;
	return numbytes;

}

