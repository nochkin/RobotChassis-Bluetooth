#include <SoftwareSerial.h>
#include "SerialCommand.h"

#define LIGHTS 7

#define LMD18200_PWM 0
#define LMD18200_DIR 1
#define LMD18200_BRK 2
uint8_t lmd18200[2][3] = {
  // PWM, DIR, BRK
  {A0, 6, 7},
  {A1, 8, 9},
};

uint8_t l298n[2][2] = {
  // A, B
  // C, D
  {6, 9},
  {10, 11},
};

#define MIN_SPEED 40
#define MAX_SPEED 170

#define BT_RX 3
#define BT_TX 2
#define BT_RESET 5
#define BT_PIO11 4
SoftwareSerial mySerial(BT_RX, BT_TX); // RX, TX

SerialCommand myCmd(mySerial);

void setup() {
  digitalWrite(BT_RESET, HIGH);
  digitalWrite(BT_PIO11, LOW);
  pinMode(BT_RESET, OUTPUT);
  pinMode(BT_PIO11, OUTPUT);

  Serial.begin(38400);
  mySerial.begin(38400);

  setupBT();
  
  setup_lights();
  
  // setup_18200();
  setup_298();
  
  myCmd.addCommand("L", doLights);
  myCmd.addCommand("STOP", doStop);
  myCmd.addCommand("M", doMotors);
  myCmd.addDefaultHandler(unknownCommand);
  Serial.println("Start");
}

void loop() {
  myCmd.readSerial();
}

void setup_lights() {
  pinMode(LIGHTS, OUTPUT);
  digitalWrite(LIGHTS, LOW);
}

void setup_18200() {
  for (int ii=0; ii<=2; ii++) {
    pinMode(lmd18200[ii][LMD18200_PWM], OUTPUT);
    pinMode(lmd18200[ii][LMD18200_DIR], OUTPUT);
    pinMode(lmd18200[ii][LMD18200_BRK], OUTPUT);
    analogWrite(lmd18200[ii][LMD18200_PWM], 0);
    digitalWrite(lmd18200[ii][LMD18200_DIR], LOW);
    digitalWrite(lmd18200[ii][LMD18200_BRK], HIGH);
  }
}

void setup_298() {
  for (int ii=0; ii<=2; ii++) {
    pinMode(l298n[ii][0], OUTPUT);
    pinMode(l298n[ii][1], OUTPUT);
    analogWrite(l298n[ii][0], 0);
    analogWrite(l298n[ii][1], 0);
  }
}

void unknownCommand() {
  mySerial.println("*Error: bad command");
}

void doLights() {
  char *arg = myCmd.next();
  if (arg != NULL) {
    int iarg = atoi(arg);
    if (iarg == 0) {
      digitalWrite(LIGHTS, LOW);
      mySerial.println("OK LIGHTS OFF");
    } else {
      digitalWrite(LIGHTS, HIGH);
      mySerial.println("OK LIGHTS ON");
    }
  } else {
    mySerial.println("*ERR: Missing LIGHTS arguments");
  }
}

void doStop() {
  setDrive(0, 0);
  mySerial.println("OK STOP");
}

void doMotors() {
  char *arg1 = myCmd.next();
  char *arg2 = myCmd.next();
  if ((arg1 != NULL) && (arg2 !=NULL)) {
    int iarg1 = int(atoi(arg1) * ((MAX_SPEED - MIN_SPEED) / 100.0));
    int iarg2 = int(atoi(arg2) * ((MAX_SPEED - MIN_SPEED) / 100.0));
    if (iarg1 > 0 ) { iarg1 += MIN_SPEED; }
    if (iarg1 < 0 ) { iarg1 -= MIN_SPEED; }
    if (iarg2 > 0 ) { iarg2 += MIN_SPEED; }
    if (iarg2 < 0 ) { iarg2 -= MIN_SPEED; }
    setDrive(iarg1, iarg2);
    mySerial.println("OK MOTORS");
  } else {
    mySerial.println("*ERR: Missing MOTOR arguments");
  }
}

void setDrive(int left, int right) {
  Serial.print("drive ");
  Serial.print(left);
  Serial.print(":");
  Serial.println(right);
  // setDriveId_18200(0, left);
  // setDriveId_18200(1, right);
  setDriveId_298(0, left);
  setDriveId_298(1, right);
}

void setDriveId_298(int driveId, int value) {
  if (value > 0) {
    analogWrite(l298n[driveId][0], 0);
    analogWrite(l298n[driveId][1], value);
  } else if (value < 0) {
    analogWrite(l298n[driveId][0], -value);
    analogWrite(l298n[driveId][1], 0);
  } else {
    analogWrite(l298n[driveId][0], 0);
    analogWrite(l298n[driveId][1], 0);
  }
}

void setDriveId_18200(int driveId, int value) {
  if (value > 0) {
    digitalWrite(lmd18200[driveId][LMD18200_BRK], LOW);
    analogWrite(lmd18200[driveId][LMD18200_PWM], value);
    digitalWrite(lmd18200[driveId][LMD18200_DIR], HIGH);
  } else if (value < 0) {
    digitalWrite(lmd18200[driveId][LMD18200_BRK], LOW);
    analogWrite(lmd18200[driveId][LMD18200_PWM], -value);
    digitalWrite(lmd18200[driveId][LMD18200_DIR], HIGH);
  } else {
    analogWrite(lmd18200[driveId][LMD18200_PWM], 0);
    digitalWrite(lmd18200[driveId][LMD18200_PWM], LOW);
    digitalWrite(lmd18200[driveId][LMD18200_BRK], HIGH);
  }
}

void setupBT() {
  //btATMode();
  //printBTInfo();
  btCommMode();
}

void btAT(){
  while(mySerial.available()){
    char myChar = mySerial.read();
    Serial.print(myChar);
  }

  while(Serial.available()){
   char myChar = Serial.read();
   mySerial.print(myChar);
  }
}

void printBTInfo() {
  Serial.print("BT Address: ");
  printBTCommand("ADDR?");
  Serial.print("BT Name: ");
  printBTCommand("NAME?");
  Serial.print("BT Passkey: ");
  printBTCommand("PSWD?");
  printBTCommand("UART=38400,1,0");
  //printBTCommand("RESET");
}

void printBTCommand(const char *cmd) {
  mySerial.write("AT+");
  mySerial.write(cmd);
  mySerial.write("\r\n");
  mySerial.setTimeout(500);

  String data = mySerial.readStringUntil('\r');
  Serial.println(data);
  mySerial.flush();
}

void btReset(byte pio11) {
  mySerial.flush();
  delay(500);
  digitalWrite(BT_PIO11, pio11);
  digitalWrite(BT_RESET, LOW);
  delay(100);
  digitalWrite(BT_RESET, HIGH);
  delay(3000);
}

void btATMode() {
  btReset(HIGH);
}

void btCommMode() {
  btReset(LOW);
}
