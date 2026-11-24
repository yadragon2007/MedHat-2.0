// ---------- Complete heart block ----------
// Atrium and ventricle beat independently (no AV conduction).
// Ventricular escape rhythm = 40 BPM.

const int HB_REST     = 0;
const int HB_CONTRACT = 25;

const unsigned long HB_V_PERIOD     = 60000UL / 40;  // 1500 ms
const unsigned long HB_A_MOVE       = 90;            // each way -> 180 ms contraction
const unsigned long HB_V_MOVE       = 125;           // each way -> 250 ms contraction
const unsigned long HB_ECG_INTERVAL = 5;

bool hbInitialized = false;
bool hbHasAtrial = false, hbHasVentricle = false;
unsigned long hbNextAtrial = 0, hbNextVentricle = 0;
unsigned long hbLastAtrial = 0, hbLastVentricle = 0;
unsigned long hbLastEcg = 0;

// 0 -> 1 -> 0 over [start, end), otherwise 0
float hbTriangle(unsigned long t, unsigned long start, unsigned long end) {
  if (t < start || t >= end) return 0.0f;
  float p = (float)(t - start) / (end - start);
  return p < 0.5f ? p * 2 : 2 - p * 2;
}

// ECG sample for the Serial Plotter (also called from inside moveServo)
void hbSampleEcg() {
  unsigned long now = millis();
  if (now - hbLastEcg < HB_ECG_INTERVAL) return;
  hbLastEcg = now;

  float ecg = 2048;

  if (hbHasAtrial) {
    ecg += 260 * hbTriangle(now - hbLastAtrial, 70, 160);       // P wave
  }
  if (hbHasVentricle) {
    unsigned long t = now - hbLastVentricle;
    ecg -= 850 * hbTriangle(t, 15, 80);                          // QRS
    ecg += 350 * hbTriangle(t, 180, 330);                        // T wave
  }

  Serial.println((int)constrain(ecg, 0.0f, 4095.0f));
}

// One smooth contraction + relaxation
void hbBeat(Servo &servo, unsigned long moveMs) {
  moveServo(servo, HB_REST, HB_CONTRACT, moveMs, hbSampleEcg);
  moveServo(servo, HB_CONTRACT, HB_REST, moveMs, hbSampleEcg);
}

void completeHeartBlock() {
  if (!hbInitialized) {
    hbInitialized = true;
    unsigned long now = millis();
    hbNextAtrial    = now;
    hbNextVentricle = now + HB_V_PERIOD / 2;
    atriumServo.write(HB_REST);
    ventricleServo.write(HB_REST);
  }

  // Atrium: independent rhythm set by the pot
  if ((long)(millis() - hbNextAtrial) >= 0) {
    int bpm = selectBpm();
    hbLastAtrial = millis();
    hbHasAtrial  = true;
    hbNextAtrial = hbLastAtrial + 60000UL / bpm;
    hbBeat(atriumServo, HB_A_MOVE);
  }

  // Ventricle: independent escape rhythm (no signal from the atrium)
  if ((long)(millis() - hbNextVentricle) >= 0) {
    hbLastVentricle  = millis();
    hbHasVentricle   = true;
    hbNextVentricle += HB_V_PERIOD;   // fixed 40 BPM, no drift
    hbBeat(ventricleServo, HB_V_MOVE);
  }

  hbSampleEcg();
}