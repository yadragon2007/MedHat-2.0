unsigned long lastPressMs[4] = { 0, 0, 0, 0 };

// Safe to call from anywhere, including inside moveServo()
void pollButtons() {
  for (int i = 0; i < NUM_MODES; i++) {
    bool now = digitalRead(BTNs[i]);
    if (lastState[i] == HIGH && now == LOW && millis() - lastPressMs[i] > 30) {
      lastPressMs[i] = millis();
      if (i != currentMode) {
        currentMode = i;
        modeChanged = true;
        for (int j = 0; j < NUM_MODES; j++) digitalWrite(LEDs[j], j == i ? HIGH : LOW);
      }
    }
    lastState[i] = now;
  }
}

// Put everything back to a clean state when switching modes
void resetModeState() {
  atriumServo.write(0);
  ventricleServo.write(0);

  // complete heart block
  hbInitialized = false;
  hbHasAtrial = false;
  hbHasVentricle = false;

  // ECG task restarts cleanly
  ecgBeatTrigger = false;
}

void HeartMode() {
  pollButtons();
  if (modeChanged) {
    modeChanged = false;
    resetModeState();
  }

  switch (currentMode) {
    case 0: normalHeartBeat(); break;
    case 1: lowBloodPressure(); break;
    case 2: ventircularFirbrillation(); break;
    case 3: completeHeartBlock(); break;
  }
}