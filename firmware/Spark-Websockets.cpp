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

/*
 * Base64 Ecoding Only Copyright (c) 1996-1999 by Internet Software Consortium.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND INTERNET SOFTWARE CONSORTIUM DISCLAIMS
 * ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL INTERNET SOFTWARE
 * CONSORTIUM BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 * PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS
 * ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 */

/*
 * Base64 Encoding Only - Portions Copyright (c) 1995 by International Business Machines, Inc.
 *
 * International Business Machines, Inc. (hereinafter called IBM) grants
 * permission under its copyrights to use, copy, modify, and distribute this
 * Software with or without fee, provided that the above copyright notice and
 * all paragraphs of this notice appear in all copies, and that the name of IBM
 * not be used in connection with the marketing of any product incorporating
 * the Software or modifications thereof, without specific, written prior
 * permission.
 *
 * To the extent it has a right to do so, IBM grants an immunity from suit
 * under its patents, if any, for the use, sale or manufacture of products to
 * the extent that such products are used for performing Domain Name System
 * dynamic updates in TCP/IP networks by means of the Software.  No immunity is
 * granted for any product per se or for any other function of any product.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", AND IBM DISCLAIMS ALL WARRANTIES,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE.  IN NO EVENT SHALL IBM BE LIABLE FOR ANY SPECIAL,
 * DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER ARISING
 * OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE, EVEN
 * IF IBM IS APPRISED OF THE POSSIBILITY OF SUCH DAMAGES.
 */

#define HANDSHAKE // uncomment to print out the sent and received handshake messages
// #define TRACE // uncomment to support TRACE level debugging of wire protocol
#define DEBUGGING // turn on debugging

#include "Spark-Websockets.h"
// #include <stdlib.h>
// #include <stdarg.h>
// #include <stdlib.h>
// #include <string.h>

//char *stringVar = "{0}";

void WebSocketClient::connect(const char hostname[], int port, const char protocol[], const char path[]) {
  _hostname = hostname;
  _port = port;
  _protocol = protocol;
  _path = path;
  _retryTimeout = millis();
  _canConnect = true;
}


void WebSocketClient::reconnect() {
  bool result = false;
  bool isconnected = false;
	#ifdef DEBUGGING
	Serial.println("Connecting...");
	#endif
	//byte server[] = { 192, 168, 1, 100 };
	int i, count;
	for (i=0, count=0; _hostname[i]; i++)
	  count += (_hostname[i] == '.');
	if (count == 3)
	{
		byte ip[4];
		sscanf(_hostname, "%hu.%hu.%hu.%hu", &ip[0], &ip[1], &ip[2], &ip[3]);
		isconnected = _client.connect(ip, _port);
	}
	else
	{
		isconnected = _client.connect(_hostname, _port);
	}
	if(isconnected)
	{
		#ifdef DEBUGGING
		Serial.println("Connected, sending handshake.");
		#endif
		sendHandshake(_hostname, _path, _protocol);
		result = readHandshake();
	}
  if(!result) {
    #ifdef DEBUGGING
    Serial.println("Connection Failed!");
    #endif
    if(_onError != NULL) {
      _onError(*this, "Connection Failed!");
    }
    _client.stop();
  } else {
      if(_onOpen != NULL) {
          _onOpen(*this);
      }
  }
}

bool WebSocketClient::connected() {
  return _client.connected();
}

void WebSocketClient::disconnect() {
  _client.stop();
}

byte WebSocketClient::nextByte() {
  while(_client.available() == 0);
  byte b = _client.read();

  #ifdef DEBUGGING
  if(b < 0) {
    Serial.println("Internal Error in Ethernet Client Library (-1 returned where >= 0 expected)");
  }
  #endif

  return b;
}

