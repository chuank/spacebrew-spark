/*
SBSWebSocketClient, a websocket client tuned specifically for Spacebrew and Spark devices

The MIT License (MIT)

Copyright (c) 2014 Chuan Khoo
http://www.chuank.com

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

/*
 WebsocketClient, a websocket client for Arduino
 Copyright 2011 Kevin Rohling
 Copyright 2012 Ian Moore
 http://kevinrohling.com
 http://www.incamoon.com

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in
 all copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 THE SOFTWARE.
*/

#ifndef WEBSOCKETCLIENT_H
#define WEBSOCKETCLIENT_H

#define HANDSHAKE // uncomment to print out the sent and received handshake messages
// #define TRACE // uncomment to support TRACE level debugging of wire protocol
#define DEBUG // turn on debugging

#define RETRY_TIMEOUT 1000
#define RECONNECT_TIMEOUT 1000    // larger delays = better chance of recovery

#include "spark_wiring_usbserial.h"
#include "spark_wiring_tcpclient.h"
#include "spark_wiring_string.h"

enum SBHead { SB_START, SB_MID, SB_END };

class SBSWebSocketClient {
public:
  typedef void (*OnMessage)(SBSWebSocketClient client, char* message);
  typedef void (*OnOpen)(SBSWebSocketClient client);
  typedef void (*OnClose)(SBSWebSocketClient client, int code, char* message);
  typedef void (*OnError)(SBSWebSocketClient client, char* message);
  void connect(const char hostname[], int port = 80, const char protocol[] = NULL, const char path[] = "/");
  bool connected();
  void disconnect();
  void monitor();
  void onOpen(OnOpen function);
  void onClose(OnClose function);
  void onMessage(OnMessage function);
  void onError(OnError function);
  bool send(char* message);
  void sendConfig (char* message);
private:

String SBSWebSocketClientStringTable = {
      // "GET / HTTP/1.1\x0d\x0a"//, //"GET {0} HTTP/1.1",
			// "Upgrade: websocket\x0d\x0a"//,
			// "Connection: Upgrade\x0d\x0a"//,
			// "Host: {0}:{1}\x0d\x0a"//,//"Host: {0}",
      // "Pragma: no-cache\x0d\x0a"
      // "Cache-Control: no-cache\x0d\x0a"
			// "Origin: SparkWebSocketClient\x0d\x0a"//,
			// "Sec-WebSocket-Key:  1VTFj/CydlBCZDucDqw8eA==\x0d\x0a"//,
			// "Sec-WebSocket-Version: 13\x0d\x0a"//,
			// "\x0d\x0a"};

			"GET / HTTP/1.1\x0d\x0a"//,                                  // Websocket requires HTTP 1.1
      "Connection: Upgrade\x0d\x0a"//,                             // upgrade connection
			"Upgrade: websocket\x0d\x0a"//,                              // to websocket
			"Host: {0}:{1}\x0d\x0a"//,                                   // host:port information
			"Origin: SparkWebSocketClient\x0d\x0a"//,                    // TODO #7 ID origin
      "Sec-WebSocket-Version: 13\x0d\x0a"//,                       // websocket version
			"Sec-WebSocket-Key:  1VTFj/CydlBCZDucDqw8eA==\x0d\x0a"//,    // TODO #2 base64 hash
			"\x0d\x0a"};                                                 // requires a terminating CRLF

  const char* _hostname;
  const byte* _host;
  int _port;
  const char* _path;
  const char* _protocol;
  bool _canConnect;
  bool _reconnecting;
  bool _sendingConfig;
  unsigned long _retryTimeout;
  void reconnect();
  void sendHandshake(const char* hostname, const char* path, const char* protocol);
  TCPClient _tcpclient;
  OnOpen _onOpen;
  OnClose _onClose;
  OnMessage _onMessage;
  OnError _onError;
  char* _packet;
  unsigned int _packetLength;
  byte _opCode;
  SBHead dataType;
  bool readHandshake();
  void readLine(char* buffer);
  bool verifyData(SBHead type, char* p);
  void generateHash(char* buffer, size_t bufferlen);
  size_t base64Encode(byte* src, size_t srclength, char* target, size_t targetsize);
  byte nextByte();


};

const char b64Alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

#endif
