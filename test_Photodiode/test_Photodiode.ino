void setup() {
  pinMode(33, INPUT);
  Serial.begin(115200);
}

void loop() {
  Serial.println(analogRead(33));
  delay(700);
}
