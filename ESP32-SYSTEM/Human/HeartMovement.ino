int selectBpm() {
  int raw = analogRead(bpmPotentiometerPin);
  int bpm = map(raw, 0, 4095, 40, 150);
  delay(50);
  return bpm;
}

// New version: calls onTick() on every step so other work can keep running
void moveServo(Servo &s, int ang1, int ang2, unsigned long durationMs, void (*onTick)()) {
  if (durationMs == 0 || ang1 == ang2) {
    s.write(ang2);
    return;
  }

  unsigned long start = millis();
  unsigned long elapsed;

  while ((elapsed = millis() - start) < durationMs) {
    float t = (float)elapsed / durationMs;  // 0.0 -> 1.0
    s.write(ang1 + (int)((ang2 - ang1) * t));
    if (onTick) onTick();
    delay(10);  // ~100 updates per second
  }
  s.write(ang2);  // land exactly on target
}

// Original signature still works
void moveServo(Servo &s, int ang1, int ang2, unsigned long durationMs) {
  moveServo(s, ang1, ang2, durationMs, nullptr);
}