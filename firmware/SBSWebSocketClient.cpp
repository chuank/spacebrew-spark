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
// #define TRACEOUT  // uncomment to print outgoing websocket messages (causes issues if you have analog output)
#define TRACEIN   // uncomment to print incoming websocket messages
#define DEBUG // turn on debugging

#include "SBSWebSocketClient.h"

void SBSWebSocketClient::connect(const char hostname[], int port, const char protocol[], const char path[]) {
  _hostname = hostname;
  _port = port;
  _protocol = protocol;
  _path = path;
  _retryTimeout = millis();
  _canConnect = true;
  _sendingConfig = false;
}

void SBSWebSocketClient::reconnect() {
  bool result = false;
  bool isconnected = false;
	#ifdef DEBUG
	Serial.print("[info] Connecting websocket at: ");
  Serial.print(_hostname);
	#endif

	int i, count;
	for (i=0, count=0; _hostname[i]; i++)
	  count += (_hostname[i] == '.');
	if (count == 3)
	{
    #ifdef DEBUG
    Serial.println(" (IP)");
    #endif
		byte ip[4];
		sscanf(_hostname, "%hu.%hu.%hu.%hu", &ip[0], &ip[1], &ip[2], &ip[3]);
		isconnected = _tcpclient.connect(ip, _port);
	} else {
    #ifdef DEBUG
    Serial.println(" (hostname)");
    #endif
		isconnected = _tcpclient.connect(_hostname, _port);
	}

  delay(100);               // delay allows slower servers (e.g. RPi) to respond

	if(isconnected)
	{
		#ifdef DEBUG
		Serial.println("[info] Connected, sending handshake...");
		#endif
    // _tcpclient.flush();
		sendHandshake(_hostname, _path, _protocol);
    // FIXME #5 will adding a delay work?
    delay(250);
		result = readHandshake();
	}

  if(!result) {
    // #ifdef DEBUG
    // Serial.println("[error] Connection Failed!");
    // #endif
    if(_onError != NULL) {
      _onError(*this, "[error] Connection Failed!");
    }
    _tcpclient.stop();
  } else {
      if(_onOpen != NULL) {
          _onOpen(*this);
      }
  }
}

bool SBSWebSocketClient::connected() {
  return _tcpclient.connected();
}

void SBSWebSocketClient::disconnect() {
  _tcpclient.stop();
}

byte SBSWebSocketClient::nextByte() {
  //while(_tcpclient.available() == 0);
  byte b;
  if(_tcpclient.available()>0) {
    b = _tcpclient.read();
  } else {
    b = -1;
  }

  #ifdef DEBUG
  if(b < 0) {
    Serial.println("[error] TCPClient Internal Error (-1 returned where >= 0 expected)");
  }
  #endif

  return b;
}

