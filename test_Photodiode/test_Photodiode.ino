void setup() {
  pinMode(34, INPUT);
  Serial.begin(115200);
}

void loop() {
  Serial.println(analogRead(34));
  delay(400);
}
