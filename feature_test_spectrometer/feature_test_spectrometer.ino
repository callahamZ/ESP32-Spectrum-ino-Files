#define ledBlue 13
#define buzzer 14

void setup() {
  Serial.begin(115200);
  pinMode(ledBlue, OUTPUT);
  pinMode(buzzer, OUTPUT);

  digitalWrite(ledBlue, HIGH);
  delay(200);
  digitalWrite(ledBlue, LOW);
  delay(200);
  digitalWrite(ledBlue, HIGH);

  tone(buzzer, 1100, 200);
  tone(buzzer, 1500, 200);
  delay(500);
}

void loop() {
  //Serial.println("test");
  //delay(500);
}
