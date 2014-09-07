/*
 NanoBoard

 mod 7 July, 2011
 by tiisai.dip.jp


 BASED ON  HelloBoardV2

 Created 30 October. 2009
 by PINY and Song Hojun.

 Add single/double motor function on 27 Dec 2011
 by Kazuhiro Abe <abee@squeakland.jp>

 Add motor mode toggle function on 17 Aug 2012
 Add diagnostic mode 20 Oct 2012
 Add LED interval off 18 Jan 2013

 by tiisai.dip.jp

 */
#include <EEPROM.h>
#include <NewPing.h>

#define TRIGGER_PIN  11  // Arduino pin tied to trigger pin on the ultrasonic sensor.
#define ECHO_PIN     15  // Arduino pin tied to echo pin on the ultrasonic sensor.
#define MAX_DISTANCE 108 // Maximum distance we want to ping for (in centimeters). Maximum sensor distance is rated at 400-500cm.
NewPing sonar(TRIGGER_PIN, ECHO_PIN, MAX_DISTANCE); // NewPing setup of pins and maximum distance.

// Sensor <--> Analog port mapping for NanoBoard
#define SoundSensor  0
#define ResistanceA  1
#define ResistanceB  2
#define ResistanceC  3
#define ResistanceD  4
#define LightSensor  5
#define Slider       6
#define Button       7

// motor port mapping
#define PWM_PIN_A     9
#define THISWAY_PIN_A 7
#define THATWAY_PIN_A 8

#define PWM_PIN_B     10
#define THISWAY_PIN_B 5
#define THATWAY_PIN_B 6

// other ports
#define RXD           0
#define TXD           1
#define GPIO2         2
#define GPIO3         3
#define GPIO4         4
#define SERVO         11
#define LED           13

// EEPROM MODE setting
#define EEPROMADS 1
#define MODES 4  // 0:one motor, 1:two motor, 2: 2motor+soner  3:bluebot 0xFF:diagnostic

int sliderValue      = 0;
int lightValue       = 0;
int soundValue       = 0;
int buttonValue      = 0;
int resistanceAValue = 0;
int resistanceBValue = 0;
int resistanceCValue = 0;
int resistanceDValue = 0;

unsigned long lastIncommingMicroSec = 0;

uint8_t incomingByte;
uint8_t motorMode;

#define sensorChannels 8
#define maxNumReadings 30
int smoothingValues[sensorChannels][maxNumReadings];
int smoothingIndex[sensorChannels];
int smoothingTotal[sensorChannels];

// motor variables
byte motorDirectionA = 0;
byte isMotorOnA      = 0;
byte motorPowerA     = 0;

byte motorDirectionB = 0;
byte isMotorOnB      = 0;
byte motorPowerB     = 0;

void setup()
{
  setupSmoothing();

  pinMode(LED, OUTPUT);     // NanoBoard LED port

  // set pin mode for motor
  pinMode(PWM_PIN_A,     OUTPUT);
  pinMode(THISWAY_PIN_A, OUTPUT);
  pinMode(THATWAY_PIN_A, OUTPUT);
  pinMode(PWM_PIN_B,     OUTPUT);
  pinMode(THISWAY_PIN_B, OUTPUT);
  pinMode(THATWAY_PIN_B, OUTPUT);

  motorMode = EEPROM.read(EEPROMADS); // read current mode from EEPROM
  if (motorMode == 0xFF)  {  //First PowerON then diagnostic
    EEPROM.write(EEPROMADS, ++motorMode);
    while (readButton() > 10) {
      for (int i = 2; i < 13; i++) {
        digitalWrite(i, HIGH);   // set NanoBoard LED on
        delay(200);
        digitalWrite(i, LOW);   // set NanoBoard LED off
      }
    }
  }
  else if (readButton() < 10) {       // check button pushed
    motorMode = (motorMode + 1) % MODES;  //change mode
    EEPROM.write(EEPROMADS, motorMode);
  }

  for (int i = 0; i < motorMode; i++) {
    digitalWrite(LED, HIGH);
    delay(500);
    digitalWrite(LED, LOW);
    delay(500);
  }

  Serial.begin(38400);
}

void setupSmoothing()
{
  for (int i = 0; i < sensorChannels; i++) {
    for (int j = 0 ; j < maxNumReadings ; j++) {
      smoothingValues[i][j] = 0;
    }
    smoothingTotal[i] = 0;
    smoothingIndex[i] = 0;
  }
}

int smoothingValue(int channel, int value, int numReadings)
{
  int index = smoothingIndex[channel];

  smoothingTotal[channel] += (value - smoothingValues[channel][index]);
  smoothingValues[channel][index] = value;
  smoothingIndex[channel]++;
  if (smoothingIndex[channel] >= numReadings) {
    smoothingIndex[channel] = 0;
  }
  return int(round(smoothingTotal[channel] / numReadings));
}