void WebSocketClient::monitor () {

  if(!_canConnect) {
    #ifdef DEBUGGING
    Serial.println("@@@ cannot connect");
    #endif
    return;
  }

  if(_reconnecting) {
    #ifdef DEBUGGING
    Serial.println("@@@ reconnecting");
    #endif
    return;
  }

  if(!connected() && millis() > _retryTimeout) {
    _retryTimeout = millis() + RETRY_TIMEOUT;
    _reconnecting = true;
    #ifdef DEBUGGING
    Serial.println("@@@ websocket: reconnecting...");
    #endif
    reconnect();
    _reconnecting = false;
    return;
  }

  // Serial.println(_client.available());

	if (_client.available() > 2) {
    #ifdef DEBUGGING
    // Serial.print("+");   // we should get a regular ping from the server
    #endif

    byte hdr = nextByte();
    bool fin = hdr & 0x80;

    #ifdef TRACE
    Serial.print("fin = ");
    Serial.println(fin);
    #endif

    int opCode = hdr & 0x0F;

    #ifdef TRACE
    Serial.print("op = ");
    Serial.println(opCode);
    #endif

    hdr = nextByte();
    bool mask = hdr & 0x80;
    int len = hdr & 0x7F;
    if(len == 126) {
      len = nextByte();
      len <<= 8;
      len += nextByte();
    } else if (len == 127) {
      len = nextByte();
      for(int i = 0; i < 7; i++) { // NOTE: This may not be correct.  RFC 6455 defines network byte order(??). (section 5.2)
        len <<= 8;
        len += nextByte();
      }
    }

    #ifdef TRACE
    Serial.print("len = ");
    Serial.println(len);
    #endif

    if(mask) { // skipping 4 bytes for now.
      for(int i = 0; i < 4; i++) {
        nextByte();
      }
    }

    if(mask) {

    #ifdef DEBUGGING
    Serial.println("Masking not yet supported (RFC 6455 section 5.3)");
    #endif

      if(_onError != NULL) {
        _onError(*this, "Masking not supported");
      }
      free(_packet);
      return;
    }

    if(!fin) {
      if(_packet == NULL) {
        _packet = (char*) malloc(len);

        uint8_t temppack[len];
	      _client.read(temppack, len);
        for(int i = 0; i < len; i++) {
          _packet[i] = (char)temppack[i];
        }
        // for(int i = 0; i < len; i++) {
        //   _packet[i] = nextByte();
        // }

        _packetLength = len;
        _opCode = opCode;
      } else {
        int copyLen = _packetLength;
        _packetLength += len;
        char *temp = _packet;
        _packet = (char*)malloc(_packetLength);
        for(int i = 0; i < copyLen; i++) {
          if(i < copyLen) {
            _packet[i] = temp[i];
          } else {
            _packet[i] = nextByte();
          }
        }
        free(temp);
      }
      return;
    }

    if(_packet == NULL) {
      _packet = (char*) malloc(len + 1);
      uint8_t temppack[len];
      _client.read(temppack, len);
      for(int i = 0; i < len; i++) {
        _packet[i] = (char)temppack[i];
      }
      // for(int i = 0; i < len; i++) {
      //   _packet[i] = nextByte();
      // }
      _packet[len] = 0x0;
    } else {
      int copyLen = _packetLength;
      _packetLength += len;
      char *temp = _packet;
      _packet = (char*) malloc(_packetLength + 1);
      for(int i = 0; i < _packetLength; i++) {
        if(i < copyLen) {
          _packet[i] = temp[i];
        } else {
          _packet[i] = nextByte();
        }
      }
      _packet[_packetLength] = 0x0;
      free(temp);
    }

    if(opCode == 0 && _opCode > 0) {
      opCode = _opCode;
      _opCode = 0;
    }

    switch(opCode) {
      case 0x00:
        #ifdef DEBUGGING
        Serial.println("Unexpected Continuation OpCode");
        #endif
        break;

      case 0x01:          // incoming message
        #ifdef DEBUGGING
      	// Serial.print("onMessage: data = ");
      	// Serial.println(_packet);
        #endif

        if (_onMessage != NULL) {
          _onMessage(*this, _packet);
        }
        break;

      case 0x02:
        #ifdef DEBUGGING
        Serial.println("Binary messages not yet supported (RFC 6455 section 5.6)");
        #endif

        if(_onError != NULL) {
          _onError(*this, "Binary Messages not supported");
        }
        break;

      case 0x09:          // ping
        #ifdef DEBUGGING
	      Serial.print(".");
        #endif

        _client.write(0x8A);  // reply with pong
	      // _client.write(0x8A);//_client.write(0x09);//0x0A; - pong
        _client.write(byte(0x00));
        break;

      case 0x0A:          // pong
        #ifdef DEBUGGING
      	Serial.print("onPong");
        #endif
        break;

      case 0x08:
        unsigned int code = ((byte)_packet[0] << 8) + (byte)_packet[1];

        #ifdef DEBUGGING
    		Serial.print("onClose code=");
        Serial.println(code);
        //Serial.print(" message = ");
        //Serial.print(message);
        #endif

        if(_onClose != NULL) {
          _onClose(*this, code, (_packet + 2));
        }
        _client.stop();
        break;
    }

    free(_packet);
    _packet = NULL;
  } else {
    Spark.process();
  }
}

void WebSocketClient::onMessage(OnMessage fn) {
  _onMessage = fn;
}

void WebSocketClient::onOpen(OnOpen fn) {
  _onOpen = fn;
}

void WebSocketClient::onClose(OnClose fn) {
  _onClose = fn;
}

void WebSocketClient::onError(OnError fn) {
  _onError = fn;
}


