spacebrew-sparkcore
===================

SpaceBrew + Spark Core integration

This is a test and work-in-progress towards getting SpaceBrew and the Spark Core to talk nice to one another.

The SpaceBrew portion is a port from the original SpaceBrew-Arduino library by LAB@Rockwell:
https://github.com/labatrockwell/spacebrew-arduino-library

Websocket implementation is from ekbduffy's Spark WebSocket:
https://github.com/ekbduffy/spark_websockets



Issues
------
Spark Cores are able to establish contact with the SpaceBrew server, including registration of publishers+subscribers.

Code is not 100% working.

Connection dropouts are common, especially with data going from Spark Core to the SpaceBrew server.

Ping issues - Spacebrew fails to continue pinging after the dropout, effectively terminating the websocket.


