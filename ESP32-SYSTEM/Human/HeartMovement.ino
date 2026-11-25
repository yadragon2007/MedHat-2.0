int selectBpm() {
  int raw = analogRead(bpmPotentiometerPin);
  return map(raw, 0, 4095, 40, 150);   // removed the 50 ms delay
}

void moveServo(Servo &s, int ang1, int ang2, unsigned long durationMs, void (*onTick)()) {
  if (modeChanged) return;             // abandon the beat immediately

  if (durationMs == 0 || ang1 == ang2) {
    s.write(ang2);
    return;
  }

  unsigned long start = millis();
  unsigned long elapsed;

  while ((elapsed = millis() - start) < durationMs) {
    pollButtons();
    if (modeChanged) return;           // don't finish the old mode's movement

    float t = (float)elapsed / durationMs;
    s.write(ang1 + (int)((ang2 - ang1) * t));
    if (onTick) onTick();
    delay(10);
  }
  s.write(ang2);
}

void moveServo(Servo &s, int ang1, int ang2, unsigned long durationMs) {
  moveServo(s, ang1, ang2, durationMs, nullptr);
}