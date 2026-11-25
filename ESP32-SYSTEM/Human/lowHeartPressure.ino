// ---------- Low Blood Pressure ----------
// Atrium and ventricle contract together.
// Both servos move smoothly.
// BPM is controlled by the potentiometer.

void lowBloodPressure() {

  const int LB_REST = 0;
  const int LB_CONTRACT = 12;

  const unsigned long LB_MOVE_TIME = 100;
  const unsigned long LB_ECG_INTERVAL = 5;

  static bool initialized = false;
  static bool hasBeat = false;

  static unsigned long nextBeat = 0;
  static unsigned long lastBeat = 0;
  static unsigned long lastServoUpdate = 0;
  static unsigned long lastECG = 0;

  static unsigned long beatPeriod = 1000;

  unsigned long now = millis();

  // Initialize the simulation
  if (!initialized) {
    initialized = true;

    nextBeat = now;
    atriumServo.write(LB_REST);
    ventricleServo.write(LB_REST);
  }

  // Start a new coordinated heartbeat
  if ((long)(now - nextBeat) >= 0) {

    int bpm = selectBpm();

    beatPeriod = 60000UL / bpm;

    lastBeat = now;
    nextBeat = now + beatPeriod;

    hasBeat = true;
  }

  // Smoothly move both servos
  if (hasBeat && now - lastServoUpdate >= 10) {

    lastServoUpdate = now;

    unsigned long t = now - lastBeat;

    int angle = LB_REST;

    // Contract gradually
    if (t < LB_MOVE_TIME) {

      float progress = (float)t / LB_MOVE_TIME;

      angle = LB_REST +
              (int)((LB_CONTRACT - LB_REST) * progress);
    }

    // Relax gradually
    else if (t < LB_MOVE_TIME * 2) {

      float progress =
          (float)(t - LB_MOVE_TIME) / LB_MOVE_TIME;

      angle = LB_CONTRACT -
              (int)((LB_CONTRACT - LB_REST) * progress);
    }

    // Rest until the next beat
    else {
      angle = LB_REST;
    }

    atriumServo.write(angle);
    ventricleServo.write(angle);
  }

  // ECG simulation for Serial Plotter
  if (now - lastECG >= LB_ECG_INTERVAL) {

    lastECG = now;

    float ecg = 2048;

    if (hasBeat) {

      unsigned long t = now - lastBeat;

      // P wave
      if (t >= 70 && t < 160) {

        float p = (float)(t - 70) / 90;

        ecg += 260 *
               (p < 0.5 ? p * 2 : 2 - p * 2);
      }

      // QRS complex
      if (t >= 15 && t < 80) {

        float qrs = (float)(t - 15) / 65;

        ecg -= 850 *
               (qrs < 0.5 ? qrs * 2 : 2 - qrs * 2);
      }

      // T wave
      if (t >= 180 && t < 330) {

        float tw = (float)(t - 180) / 150;

        ecg += 350 *
               (tw < 0.5 ? tw * 2 : 2 - tw * 2);
      }
    }

    Serial.println((int)constrain(ecg, 0.0f, 4095.0f));
  }
}