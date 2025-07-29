#include <Adafruit_AS7341.h>
#include "DHT.h"
#include <EEPROM.h>

Adafruit_AS7341 as7341;

#define ledBlue 13
#define buzzer 14
#define photoDiode 33
#define captureButton 15
#define DHTPIN 32
#define DHTTYPE DHT22

// EEPROM addresses for storing Wi-Fi credentials (kept for EEPROM init, but not used for WiFi)
#define EEPROM_SSID_ADDR 0
#define EEPROM_PASS_ADDR 32 // Leave enough space for SSID (max 32 bytes)

#define MAX_SSID_LENGTH 32
#define MAX_PASS_LENGTH 32

DHT dht(DHTPIN, DHTTYPE);

// No longer storing or using WiFi credentials
// char wifi_ssid[MAX_SSID_LENGTH] = "";
// char wifi_pass[MAX_PASS_LENGTH] = "";
// bool wifiConnected = false; // Flag no longer needed

//=================================================================
// These functions are no longer strictly needed for WiFi, but EEPROM is still initialized.
// Keeping them for now in case EEPROM is used for other purposes later,
// but they won't be called for WiFi credentials.
void saveWiFiCredentials(const char *ssid, const char *pass)
{
  EEPROM.writeString(EEPROM_SSID_ADDR, ssid);
  EEPROM.writeString(EEPROM_PASS_ADDR, pass);
  EEPROM.commit();
  Serial.println("EEPROM credentials saved (not used for WiFi).");
}

void loadWiFiCredentials()
{
  // No longer loading WiFi credentials
  // EEPROM.readString(EEPROM_SSID_ADDR, wifi_ssid, MAX_SSID_LENGTH);
  // EEPROM.readString(EEPROM_PASS_ADDR, wifi_pass, MAX_PASS_LENGTH);
  Serial.println("EEPROM initialized, but WiFi credentials are not loaded or used.");
}

void setup() {
  delay(200);
  Serial.begin(115200);
  pinMode(ledBlue, OUTPUT);
  pinMode(buzzer, OUTPUT);
  pinMode(photoDiode, INPUT);
  pinMode(captureButton, INPUT);

  while (!Serial)
  {
    delay(1);
  }

  if (!as7341.begin())
  {
    Serial.println("Could not find AS7341");
    while (1)
    {
      delay(10);
    }
  }

  as7341.setATIME(100);
  as7341.setASTEP(999);
  as7341.setGain(AS7341_GAIN_256X);

  EEPROM.begin(512); // Initialize EEPROM with a size of 512 bytes
  // loadWiFiCredentials(); // No longer needed for WiFi

  // All WiFi connection logic removed
  Serial.println("WiFi functionality removed. Data will only be sent via Serial.");

  dht.begin();

  // Simplified initial feedback since no WiFi connection status to report
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
  bool buttonPressed = false; // Debouncing flag

  int buttonState = digitalRead(captureButton);

  if (buttonState == LOW && !buttonPressed)
  {
    buttonPressed = true; // Set the flag to indicate button is pressed
    Serial.println("Button Pressed");
    delay(400); // Debounce delay

    int lux = analogRead(photoDiode);

    as7341.setLEDCurrent(20); // dalam ukuran mA
    as7341.enableLED(true);

    tone(buzzer, 1800, 100);

    if (!as7341.readAllChannels(readings))
    {
      Serial.println("Error reading all channels!");
      return;
    }
    as7341.enableLED(false);

    String serialData = "@DataCap";
    for (int i = 0; i < 10; i++)
    {
      if (i == 4 || i == 5)
      {
        continue; // Skip the duplicate clear/NIR readings for serial output
      }
      serialData += ",";
      if (i < 5)
      {
        serialData += String(readings[i]);
      }
      else
      {
        serialData += String(readings[i]);
      }
    }

    serialData += ",";
    serialData += String(readings[10]); // Clear channel
    serialData += ",";
    serialData += String(readings[11]); // NIR channel

    serialData += ",";
    serialData += String(lux); // Lux from photoDiode

    float suhu = dht.readTemperature();
    if (isnan(suhu))
    {
      suhu = 0;
    }
    serialData += ",";
    serialData += String(suhu); // Temperature from DHT22

    Serial.println(serialData);
  }
  else if (buttonState == HIGH && buttonPressed)
  {
    buttonPressed = false; // Reset the flag when the button is released
  }
}
