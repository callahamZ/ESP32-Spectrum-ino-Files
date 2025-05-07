void setup() {
  Serial.begin(115200);
}

void loop() {
  int F1 = random(0, 1000);
  int F2 = random(0, 1000);
  int F3 = random(0, 1000);
  int F4 = random(0, 1000);
  int F5 = random(0, 1000);
  int F6 = random(0, 1000);
  int F7 = random(0, 1000);
  int F8 = random(0, 1000);

  int lux = random(0, 100);
  int suhu = random (25, 35);

  String outputString = "@DataCap," + String(F1) + "," + String(F2) + "," + String(F3) + "," + String(F4) + "," + String(F5) + "," + String(F6) + "," + String(F7) + "," + String(F8) + "," + String(lux) + "," + String(suhu);

  Serial.println(outputString);
  delay(3000);
}
