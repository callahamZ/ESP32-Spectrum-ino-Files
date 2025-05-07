#include <Adafruit_AS7341.h>

#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"

#include "DHT.h"

Adafruit_AS7341 as7341;

#define ledBlue 13
#define buzzer 14
#define photoDiode 34
#define captureButton 15

#define DHTPIN 33
#define DHTTYPE DHT22

// #define WIFI_SSID "TKL"
// #define WIFI_PASS "oraganti"
// #define API "AIzaSyADIx4Bxv0SO8nUIAzv-1n53oYEFi1h14I"
// #define DATABASE_URL "https://esp32-light-spectrum-analyzer-default-rtdb.asia-southeast1.firebasedatabase.app/"

// FirebaseAuth auth;
// FirebaseData fbdo;
// FirebaseConfig config;

DHT dht(DHTPIN, DHTTYPE);

// bool signUpOK = false;

//====================================================================

// bool uploadToFirebase(FirebaseData &fbdo, const char *path, float value) {
//   if (Firebase.RTDB.setFloat(&fbdo, path, value)) {
//     Serial.print(value);
//     Serial.print(" - Saved to ");
//     Serial.println(path);
//     return true;
//   } else {
//     Serial.print("GAGAL : ");
//     Serial.println(fbdo.errorReason());
//     return false;
//   }
// }

//=================================================================

void setup() {
  Serial.begin(115200);

  pinMode(ledBlue, OUTPUT);
  pinMode(buzzer, OUTPUT);
  pinMode(photoDiode, INPUT);
  pinMode(captureButton, INPUT_PULLUP); // Use INPUT_PULLUP for easier button press detection

  while (!Serial) {
    delay(1);
  }

  if (!as7341.begin()){
    Serial.println("Could not find AS7341");
    while (1) { delay(10); }
  }

  as7341.setATIME(100);
  as7341.setASTEP(999);
  as7341.setGain(AS7341_GAIN_256X);

  // WiFi.begin(WIFI_SSID, WIFI_PASS);
  // Serial.print("Menghubungkan ke Wifi");
  // while(WiFi.status() != WL_CONNECTED) {
  //   Serial.print(".");
  //   delay(300);
  // }
  // Serial.println();
  // Serial.print("Terhubung dengan IP : ");
  // Serial.println(WiFi.localIP());
  // Serial.println();

  // String ssid = String(WiFi.SSID());
  // String ip = WiFi.localIP().toString();

  // config.api_key = API;
  // config.database_url = DATABASE_URL;
  // if(Firebase.signUp(&config, &auth, "", "")) {
  //   Serial.println("Sign Up OK");
  //   signUpOK = true;
  // } else {
  //   Serial.printf("%s\n", config.signer.signupError.message.c_str());
  // }

  // config.token_status_callback = tokenStatusCallback;
  // Firebase.begin(&config, &auth);
  // Firebase.reconnectWiFi(true);

  // if (Firebase.ready() && signUpOK) {
  //   uploadToFirebase(fbdo, "Informasi/SSID", ssid.toFloat());
  //   uploadToFirebase(fbdo, "Informasi/IP", ip.toFloat());
  // }

  dht.begin();

  digitalWrite(ledBlue, HIGH);
  delay(200);
  digitalWrite(ledBlue, LOW);
  delay(200);
  digitalWrite(ledBlue, HIGH);

  tone(buzzer, 1100, 200);
  tone(buzzer, 1500, 200);
}

// =======================================================

void loop() {
  uint16_t readings[12];
  static bool buttonPressed = false; // Debouncing flag

  int buttonState = digitalRead(captureButton);

  // Check if the button is pressed (LOW with INPUT_PULLUP) and was not pressed before
  if (buttonState == LOW && !buttonPressed) {
    buttonPressed = true; // Set the flag to indicate button is pressed
    Serial.println("");
    delay(400); // Debounce delay

    int lux = analogRead(photoDiode);

    as7341.setLEDCurrent(20); // dalam ukuran mA
    as7341.enableLED(true);

    tone(buzzer, 1800, 100);

    if (!as7341.readAllChannels(readings)){
      Serial.println("Error reading all channels!");
      return;
    }

    String serialData = "@DataCap";
    for (int i = 0; i < 12; i++) {
      if (i == 4 || i == 5) {
        continue; // Skip the duplicate clear/NIR readings for serial output
      }
      serialData += ",";
      if (i < 5) {
        serialData += String(readings[i]);
      } else {
        serialData += String(readings[i]);
      }
      if (i == 9) break; // Stop after the first 8 F channels
    }
    serialData += ",";
    serialData += String(lux);

    float suhu = dht.readTemperature();
    if (isnan(suhu)) {
      suhu = 0;
      // Serial.println(F("Failed to read from DHT sensor!"));
      // return;
    }
    serialData += ",";
    serialData += String(suhu);

    Serial.println(serialData); // Print the combined data

    as7341.enableLED(false);
  } else if (buttonState == HIGH && buttonPressed) {
    buttonPressed = false; // Reset the flag when the button is released
  }
  // No delay here to keep checking the button state
}