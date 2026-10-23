// Line follower + TCS3200 red-stop — ESP32-S3 (Arduino core 3.x)
//
// The colour sensor is read in the background with a non-blocking state
// machine, so the line following keeps running at full speed.

// ---- Motor pins ----
const int ENA_F = 7,  IN1_F = 8,  IN2_F = 9;   // front-left
const int ENB_F = 10, IN3_F = 11, IN4_F = 12;  // front-right
const int ENA_R = 13, IN1_R = 14, IN2_R = 15;  // physically REAR-RIGHT
const int ENB_R = 16, IN3_R = 17, IN4_R = 18;  // physically REAR-LEFT

// ---- IR sensor pins ----
const int leftIR  = 2;
const int rightIR = 1;

// ---- TCS3200 pins ----
// VCC + S0 -> 3.3V, GND -> GND, OE -> GND
const int S1_PIN  = 5;    // driven LOW -> 20% frequency scaling
const int S2_PIN  = 6;
const int S3_PIN  = 21;
const int OUT_PIN = 39;

// ---- Speeds ----
const int SPEED      = 100;
const int FAST_SPEED = 130;
const int SLOW_SPEED = 0;

// ---- Red detection (from your readings) ----
// Red:   R ~8200-9700 Hz, R/G ~1.45-1.9
// Tiles: R ~3700-5900 Hz, R/G ~1.1-1.3
// Black: R ~2100-2300 Hz, R/G ~1.07
const uint32_t RED_MIN_HZ = 6000;   // R must be at least this bright
const float    RED_RATIO  = 1.40;   // R must be this many times G
const int      RED_CONFIRM = 1;     // consecutive red readings before stopping
const uint32_t STOP_MS     = 3000;  // how long to stop on red
const int      CLEAR_CONFIRM = 3;   // non-red readings needed before red can trigger again

enum RoverState { RUNNING, PAUSED, LEAVING_RED };
RoverState roverState = RUNNING;
uint32_t pauseStart = 0;
int clearStreak = 0;
uint32_t readingCount = 0;          // increments after each R+G reading
uint32_t lastSeenReading = 0;

// ---- Colour measurement timing ----
const uint32_t SETTLE_MS = 3;       // filter mux settle time
const uint32_t WINDOW_MS = 15;      // counting window per colour

volatile uint32_t edgeCount = 0;
void IRAM_ATTR onEdge() { edgeCount++; }

enum ColorPhase { SETTLE_RED, COUNT_RED, SETTLE_GREEN, COUNT_GREEN };
ColorPhase phase = SETTLE_RED;
uint32_t phaseStart = 0;
uint32_t redHz = 0, greenHz = 0;
int redStreak = 0;
bool redDetected = false;

void selectFilter(bool s2, bool s3) {
  digitalWrite(S2_PIN, s2);
  digitalWrite(S3_PIN, s3);
}

// Call every loop; never blocks. Updates redDetected after each R+G pair.
void updateColor() {
  uint32_t now = millis();
  uint32_t elapsed = now - phaseStart;

  switch (phase) {
    case SETTLE_RED:
      if (elapsed >= SETTLE_MS) { edgeCount = 0; phaseStart = now; phase = COUNT_RED; }
      break;

    case COUNT_RED:
      if (elapsed >= WINDOW_MS) {
        redHz = (edgeCount * 1000UL) / elapsed;
        selectFilter(HIGH, HIGH);                 // green filter
        phaseStart = now; phase = SETTLE_GREEN;
      }
      break;

    case SETTLE_GREEN:
      if (elapsed >= SETTLE_MS) { edgeCount = 0; phaseStart = now; phase = COUNT_GREEN; }
      break;

    case COUNT_GREEN:
      if (elapsed >= WINDOW_MS) {
        greenHz = (edgeCount * 1000UL) / elapsed;

        bool isRed = redHz >= RED_MIN_HZ && redHz >= RED_RATIO * greenHz;
        redStreak = isRed ? redStreak + 1 : 0;
        redDetected = redStreak >= RED_CONFIRM;
        readingCount++;

        Serial.printf("R: %5lu Hz  G: %5lu Hz  ratio: %.2f  %s\n",
                      redHz, greenHz,
                      greenHz ? (float)redHz / greenHz : 0.0f,
                      redDetected ? "RED -> STOP" : "");

        selectFilter(LOW, LOW);                   // back to red filter
        phaseStart = now; phase = SETTLE_RED;
      }
      break;
  }
}