void WebSocketClient::sendHandshake(const char* hostname, const char* path, const char* protocol) {
  #ifdef DEBUGGING
  Serial.println("Sending handshake!");
  #endif

  WebSocketClientStringTable.replace("{0}", hostname);
  String strport = String(_port);
  WebSocketClientStringTable.replace("{1}", strport);

  _client.print(WebSocketClientStringTable);
  #ifdef HANDSHAKE
  Serial.println(WebSocketClientStringTable);
  Serial.println("Handshake sent");
  #endif
}

bool WebSocketClient::readHandshake() {
  #ifdef HANDSHAKE
	Serial.println("Reading handshake!");
  #endif
  bool result = true;
  char line[128];
  int maxAttempts = 300, attempts = 0;
  //char response;
  //response = reinterpret_cast<char>(WebSocketClientStringTable[9]);

  while(_client.available() == 0 && attempts < maxAttempts) {
    delay(50);
    attempts++;
  }

  while(true) {
    readLine(line);
    #ifdef HANDSHAKE
    Serial.print("handshake rcvd line: ");
    Serial.println(line);
    #endif

    if(strcmp(line, "") == 0) {
      break;
    }
    if(strncmp(line, "1VTFj/CydlBCZDucDqw8eA==", 12) == 0) {
      result = true;
    }
  }

  if(!result) {
    #ifdef DEBUGGING
    Serial.println("Handshake Failed! Terminating");
    #endif
    _client.stop();
  }
  else
	{
    #ifdef DEBUGGING
	  Serial.println("Handshake Ok!");
    #endif
	}
  return result;
}

void WebSocketClient::readLine(char* buffer) {
  char character;

  int i = 0;
  while(_client.available() > 0 && (character = _client.read()) != '\n') {
    if (character != '\r' && character != -1) {
		  //Serial.print(character);
      buffer[i++] = character;
    }
  }
  buffer[i] = 0x0;
}

bool WebSocketClient::send (char* message) {
  if(!_canConnect || _reconnecting) {
    return false;
  }
  int len = strlen(message);
  _client.write(0x81);
  if(len > 125) {
    _client.write(0xFE);
    _client.write(byte(len >> 8));
    _client.write(byte(len & 0xFF));
  } else {
    _client.write(0x80 | byte(len));
  }
  for(int i = 0; i < 4; i++) {
    _client.write((byte)0x00); // use 0x00 for mask bytes which is effectively a NOOP
  }
  _client.print(message);
  #ifdef TRACE
  Serial.print("message sent: ");
  Serial.print(_client.connected());  // STOPPED HERE, STILL TRUE (1) WHEN PING BREAKS!!!
  Serial.print(" ");
  Serial.println(message);
  #endif
  return true;
}


size_t WebSocketClient::base64Encode(byte* src, size_t srclength, char* target, size_t targsize) {

  size_t datalength = 0;
	char input[3];
	char output[4];
	size_t i;

	while (2 < srclength) {
		input[0] = *src++;
		input[1] = *src++;
		input[2] = *src++;
		srclength -= 3;

		output[0] = input[0] >> 2;
		output[1] = ((input[0] & 0x03) << 4) + (input[1] >> 4);
		output[2] = ((input[1] & 0x0f) << 2) + (input[2] >> 6);
		output[3] = input[2] & 0x3f;

		if (datalength + 4 > targsize) {
			return (-1);
    }

		target[datalength++] = b64Alphabet[output[0]];
		target[datalength++] = b64Alphabet[output[1]];
		target[datalength++] = b64Alphabet[output[2]];
		target[datalength++] = b64Alphabet[output[3]];
	}

  // Padding
	if (0 != srclength) {
		input[0] = input[1] = input[2] = '\0';
		for (i = 0; i < srclength; i++) {
			input[i] = *src++;
    }

		output[0] = input[0] >> 2;
		output[1] = ((input[0] & 0x03) << 4) + (input[1] >> 4);
		output[2] = ((input[1] & 0x0f) << 2) + (input[2] >> 6);

		if (datalength + 4 > targsize) {
			return (-1);
    }

		target[datalength++] = b64Alphabet[output[0]];
		target[datalength++] = b64Alphabet[output[1]];
		if (srclength == 1) {
			target[datalength++] = '=';
    } else {
			target[datalength++] = b64Alphabet[output[2]];
    }
		target[datalength++] = '=';
	}
	if (datalength >= targsize) {
		return (-1);
  }
	target[datalength] = '\0';
	return (datalength);
}

void WebSocketClient::generateHash(char buffer[], size_t bufferlen) {
  byte bytes[16];
  for(int i = 0; i < 16; i++) {
    bytes[i] = rand() % 255 + 1;
  }
  base64Encode(bytes, 16, buffer, bufferlen);
}
