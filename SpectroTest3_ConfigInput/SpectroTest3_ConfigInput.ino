#include <Adafruit_AS7341.h>
#include "DHT.h"
#include <EEPROM.h>
#include "BluetoothSerial.h"

#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif

// Corrected EEPROM Addresses and magic number for initialization
// Addresses are spaced to avoid overlapping memory writes/reads.
#define EEPROM_LED_STATE_ADDR 64    // Address to store LED state (int, 4 bytes)
#define EEPROM_LED_CURRENT_ADDR 68  // Address to store LED current (int, 4 bytes)
#define EEPROM_MAGIC_ADDR 72        // Address for our EEPROM initialization flag (long, 4 bytes)
#define EEPROM_MAGIC_NUMBER 12345678 // A unique number to check for initialization

// Global variables for LED settings
int savedLedState = 0;
int savedLedCurrent = 20;

Adafruit_AS7341 as7341;

#define ledBlue 13
#define buzzer 14
#define photoDiode 33
#define captureButton 15
#define DHTPIN 32
#define DHTTYPE DHT22

DHT dht(DHTPIN, DHTTYPE);

BluetoothSerial SerialBT;

void setup() {
  delay(200);
  Serial.begin(115200);
  pinMode(ledBlue, OUTPUT);
  pinMode(buzzer, OUTPUT);
  pinMode(photoDiode, INPUT);
  pinMode(captureButton, INPUT_PULLUP);

  while (!Serial) {
    delay(1);
  }

  // Initialize EEPROM with a sufficient size
  if (!EEPROM.begin(512)) {
    Serial.println("Failed to initialize EEPROM!");
    while (1) {
      delay(10);
    }
  }

  // Initialize AS7341 sensor first
  if (!as7341.begin()) {
    Serial.println("Could not find AS7341");
    while (1) {
      delay(10);
    }
  }

  // Set AS7341 parameters
  as7341.setATIME(100);
  as7341.setASTEP(999);
  as7341.setGain(AS7341_GAIN_256X);

  // Now check EEPROM and apply settings
  long eepromMagicNumber = 0;
  EEPROM.get(EEPROM_MAGIC_ADDR, eepromMagicNumber);

  if (eepromMagicNumber != EEPROM_MAGIC_NUMBER) {
    // This is the first run or EEPROM was wiped.
    Serial.println("EEPROM not initialized. Setting default values.");
    
    // Set default values
    savedLedState = 0;
    savedLedCurrent = 20;

    // Save the default values and the magic number
    EEPROM.put(EEPROM_LED_STATE_ADDR, savedLedState);
    EEPROM.put(EEPROM_LED_CURRENT_ADDR, savedLedCurrent);
    EEPROM.put(EEPROM_MAGIC_ADDR, EEPROM_MAGIC_NUMBER);
    EEPROM.commit();
  } else {
    // EEPROM has been initialized, load saved settings
    Serial.println("EEPROM already initialized. Loading saved settings.");
    EEPROM.get(EEPROM_LED_STATE_ADDR, savedLedState);
    EEPROM.get(EEPROM_LED_CURRENT_ADDR, savedLedCurrent);
    
    // Validate the loaded values in case of corruption
    if (savedLedState != 0 && savedLedState != 1) {
      savedLedState = 0; // Default to off
    }
    if (savedLedCurrent < 20 || savedLedCurrent > 100) {
      savedLedCurrent = 20; // Default to minimum valid current
    }
  }

  // Apply the loaded/default settings to the now-initialized AS7341
  //as7341.enableLED(savedLedState);
  as7341.setLEDCurrent(savedLedCurrent);
  
  Serial.print("Loaded LED State: ");
  Serial.println(savedLedState ? "ON" : "OFF");
  Serial.print("Loaded LED Current: ");
  Serial.println(savedLedCurrent);

  // Initialize DHT sensor
  dht.begin();

  // Initialize Bluetooth Serial
  SerialBT.begin("MySpectrum ESP32 AS7341");
  Serial.println("Bluetooth Serial started");

  // Initial feedback with LED and buzzer
  digitalWrite(ledBlue, HIGH);
  delay(200);
  digitalWrite(ledBlue, LOW);
  delay(200);
  digitalWrite(ledBlue, HIGH);

  tone(buzzer, 1100, 200);
  tone(buzzer, 1500, 200);
}

