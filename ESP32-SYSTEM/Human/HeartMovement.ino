int selectBpm() {
  int raw = analogRead(bpmPotentiometerPin);
  int bpm = map(raw, 0, 4095, 40, 150);
  delay(50);
  return bpm;
}

void moveServo(Servo &s, int ang1, int ang2, unsigned long durationMs) {
  if (durationMs == 0 || ang1 == ang2) {
    s.write(ang2);
    return;
  }

  unsigned long start = millis();
  unsigned long elapsed;

  while ((elapsed = millis() - start) < durationMs) {
    float t = (float)elapsed / durationMs;  // 0.0 -> 1.0
    int angle = ang1 + (int)((ang2 - ang1) * t);
    Serial.println(angle);

    s.write(angle);
    delay(10);  // ~100 updates per second
  }
  s.write(ang2);  // make sure it lands exactly on target
}