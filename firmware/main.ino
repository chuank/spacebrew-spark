SYSTEM_MODE(MANUAL);      // MANUAL mode; full attention to Spacebrew websocket server required

#include "Spacebrew.h"
Spacebrew sb;

// name + descriptor for Spacebrew
#define APP_NAME  "SparkCore"
#define APP_DESC  "Trying out Spark Cores with Spacebrew!"

#define BTNPIN    D3      // button (pullup), connected as Spacebrew publisher
#define LEDPIN    D1      // LED to D1, connected as Spacebrew subscriber
#define STPIN     D0      // when ON, Spacebrew connected
#define APIN      A0      // analog pin, connected as Spacebrew publisher

int lastBtnState = 0;
int lastAnalog = -1;

void setup() {
  WiFi.connect();             // not using Spark Cloud, so turn on WiFi manually
  while(!WiFi.ready());       // wait for WiFi to establish connection

  Serial.begin(38400);
  delay(2000);

  // debug purposes - uncomment next 2 lines to start up automatically
  Serial.println("Send any char to start");
  while(!Serial.available());

  pinMode(BTNPIN,INPUT_PULLUP);
  pinMode(LEDPIN,OUTPUT);
  pinMode(STPIN,OUTPUT);

  //connect to Spacebrew callbacks
  sb.onOpen(onOpen);
  sb.onClose(onClose);
  sb.onError(onError);

  //connect to Spacebrew message callbacks
  sb.onBooleanMessage(onBooleanMessage);
  sb.onStringMessage(onStringMessage);
  sb.onRangeMessage(onRangeMessage);

  //register publishers and subscribers
  sb.addPublish("Analog", SB_RANGE);
  sb.addPublish("Button", SB_BOOLEAN);
  sb.addSubscribe("LED", SB_BOOLEAN);
  sb.addSubscribe("Text", SB_STRING);
  sb.addSubscribe("Range", SB_RANGE);

  //connect to the spacebrew server
  sb.connect("192.168.2.101", APP_NAME, APP_DESC);      // 'localhost' does not work, use IP instead
}

void loop() {
  // always monitor Spacebrew connection
  sb.monitor();

  int analogIn = map(analogRead(APIN),0,4095,0,1023);
  if (analogIn != lastAnalog){
    sb.send("Analog", analogIn);
    lastAnalog = analogIn;
  }

  int buttonState = digitalRead(BTNPIN);
  if (buttonState != lastBtnState) {
    sb.send("Button", buttonState == LOW);
    lastBtnState = buttonState;
  }

  delay(10);       // slow things down a touch
}


// Spacebrew callbacks
void onOpen(){
  Serial.println("### SpaceBrew: connected");
  sb.send("R!", true);   // force a non-existent reset message on server side
  digitalWrite(STPIN,HIGH);
}

void onClose(int code, char* message){
  Serial.println("### SpaceBrew: closed");
  digitalWrite(STPIN,LOW);
}

void onError(char* message) {
  Serial.print("!!! ERROR: ");
  Serial.println(message);
}

void onBooleanMessage(char *name, bool value) {
  //turn the LED on and off based on the incoming boolean
  digitalWrite(LEDPIN, value ? HIGH : LOW);
}

void onStringMessage(char *name, char *value) {
  Serial.print("SB_STRING: ");
  Serial.println(value);
}

void onRangeMessage(char *name, int value) {
  Serial.print("SB_RANGE: ");
  Serial.println(value);
}