void loop() {
  // Check for incoming USB serial data to set LED parameters
  if (Serial.available()) {
    String incomingData = Serial.readStringUntil('\n');
    incomingData.trim(); // Remove any leading/trailing whitespace

    // Check if the command starts with the correct header
    if (incomingData.startsWith("@SetLED,")) {
      // Find the position of the two commas to extract the values
      int firstComma = incomingData.indexOf(',');
      int secondComma = incomingData.indexOf(',', firstComma + 1);

      if (firstComma != -1 && secondComma != -1) {
        // Extract and parse the state and current values
        String stateStr = incomingData.substring(firstComma + 1, secondComma);
        String currentStr = incomingData.substring(secondComma + 1);

        int newState = stateStr.toInt();
        int newCurrent = currentStr.toInt();

        // Validate the new values
        if ((newState == 0 || newState == 1) && (newCurrent >= 20 && newCurrent <= 100)) {
          // Update the global variables
          savedLedState = newState;
          savedLedCurrent = newCurrent;

          // Apply the new settings to the AS7341
          // NOTE: We don't enable the LED here, it's a "setting" that will be used during capture.
          as7341.setLEDCurrent(savedLedCurrent);

          // Save the new values to EEPROM
          EEPROM.put(EEPROM_LED_STATE_ADDR, savedLedState);
          EEPROM.put(EEPROM_LED_CURRENT_ADDR, savedLedCurrent);
          EEPROM.commit();

          // Send a confirmation message
          String confirmationMsg = "@LED_Settings,";
          confirmationMsg += savedLedState;
          confirmationMsg += ",";
          confirmationMsg += savedLedCurrent;

          Serial.println(confirmationMsg);
          SerialBT.println(confirmationMsg);

          tone(buzzer, 1100, 200);
          tone(buzzer, 1500, 200);
          tone(buzzer, 1800, 200);
          
        } else {
          // Respond with an error if values are invalid
          Serial.println("@Error,Invalid LED parameters");
          SerialBT.println("@Error,Invalid LED parameters");
        }
      }
    }
  }

  // Existing button capture logic
  uint16_t readings[12];
  bool buttonPressed = false; // Debouncing flag

  int buttonState = digitalRead(captureButton);

  if (buttonState == LOW && !buttonPressed) {
    buttonPressed = true;
    Serial.println("Button Pressed");
    delay(400);

    int lux = analogRead(photoDiode);

    // Use the saved LED settings for the capture
    as7341.setLEDCurrent(savedLedCurrent);

    // Only enable the LED for the brief moment of measurement if the saved state is ON
    if (savedLedState == 1) {
      as7341.enableLED(true);
    }

    tone(buzzer, 1800, 100);

    if (!as7341.readAllChannels(readings)) {
      Serial.println("Error reading all channels!");
      SerialBT.println("Error reading all channels!");
      // Don't return, still want to turn off LED if it's on
    }
    
    // Always turn the LED off after the reading, regardless of the saved state
    // This restores the "flash" behavior for the measurement
    as7341.enableLED(false);

    String serialData = "@DataCap";
    for (int i = 0; i < 10; i++) {
      if (i == 4 || i == 5) {
        continue;
      }
      serialData += ",";
      serialData += String(readings[i]);
    }

    serialData += ",";
    serialData += String(readings[10]);
    serialData += ",";
    serialData += String(readings[11]);

    serialData += ",";
    serialData += String(lux);

    float suhu = dht.readTemperature();
    if (isnan(suhu)) {
      suhu = 0;
    }
    serialData += ",";
    serialData += String(suhu);

    Serial.println(serialData);
    if (SerialBT.connected()) {
      SerialBT.println(serialData);
    }
  } else if (buttonState == HIGH && buttonPressed) {
    buttonPressed = false;
  }
}
