//SYSTEM_MODE(MANUAL);

#include "Spacebrew.h"

Spacebrew sb;

// name + descriptor for Spacebrew
#define APP_NAME  "NormalNinja"
#define APP_DESC  "Trying out Spark Cores with Spacebrew!"
#define BTNPIN    D3
#define LEDPIN    D1
#define STPIN     D0
#define APIN      A0

int lastBtnState = 0;
int lastAnalog = -1;

void setup() {
  Serial.begin(38400);
  delay(2000);
  Serial.println("Send any char to start");
  while(!Serial.available())
    Spark.process();

  pinMode(BTNPIN,INPUT_PULLUP);
  pinMode(LEDPIN,OUTPUT);
  pinMode(STPIN,OUTPUT);

  //connect to spacebrew library info
  sb.onOpen(onOpen);
  sb.onClose(onClose);
  sb.onError(onError);

  //connect to message callbacks
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
  /*sb.connect("spacebrew.chuank.com", APP_NAME, APP_DESC);*/
  sb.connect("192.168.2.101", APP_NAME, APP_DESC);

}

void loop() {
  // monitor connections first before proceeding with other code
  sb.monitor();

  int analogIn = map(analogRead(APIN),0,4095,0,1023);
  if (analogIn != lastAnalog){
    //sb.send("Analog", analogIn);
    lastAnalog = analogIn;
  }

  int buttonState = digitalRead(BTNPIN);
  if (buttonState != lastBtnState) {
    sb.send("Button", buttonState == LOW);
    lastBtnState = buttonState;
  }

  delay(50);    // a slight delay seems to help
}

void onBooleanMessage(char *name, bool value) {
  //turn the 'digital' LED on and off based on the incoming boolean
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

void onOpen(){
  //Serial.println("### SpaceBrew: connected");
  digitalWrite(STPIN,HIGH);
}

void onClose(int code, char* message){
  //Serial.print("### SpaceBrew: closed");
  digitalWrite(STPIN,LOW);
}

void onError(char* message) {
  //Serial.print("!!! ERROR: ");
  //Serial.println(message);
}