void SBSWebSocketClient::monitor () {

  if(!_canConnect) {
    #ifdef DEBUG
    Serial.println("[error] cannot connect");
    #endif
    return;
  }

  if(!connected() && millis() > _retryTimeout) {
    _retryTimeout = millis() + RETRY_TIMEOUT;
    _reconnecting = true;
    #ifdef DEBUG
    Serial.println("[info] websocket: connecting...");
    #endif
    reconnect();
    _reconnecting = false;
    return;
  }

	if (_tcpclient.available() > 2) {
    byte hdr = nextByte();
    bool fin = hdr & 0x80;  // are we looking at the concluding data frame?

    int opCode = hdr & 0x0F;

    hdr = nextByte();
    bool mask = hdr & 0x80;
    int len = hdr & 0x7F;
    if(len == 126) {
      #ifdef DEBUG
      Serial.println("[info] len header: 16-bit");
      #endif
      len = nextByte();
      len <<= 8;
      len += nextByte();
    } else if (len == 127) {
      #ifdef DEBUG
      Serial.println("[info] len header: 64-bit");
      #endif
      len = nextByte();
      for(int i = 0; i < 7; i++) { // NOTE: This may not be correct.  RFC 6455 defines network byte order(??). (section 5.2)
        len <<= 8;
        len += nextByte();
      }
    }

    #ifdef TRACE
    Serial.print("[trace] fin = ");
    Serial.print(fin);
    Serial.print(", op = ");
    Serial.print(opCode);
    Serial.print(", len = ");
    Serial.println(len);
    #endif

    if(mask) { // skipping 4 bytes for now.
      for(int i = 0; i < 4; i++) {
        nextByte();
      }
    }

    if(mask) {

    #ifdef DEBUG
    Serial.println("[error] Masking not yet supported (RFC 6455 section 5.3)");
    #endif

      if(_onError != NULL) {
        _onError(*this, "[error] Masking not supported");
      }
      free(_packet);
      return;
    }

    if(!fin) {                  // absence of fin bit in data frame message has subsequent data frame(s) coming!
      if(_packet == NULL) {                   // brand new data frame
        // FIXME #4 process message data here, but issues w/memory leak due to insufficient buffer

        #ifdef DEBUG
        Serial.println("[info] single frame + !FIN");
        #endif

        dataType = SB_START;
        _packet = (char*) malloc(len);

        uint8_t temppack[len];
        _tcpclient.read(temppack, len);
        for(int i = 0; i < len; i++) {
          _packet[i] = (char)temppack[i];
        }
        _packetLength = len;
        _opCode = opCode;
      } else {                                // data frame continues from previous packet (_packet != NULL)
        #ifdef DEBUG
        Serial.println("[info] continuation frame + !FIN");
        #endif

        dataType = SB_MID;

        int copyLen = _packetLength;
        _packetLength += len;
        char *temp = _packet;
        _packet = (char*)malloc(_packetLength);

        for(int i = 0; i < copyLen; i++) {
          _packet[i] = temp[i];               // preserve previous _packet data
        }
        uint8_t temppack[len];
        _tcpclient.read(temppack, len);
        for(int i = 0; i < len; i++) {        // add to existing _packet data
          _packet[i+copyLen] = (char)temppack[i];
        }

        free(temp);
      }
      return;     // bail out early, as there's more data frame(s) to process
    }

    // from here on, FIN bit is present, therefore this is the concluding data frame
    if(_packet == NULL) {             // brand new data frame
      #ifdef DEBUG
      if(len!=4) {      // 4 == ping (0D 0A 0D 0A)
        Serial.println("\r\n[info] single frame + FIN");
      }
      #endif

      dataType = SB_END;

      _packet = (char*) malloc(len + 1);
      uint8_t temppack[len];
      _tcpclient.read(temppack, len);
      for(int i = 0; i < len; i++) {
        _packet[i] = (char)temppack[i];
      }
      _packet[len] = 0x0;             // FIN, so put in the terminating char

    } else {                          // data frame continues from previous packet (_packet != NULL)
      #ifdef DEBUG
      Serial.println("\r\n[info] continuation frame + FIN");
      #endif

      dataType = SB_END;

      int copyLen = _packetLength;
      _packetLength += len;
      char *temp = _packet;
      _packet = (char*) malloc(_packetLength + 1);

      for(int i = 0; i < copyLen; i++) {
        _packet[i] = temp[i];                 // preserve previous _packet data
      }
      uint8_t temppack[len];
      _tcpclient.read(temppack, len);
      for(int i = 0; i < len; i++) {          // add to existing _packet data
        _packet[i+copyLen] = (char)temppack[i];
      }

      _packet[_packetLength] = 0x0;   // FIN, so put in the terminating char
      free(temp);
    }

    if(opCode == 0 && _opCode > 0) {
      opCode = _opCode;
      _opCode = 0;
    }

    #ifdef DEBUG
    if(opCode!=9) {
      Serial.print("[info] opCode: ");
      Serial.println(opCode,HEX);
    }
    #endif

    switch(opCode) {
      case 0x00:
        #ifdef DEBUG
        Serial.println("[error] Unexpected Continuation OpCode");
        #endif
        break;

      case 0x01:          // incoming message
        if (_onMessage != NULL) {
          Serial.println("[[incoming data]]");

          // TODO #4 do not verify if we're still awaiting response from sendconfig!!!

          if(verifyData(dataType,_packet)) {     // check for malformed data
            // data is as expected
            #ifdef DEBUG
            Serial.print("[info] incoming (");
            Serial.print(len);
            Serial.print(") : ");
            Serial.println(_packet);
            #endif

            _onMessage(*this, _packet);

          } else {
            #ifdef DEBUG
            Serial.print("[error] malformed data: ");
            Serial.println(_packet);
            Serial.println("[### CRITICAL ###] POSSIBLE BUFFER OVERRUN; attempting reconnect");
            #endif

            // FIXME #4 double-check bail out and reconnect sequence
            _tcpclient.flush();
            free(_packet);
            _packet = NULL;

            unsigned int code = ((byte)_packet[0] << 8) + (byte)_packet[1];
            if(_onClose != NULL) {
              _onClose(*this, code, (_packet + 2));
            }
            _tcpclient.stop();           // monitor() method takes care of reconnection!
            delay(RECONNECT_TIMEOUT);
          }
        }
        break;

      case 0x02:        // binary message
        #ifdef DEBUG
        Serial.println("[error] Binary messages not yet supported (RFC 6455 section 5.6)");
        #endif

        if(_onError != NULL) {
          _onError(*this, "[error] Binary Messages not supported");
        }
        break;

      case 0x09:          // ping
        #ifdef DEBUG
	      Serial.print(".");
        #endif

        //TODO OPTIMIZE/VERIFY: send as 2 bytes using client.write(buf,len)
        _tcpclient.write(0x8A);  // reply with pong
        _tcpclient.write(byte(0x00));
        break;

      case 0x0A:          // pong
        #ifdef DEBUG
      	Serial.print("[info] someone pong'ed me");
        #endif
        break;

      case 0x08:          // close connection
        unsigned int code = ((byte)_packet[0] << 8) + (byte)_packet[1];

        #ifdef DEBUG
    		Serial.print("[info] onClose code = ");
        Serial.println(code);
        Serial.print(", message = ");
        Serial.print(_packet);
        #endif

        if(_onClose != NULL) {
          _onClose(*this, code, (_packet + 2));
        }
        _tcpclient.stop();
        break;
    }

    free(_packet);
    _packet = NULL;
  }
}

