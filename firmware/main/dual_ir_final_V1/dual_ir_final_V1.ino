// ---- Motor pins ----
const int ENA_F = 7,  IN1_F = 8,  IN2_F = 9;   // front-left
const int ENB_F = 10, IN3_F = 11, IN4_F = 12;  // front-right
const int ENA_R = 13, IN1_R = 14, IN2_R = 15;  // physically REAR-RIGHT
const int ENB_R = 16, IN3_R = 17, IN4_R = 18;  // physically REAR-LEFT

// ---- IR sensor pins ----
const int leftIR = 2;
const int rightIR = 1;
const int SPEED = 100;
const int FAST_SPEED = 130;
const int SLOW_SPEED = 0;

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
}

void loop() {
  int left = digitalRead(leftIR);
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
    driveSide(true, 0);
    driveSide(false, 0);
  }

  delay(20);
}