void setMotor(int en, int in1, int in2, int speed) {
  digitalWrite(in1, speed >= 0 ? HIGH : LOW);
  digitalWrite(in2, speed >= 0 ? LOW : HIGH);
  ledcWrite(en, constrain(abs(speed), 0, 255));
}

void driveSide(bool isLeft, int speed) {
  if (isLeft) {
    setMotor(ENA_F, IN1_F, IN2_F, speed);   // front-left
    setMotor(ENB_R, IN3_R, IN4_R, -speed);  // rear-left (inverted)
  } else {
    setMotor(ENB_F, IN3_F, IN4_F, speed);   // front-right
    setMotor(ENA_R, IN1_R, IN2_R, -speed);  // rear-right (inverted)
  }
}

void stopAll() {
  driveSide(true, 0);
  driveSide(false, 0);
}

void setup() {
  Serial.begin(115200);

  pinMode(leftIR, INPUT);
  pinMode(rightIR, INPUT);

  int outs[] = {IN1_F, IN2_F, IN3_F, IN4_F, IN1_R, IN2_R, IN3_R, IN4_R};
  for (int p : outs) pinMode(p, OUTPUT);

  ledcAttach(ENA_F, 1000, 8);
  ledcAttach(ENB_F, 1000, 8);
  ledcAttach(ENA_R, 1000, 8);
  ledcAttach(ENB_R, 1000, 8);

  pinMode(S1_PIN, OUTPUT);
  digitalWrite(S1_PIN, LOW);
  pinMode(S2_PIN, OUTPUT);
  pinMode(S3_PIN, OUTPUT);
  pinMode(OUT_PIN, INPUT);
  selectFilter(LOW, LOW);                         // start on red filter
  attachInterrupt(digitalPinToInterrupt(OUT_PIN), onEdge, RISING);
  phaseStart = millis();

  stopAll();
}

void loop() {
  updateColor();

  switch (roverState) {
    case RUNNING:
      if (redDetected) {
        stopAll();
        pauseStart = millis();
        roverState = PAUSED;
        Serial.println(">> Red detected: stopping for 3 s");
        return;
      }
      break;

    case PAUSED:
      stopAll();
      if (millis() - pauseStart < STOP_MS) return;
      clearStreak = 0;
      lastSeenReading = readingCount;
      roverState = LEAVING_RED;           // drive off the patch, ignoring red
      Serial.println(">> Resuming");
      break;

    case LEAVING_RED:
      // Re-arm only after a few fresh readings that are NOT red,
      // so the same patch doesn't trigger another stop.
      if (readingCount != lastSeenReading) {
        lastSeenReading = readingCount;
        clearStreak = redDetected ? 0 : clearStreak + 1;
        if (clearStreak >= CLEAR_CONFIRM) roverState = RUNNING;
      }
      break;
  }

  int left  = digitalRead(leftIR);
  int right = digitalRead(rightIR);

  if (left == 0 && right == 0) {
    // no line under either — straight
    driveSide(true, SPEED);
    driveSide(false, SPEED);
  }
  else if (left == 0 && right == 1) {
    // right IR on line — left side fast, right side slow
    driveSide(true, FAST_SPEED);
    driveSide(false, SLOW_SPEED);
  }
  else if (left == 1 && right == 0) {
    // left IR on line — right side fast, left side slow
    driveSide(true, SLOW_SPEED);
    driveSide(false, FAST_SPEED);
  }
  else {
    // both on line, or lifted — stop
    stopAll();
  }
}