bool SBSWebSocketClient::verifyData(SBHead type, char* p) {
  // verify integrity of Spacebrew JSON message depending on the type of frame encountered
  switch(type) {
    case SB_START:
      return (strstr(p, "{\"message\":{") == NULL) ? false : true;
    case SB_END:
      return (strstr(p, "}}") == NULL) ? false : true;
    case SB_MID:
      // no way to check if it's midpoint data!
      break;
  }
  return false;
}

void SBSWebSocketClient::onMessage(OnMessage fn) {
  _onMessage = fn;
}

void SBSWebSocketClient::onOpen(OnOpen fn) {
  _onOpen = fn;
}

void SBSWebSocketClient::onClose(OnClose fn) {
  _onClose = fn;
}

void SBSWebSocketClient::onError(OnError fn) {
  _onError = fn;
}

void SBSWebSocketClient::sendHandshake(const char* hostname, const char* path, const char* protocol) {
  #ifdef DEBUG
  Serial.println("[info] Sending handshake!");
  #endif

  SBSWebSocketClientStringTable.replace("{0}", hostname);
  String strport = String(_port);
  SBSWebSocketClientStringTable.replace("{1}", strport);

  int blen = SBSWebSocketClientStringTable.length();
  char buf[blen+1];
  SBSWebSocketClientStringTable.toCharArray(buf,blen);

  // TODO #5
  _tcpclient.flush();    // flush before sending handshake
  _tcpclient.write((uint8_t *)buf,blen);

  #ifdef HANDSHAKE
  Serial.println("[info] Handshake sent: ");
  Serial.println(SBSWebSocketClientStringTable);
  #endif
}

bool SBSWebSocketClient::readHandshake() {
  #ifdef HANDSHAKE
	Serial.println("[info] Reading handshake!");
  #endif
  bool result = true;
  char line[128];
  int maxAttempts = 300, attempts = 0;
  //char response;
  //response = reinterpret_cast<char>(SBSWebSocketClientStringTable[9]);

  // blocking; wait maxAttempts * 50ms for a response from server
  while(_tcpclient.available() == 0 && attempts < maxAttempts) {
    delay(50);
    attempts++;
  }

  while(true) {
    readLine(line);
    #ifdef HANDSHAKE
    Serial.print("[info] handshake rcvd line: ");
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
    #ifdef DEBUG
    Serial.println("[error] Handshake Failed! Terminating");
    #endif
    _tcpclient.stop();
  }
  else
	{
    #ifdef DEBUG
	  Serial.println("[info] Handshake Ok!");
    #endif
	}
  return result;
}

void SBSWebSocketClient::readLine(char* buffer) {
  char character;

  int i = 0;
  while(_tcpclient.available() > 0 && (character = _tcpclient.read()) != '\n') {
    if (character != '\r' && character != -1) {
      buffer[i++] = character;
    }
  }
  buffer[i] = 0x0;
}

bool SBSWebSocketClient::send (char* message) {
  if(!_canConnect || _reconnecting) {
    return false;
  }
  int len = strlen(message);
  _tcpclient.write(0x81);
  if(len > 125) {
    _tcpclient.write(0xFE);
    _tcpclient.write(byte(len >> 8));
    _tcpclient.write(byte(len & 0xFF));
  } else {
    _tcpclient.write(0x80 | byte(len));
  }
  for(int i = 0; i < 4; i++) {
    _tcpclient.write((byte)0x00); // use 0x00 for mask bytes which is effectively a NOOP
  }

  #ifdef TRACEOUT
  Serial.print("[trace] outgoing (length:");
  Serial.print(strlen(message));
  Serial.print(") : ");
  Serial.println(message);
  #endif

  _tcpclient.write((uint8_t *)message, len);
  return true;
}

void SBSWebSocketClient::sendConfig (char* message) {
  _sendingConfig = true;
  send(message);
}


size_t SBSWebSocketClient::base64Encode(byte* src, size_t srclength, char* target, size_t targsize) {

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

void SBSWebSocketClient::generateHash(char buffer[], size_t bufferlen) {
  //TODO #2

  byte bytes[16];
  for(int i = 0; i < 16; i++) {
    bytes[i] = rand() % 255 + 1;
  }
  base64Encode(bytes, 16, buffer, bufferlen);
}
