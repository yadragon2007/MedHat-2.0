// ---------- ECG on ecgSignalPin (GPIO13): PWM -> RC filter -> 0-3.3 V ----------
// Uses currentMode and ecgSignalPin, which are defined in the main file.
// The BPM comes from selectBpm() and reaches this file through ecgStartBeat().

const int ECG_PWM_FREQ = 40000;                    // 40 kHz carrier for the RC filter
const int ECG_PWM_RES  = 10;                       // duty 0..1023 -> 0..3.3 V after filtering
const int ECG_MAX_DUTY = (1 << ECG_PWM_RES) - 1;

volatile int  ecgBpm = 70;                         // BPM chosen by selectBpm()
volatile bool ecgBeatTrigger = false;              // set at the start of each servo beat

// One Gaussian bump. t, center and sigma are in ms; amp is a fraction of 3.3 V
static inline float ecgBump(float t, float amp, float center, float sigma) {
  float d = (t - center) / sigma;
  return amp * expf(-0.5f * d * d);
}

// Normal PQRST for one beat. t = ms since the beat started, period = ms per beat.
// Positions match normalHeartBeat(): the atrium moves during the first 2/8 of the
// beat, the ventricle contracts from 2/8 to 5/8.
float ecgNormalSample(float t, float period) {
  float rPos = 0.25f * period;                           // R peak as the ventricle starts
  float v = 0.25f;                                       // baseline (~0.8 V)
  v += ecgBump(t,  0.08f, 0.06f * period, 18.0f);        // P wave (atrial contraction)
  v += ecgBump(t, -0.06f, rPos - 18.0f,   8.0f);         // Q wave
  v += ecgBump(t,  0.60f, rPos,           9.0f);         // R wave (tall spike, ~2.8 V)
  v += ecgBump(t, -0.12f, rPos + 18.0f,   9.0f);         // S wave
  v += ecgBump(t,  0.15f, 0.50f * period, 0.05f * period); // T wave (repolarization)
  if (v < 0.0f) v = 0.0f;
  if (v > 1.0f) v = 1.0f;
  return v;
}

void ecgWriteLevel(float level) {                        // level 0.0 .. 1.0
  ledcWrite(ecgSignalPin, (uint32_t)(level * ECG_MAX_DUTY));
}

// Runs on its own task so moveServo()'s blocking loops and delay() can't distort it
void ecgTask(void *param) {
  unsigned long beatStart = micros();
  for (;;) {
    if (currentMode == 0) {                              // normal heartbeat mode only
      if (ecgBeatTrigger) {
        ecgBeatTrigger = false;
        beatStart = micros();                            // restart in sync with the servos
      }
      float period  = 60000.0f / (float)ecgBpm;          // ms per beat
      float elapsed = (micros() - beatStart) / 1000.0f;
      if (elapsed > period) elapsed = period;            // sit on baseline until next beat
      ecgWriteLevel(ecgNormalSample(elapsed, period));
    } else {
      ecgWriteLevel(0.0f);                               // flat 0 V in the other modes
      beatStart = micros();
    }
    vTaskDelay(1);                                       // ~1 kHz sampling
  }
}

// Called from normalHeartBeat() right after selectBpm()
void ecgStartBeat(int bpm) {
  ecgBpm = bpm;
  ecgBeatTrigger = true;
}

// Call once from setup(), AFTER the servos are attached
void ecgSetup() {
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  ledcAttach(ecgSignalPin, ECG_PWM_FREQ, ECG_PWM_RES);   // core picks a free channel
#else
  const int ECG_PWM_CHANNEL = 4;                         // away from the servos' channels
  ledcSetup(ECG_PWM_CHANNEL, ECG_PWM_FREQ, ECG_PWM_RES);
  ledcAttachPin(ecgSignalPin, ECG_PWM_CHANNEL);
#endif
  xTaskCreatePinnedToCore(ecgTask, "ecgTask", 4096, NULL, 1, NULL, 0);
}