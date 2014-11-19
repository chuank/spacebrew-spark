#spacebrew-sparkcore

**[SpaceBrew](docs.spacebrew.cc) + [Spark Core](spark.io) integration**

An attempt in getting SpaceBrew and the Spark Core to talk nice to one another.

Due to the TCP buffer size of 128 bytes set in the build IDE of the [Spark Core](spark.io), buffer overruns are very likely especially if you have low-latency, short-interval messages destined for the Spark Core. When a buffer overrun happens on the Spark Core, a hard fault SOS (colour code: red SOS + 1 blink) occurs, causing the Spark Core to reboot, which, depending on your Wifi connection quality, takes a while to reestablish itself.

This implementation works around this issue by dropping the websocket connection right before the buffer overrun hits, flushing the buffer and automatically reconnecting. The down time between the Spark Core and the Spacebrew server is reduced to about 1 second (local network Spacebrew server), and re-establishes itself to the Spacebrew server on its own. Essentially your Spark Core continues running (without needing to reboot), and Spacebrew continues working (although it will report a disconnect+connect).


##Limitations
The implementations here focus on exposing typical microcontroller-centric tasks to the Spacebrew server. Therefore, the features are kept lightweight and are intended to support the basic Spacebrew datatypes: `Boolean`, `String` and `Range`.

In addition, due to the limitations of the Spark's `TCPClient` and the original Arduino Websocket implementation, be mindful of the following limitations:

* The websocket implementation requires full attention of the TCPClient. In other words, you will not be able to connect to the Spark Cloud when using Spacebrew. The code sets the Spark Core to `MANUAL MODE` on startup. **If you remain in `AUTOMATIC` mode you will encounter SOS errors as the TCPClient buffer will overflow.**
* Due to the need for `MANUAL` mode, those of you compiling via the Web IDE will have to either download the compiled .bin and upload it locally, or, better yet, change to [Spark Dev](https://github.com/spark/spark-dev), and [upload your firmware via USB and DFU](https://community.spark.io/t/support-for-flashing-spark-core-using-dfu-util-in-atom/5430).
* All incoming messages are limited to **125 bytes maximum** at the moment. This includes the JSON data structure, so you effectively have around *45 bytes* to work with. This affects users who intend to send long strings via Spacebrew to the Spark Core.
* Binary messages are not supported (not usually used for a microcontroller-based Spacebrew publisher/subscriber anyway) for the same reasons as message size limitations.
* Code is still a work-in-progress, so proceed at your own risk.

##Issues
* Successive messages sent rapidly to the Spark Core are **guaranteed** to trigger the websocket disconnect-connect sequence.
* Monitor the serial port to understand what's happening to your code, and find out when your Spark Core disconnects. At this point in time there seems to be only 2 ways to address/minimize the reconnects:
  1. Slow down the message rate of your data source that's headed for the Spark Core
  2. Edit the [Spark firmware](https://github.com/spark/firmware) source code: under `spark_wiring_tcpclient.h`, look for `TCPCLIENT_BUF_MAX_SIZE` and increase the buffer size (you then have to recompile the spark/firmware binaries and then compile your Spark firmware locally)

##Todo
* Base64 encoding for proper handshaking
* Restructuring and clearer separation of code between Spacebrew and WebSocketClient libraries
* Further work on optimising TCPClient and websocket code for the Spark Core

##Credits
The SpaceBrew portion is a port from the original SpaceBrew-Arduino library by LAB@Rockwell:
https://github.com/labatrockwell/spacebrew-arduino-library

The WebSocketClient implementation started off from ekbduffy's Spark WebSocket...
https://github.com/ekbduffy/spark_websockets

...which is itself a Spark Core derivative of the Arduino Websocket library:
https://github.com/brandenhall/Arduino-Websocket

Many thanks go out to ekbduffy for running through some of the tests.
