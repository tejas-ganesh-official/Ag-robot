const int ENA_F = 7,  IN1_F = 8,  IN2_F = 9;   // front-left
const int ENB_F = 10, IN3_F = 11, IN4_F = 12;  // front-right
const int ENA_R = 13, IN1_R = 14, IN2_R = 15;  // rear-left
const int ENB_R = 16, IN3_R = 17, IN4_R = 18;  // rear-right

void setMotor(int en, int in1, int in2, int speed) {
  digitalWrite(in1, speed >= 0 ? HIGH : LOW);
  digitalWrite(in2, speed >= 0 ? LOW : HIGH);
  ledcWrite(en, constrain(abs(speed), 0, 255));
}

void setup() {
  int outs[] = {IN1_F, IN2_F, IN3_F, IN4_F, IN1_R, IN2_R, IN3_R, IN4_R};
  for (int p : outs) pinMode(p, OUTPUT);

  ledcAttach(ENA_F, 1000, 8);
  ledcAttach(ENB_F, 1000, 8);
  ledcAttach(ENA_R, 1000, 8);
  ledcAttach(ENB_R, 1000, 8);

  setMotor(ENA_F, IN1_F, IN2_F, 120);
  setMotor(ENB_F, IN3_F, IN4_F, 120);
  setMotor(ENA_R, IN1_R, IN2_R, 120);
  setMotor(ENB_R, IN3_R, IN4_R, 120);

  delay(5000);

  setMotor(ENA_F, IN1_F, IN2_F, 0);
  setMotor(ENB_F, IN3_F, IN4_F, 0);
  setMotor(ENA_R, IN1_R, IN2_R, 0);
  setMotor(ENB_R, IN3_R, IN4_R, 0);
}

void loop() {
  // nothing — runs once
}