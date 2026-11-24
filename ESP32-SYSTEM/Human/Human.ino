#include <ESP32Servo.h>
Servo atriumServo;
Servo ventricleServo;

const int atriumServoPin = 18;
const int ventricleServoPin = 19;

const int GreenLED = 23;
const int RedLED1 = 25;
const int RedLED2 = 26;
const int RedLED3 = 27;
const int LEDs[4] = { GreenLED, RedLED1, RedLED2, RedLED3 };

const int btn1 = 17;
const int btn2 = 16;
const int btn3 = 4;
const int btn4 = 33;
const int BTNs[4] = { btn1, btn2, btn3, btn4 };

const int bpmPotentiometerPin = 34;

const int NUM_MODES = 4;
int currentMode = -1;  // -1 = nothing selected
bool lastState[4] = { HIGH, HIGH, HIGH, HIGH };

const int ecgSignalPin = 13;



void setup() {
  atriumServo.attach(atriumServoPin, 500, 2500);
  ventricleServo.attach(ventricleServoPin, 500, 2500);
  atriumServo.write(0);
  ventricleServo.write(0);

  for (int i = 0; i < NUM_MODES; i++) {
    pinMode(BTNs[i], INPUT_PULLUP);
    pinMode(LEDs[i], OUTPUT);
    digitalWrite(LEDs[i], LOW);
  }
  // ecg signal
  pinMode(ecgSignalPin, OUTPUT);
  

  Serial.begin(115200);
  Serial.println("ESP32 Serial Monitor ready");

  analogReadResolution(12);
}

void loop() {
  HeartMode();
}


void normalHeartBeat() {
  int bpm = selectBpm();
  float bps = bpm / 60.0;
  float beatPirod = 1.0 / bps;

  unsigned long StartTime = millis();
  // atrium(50, 50);
  moveServo(atriumServo, 0, 25, beatPirod*1000/8);
  moveServo(atriumServo, 25, 0, beatPirod*1000/8);
  moveServo(ventricleServo, 0, 25, beatPirod*1000/8*3);
  moveServo(ventricleServo, 25, 0, beatPirod*1000/8*3);
  unsigned long middel = millis();
  // ventricle(350, 350);
  unsigned long finalTime = millis();
}

void lowBloodPressure() {
  return;
}
void ventircularFirbrillation() {
  return;
}