void loop()
{
  if (motorMode == 3) {
    bluebotloop();
  }
  else {
    readSensors();
    if (Serial.available() > 0) {
      incomingByte = Serial.read();

      digitalWrite(LED, HIGH);
      lastIncommingMicroSec = 0;

      // rotate motor
      motorDirectionA = (incomingByte >> 7) & B1;
      if (motorMode == 0) {  // if mode=0 then one motor
        motorPowerA     = (incomingByte & B1111111) * 2;
        motorPowerB     = motorPowerA;
        motorDirectionB = motorDirectionA;
        isMotorOnB      = isMotorOnA;
      }
      else {
        motorPowerA     = ((incomingByte >> 4) & B111) * 36;
        motorPowerB     = (incomingByte & B111) * 36;
        motorDirectionB = (incomingByte >> 3) & B1;
        isMotorOnB      = motorPowerB > 0;
      }
      isMotorOnA = motorPowerA > 0;

      analogWrite(PWM_PIN_A, motorPowerA);
      digitalWrite(THISWAY_PIN_A, motorDirectionA & isMotorOnA);
      digitalWrite(THATWAY_PIN_A, ~motorDirectionA & isMotorOnA);

      analogWrite(PWM_PIN_B, motorPowerB);
      digitalWrite(THISWAY_PIN_B, motorDirectionB & isMotorOnB);
      digitalWrite(THATWAY_PIN_B, ~motorDirectionB & isMotorOnB);

      sendFirstSecondBytes(15, 0x04);
      sendFirstSecondBytes(0, resistanceDValue);
      sendFirstSecondBytes(1, resistanceCValue);
      sendFirstSecondBytes(2, resistanceBValue);
      sendFirstSecondBytes(3, buttonValue);
      sendFirstSecondBytes(4, resistanceAValue);
      sendFirstSecondBytes(5, lightValue);
      sendFirstSecondBytes(6, soundValue);
      sendFirstSecondBytes(7, sliderValue);
    }
    else {
      if (++lastIncommingMicroSec > 200) {
        digitalWrite(LED, LOW);  // set NanoBoard LED off
      }
    }
  }
}

void readSensors()
{
  sliderValue = readSlider();
  lightValue  = readLight();
  soundValue  = readSound();

  buttonValue = readButton();
  if (buttonValue < 10) {
     digitalWrite(13, HIGH);
     digitalWrite(12, HIGH);
  }
  else {
     digitalWrite(13, LOW);
     digitalWrite(12, LOW);
  }

  if (motorMode == 2) {  // if mode!=0 then two motor
    delay(5);            // Wait 15ms between pings (about 20 pings/sec). 29ms should be the shortest delay between pings.
    resistanceAValue = sonar.ping() / 5 ; // Send ping, get ping time in microseconds (uS).
  }
  else {  // if mode=0 then one motor
    resistanceAValue = readResistance(ResistanceA);
  }

  resistanceBValue = readResistance(ResistanceB);
  resistanceCValue = readResistance(ResistanceC);
  resistanceDValue = readResistance(ResistanceD);
}

int readButton()
{
  return analogRead(Button);
}

int readResistance(int adc)
{
  int value = analogRead(adc);
  value = smoothingValue(adc, value, 5);
  if (value == 1022) value = 1023;
  return value;
}

int readSlider()
{
  int sliderValue = analogRead(Slider);
  sliderValue = smoothingValue(Slider, sliderValue, 3);
  return sliderValue;
}

int readLight()
{
  int light = analogRead(LightSensor);

  // calibrate: make s-curve
  const int mid  = 600;
  const int mid2 = 900;
  if (light < mid) {
    light = int(round((40.0/mid)*light));
  }
  else if (light < mid2) {
    light = int(round((mid2 - 40)/(mid2 - float(mid))* light) - 1680);
  }
  light = constrain(light, 0, 1023);

  light = smoothingValue(LightSensor,light, 20);
  return light;
}

int readSound()
{
  int sound = analogRead(SoundSensor);
  sound = smoothingValue(SoundSensor,sound, 20);
  // noise ceiling
  if (sound < 60) {
    sound = 0;
  }
  return sound;
}

void sendFirstSecondBytes(byte channel, int value)
{
  byte firstByte  = 0b10000000 | (channel << 3) | (value >> 7);
  byte secondByte = 0b01111111 & value;
  Serial.write(firstByte);
  Serial.write(secondByte);
}

/* Magician Chassis Bluebots Bluetooth
 Hobbytronics ltd, 2012
 */
int left  = 0;
int right = 0;

unsigned char data_packet[4];
unsigned char packet_index = 0;

void bluebotloop()
{
  // check if data has been sent from the computer:
  if (Serial.available()) {
    // read the most recent byte:
    data_packet[packet_index] = Serial.read();
    packet_index++;
    if (packet_index == 4) {
      // full packet received
      process_packet();
      setMotors();
    }
  }
}

void process_packet()
{
  int lspeed, lturn;

  packet_index=0;
  if (data_packet[0] == 43 || data_packet[0] ==45) {
    if (data_packet[0]==43) {
      lturn=data_packet[1];
    }
    else lturn=data_packet[1]*-1;

    if (data_packet[2]==43) lspeed=data_packet[3]*2;
    else lspeed=data_packet[3]*-2;

    left  = (lspeed+lturn);
    left  = constrain(left, -255, 255);
    right = (lspeed-lturn);
    right = constrain(right, -255, 255);
  }
}

void setMotors()
{
  int vLeft  = left;
  int vRight = right;
  if (vLeft < 0) {
    analogWrite(PWM_PIN_A, abs(vLeft));
    digitalWrite(THISWAY_PIN_A, 0);
    digitalWrite(THATWAY_PIN_A, 1);
  }
  else{
    analogWrite(PWM_PIN_A, abs(vLeft));
    digitalWrite(THISWAY_PIN_A, 1);
    digitalWrite(THATWAY_PIN_A, 0);
  }

  if (vRight < 0) {
    analogWrite(PWM_PIN_B, abs(vRight));
    digitalWrite(THISWAY_PIN_B, 0);
    digitalWrite(THATWAY_PIN_B, 1);
  }
  else {
    analogWrite(PWM_PIN_B, abs(vRight));
    digitalWrite(THISWAY_PIN_B, 1);
    digitalWrite(THATWAY_PIN_B, 0);
  }
}

