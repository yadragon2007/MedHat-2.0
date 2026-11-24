
void HeartMode() {
  for (int i = 0; i < NUM_MODES; i++) {
    bool now = digitalRead(BTNs[i]);
    if (lastState[i] == HIGH && now == LOW) {  // new press only
      currentMode = i;
      for (int j = 0; j < NUM_MODES; j++) {
        digitalWrite(LEDs[j], j == i ? HIGH : LOW);
      }
      delay(30);
    }
    lastState[i] = now;
  }

  switch (currentMode) {
    case 0: normalHeartBeat(); break;
    case 1: lowBloodPressure(); break;
    case 2: ventircularFirbrillation(); break;
    case 3: completeHeartBlock(); break;
  }
}

