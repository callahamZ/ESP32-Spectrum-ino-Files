#include <Adafruit_AS7341.h>
#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"
#include "DHT.h"
#include <EEPROM.h>

Adafruit_AS7341 as7341;

#define ledBlue 13
#define buzzer 14
#define photoDiode 33
#define captureButton 15
#define DHTPIN 32
#define DHTTYPE DHT22

// EEPROM addresses for storing Wi-Fi credentials
#define EEPROM_SSID_ADDR 0
#define EEPROM_PASS_ADDR 32 // Leave enough space for SSID (max 32 bytes)

#define MAX_SSID_LENGTH 32
#define MAX_PASS_LENGTH 32

#define WIFI_SSID "TKL"    // default wifi credential if EEPROM is empty
#define WIFI_PASS "oraganti"

#define API "AIzaSyADIx4Bxv0SO8nUIAzv-1n53oYEFi1h14I"
#define DATABASE_URL "https://esp32-light-spectrum-analyzer-default-rtdb.asia-southeast1.firebasedatabase.app/"

FirebaseAuth auth;
FirebaseData fbdo;
FirebaseConfig config;

DHT dht(DHTPIN, DHTTYPE);

bool signUpOK = false;
char wifi_ssid[MAX_SSID_LENGTH] = "";
char wifi_pass[MAX_PASS_LENGTH] = "";

//====================================================================
// Overloaded function for float values
bool uploadToFirebase(FirebaseData &fbdo, const char *path, float value)
{
  if (Firebase.RTDB.setFloat(&fbdo, path, value))
  {
    Serial.print(value);
    Serial.print(" - Saved to ");
    Serial.println(path);
    return true;
  }
  else
  {
    Serial.print("GAGAL : ");
    Serial.println(fbdo.errorReason());
    return false;
  }
}

// NEW: Overloaded function for string values
bool uploadToFirebase(FirebaseData &fbdo, const char *path, const String &value)
{
  if (Firebase.RTDB.setString(&fbdo, path, value))
  {
    Serial.print(value);
    Serial.print(" - Saved to ");
    Serial.println(path);
    return true;
  }
  else
  {
    Serial.print("GAGAL : ");
    Serial.println(fbdo.errorReason());
    return false;
  }
}

//=================================================================
void saveWiFiCredentials(const char *ssid, const char *pass)
{
  EEPROM.writeString(EEPROM_SSID_ADDR, ssid);
  EEPROM.writeString(EEPROM_PASS_ADDR, pass);
  EEPROM.commit();
  Serial.println("Wi-Fi credentials saved to EEPROM.");
}

void loadWiFiCredentials()
{
  EEPROM.readString(EEPROM_SSID_ADDR, wifi_ssid, MAX_SSID_LENGTH);
  EEPROM.readString(EEPROM_PASS_ADDR, wifi_pass, MAX_PASS_LENGTH);
  if (wifi_ssid[0] != 0 && wifi_pass[0] != 0)
  {
    Serial.println("Wi-Fi credentials loaded from EEPROM.");
  }
  else
  {
    Serial.println("Wi-Fi credentials not found in EEPROM.  Will use hardcoded values.");
    // Consider adding a routine to get the credentials, if not found.
    strcpy(wifi_ssid, WIFI_SSID); //load default
    strcpy(wifi_pass, WIFI_PASS); //load default
  }
}

void processSerialData() {
  if (Serial.available() > 0) {
    String data = Serial.readStringUntil('\n');
    data.trim(); // Remove leading/trailing whitespace

    if (data.startsWith("@Setting,wifiSet,")) {
      // Parse the data using commas as delimiters
      int comma1 = data.indexOf(',', 17); // Find first comma after "@Setting,wifiSet,"
      int comma2 = data.indexOf(',', comma1 + 1);

      if (comma1 > 0 && comma2 > comma1) {
        String ssid = data.substring(comma1 + 1, comma2);
        String pass = data.substring(comma2 + 1);

        //basic check
        if(ssid.length() > MAX_SSID_LENGTH){
          ssid = ssid.substring(0, MAX_SSID_LENGTH -1); // -1 for null terminator
        }
        if(pass.length() > MAX_PASS_LENGTH){
          pass = pass.substring(0, MAX_PASS_LENGTH -1); // -1 for null terminator
        }

        ssid.toCharArray(wifi_ssid, MAX_SSID_LENGTH);
        pass.toCharArray(wifi_pass, MAX_PASS_LENGTH);
        saveWiFiCredentials(wifi_ssid, wifi_pass);

        Serial.println("SSID: " + ssid);
        Serial.println("Password: " + pass);
        Serial.println("Wi-Fi credentials updated and saved.");
        WiFi.disconnect();
        WiFi.begin(wifi_ssid, wifi_pass); // Attempt to connect with new credentials
      } else {
        Serial.println("Invalid serial data format.  Use @Setting,wifiSet,ssid,password");
      }
    }
  }
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

  loadWiFiCredentials(); // Load Wi-Fi credentials from EEPROM

  WiFi.begin(wifi_ssid, wifi_pass); // Use loaded credentials
  Serial.print("Menghubungkan ke Wifi");
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(300);
  }
  Serial.println();
  Serial.print("Terhubung dengan IP : ");
  Serial.println(WiFi.localIP());
  Serial.println();

  String ssid = String(WiFi.SSID());
  String ip = WiFi.localIP().toString();

  config.api_key = API;
  config.database_url = DATABASE_URL;
  if (Firebase.signUp(&config, &auth, "", ""))
  {
    Serial.println("Sign Up OK");
    signUpOK = true;
  }
  else
  {
    Serial.printf("%s\n", config.signer.signupError.message.c_str());
  }

  config.token_status_callback = tokenStatusCallback;
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  if (Firebase.ready() && signUpOK)
  {
    // *** CHANGE THESE LINES ***
    uploadToFirebase(fbdo, "Informasi/SSID", ssid); // Upload as String
    uploadToFirebase(fbdo, "Informasi/IP", ip);     // Upload as String
  }

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

    String serialData = "@DataCap";
    for (int i = 0; i < 12; i++)
    {
      if (i == 4 || i == 5)
      {
        continue; // Skip the duplicate clear/NIR readings for serial output
      }
      serialData += ",";
      char path[20];
      uint16_t valueToUpload = readings[i];
      if (i < 5)
      {
        serialData += String(readings[i]);
        sprintf(path, "sensorSpektrum/F%d", i + 1);
        uploadToFirebase(fbdo, path, readings[i]);
      }
      else
      {
        serialData += String(readings[i]);
        sprintf(path, "sensorSpektrum/F%d", i - 1);
        uploadToFirebase(fbdo, path, readings[i]);
      }
    }

    serialData += ",";
    serialData += String(readings[10]);
    uploadToFirebase(fbdo, "sensorSpektrum/Clear", readings[10]);
    serialData += ","; // Add comma before appending the next value
    serialData += String(readings[11]);
    uploadToFirebase(fbdo, "sensorSpektrum/NIR", readings[11]);

    serialData += ",";
    serialData += String(lux);
    uploadToFirebase(fbdo, "sensorCahaya/Lux", lux);

    float suhu = dht.readTemperature();
    if (isnan(suhu))
    {
      suhu = 0;
    }
    uploadToFirebase(fbdo, "sensorSuhu/Suhu", suhu);
    serialData += ",";
    serialData += String(suhu);

    Serial.println(serialData);

    as7341.enableLED(false);

    Firebase.RTDB.setBool(&fbdo, "dataFinish", true);
    delay(200);
    Serial.println("Data Finish Flag set");
    Firebase.RTDB.setBool(&fbdo, "dataFinish", false);
  }
  else if (buttonState == HIGH && buttonPressed)
  {
    buttonPressed = false; // Reset the flag when the button is released
  }
  processSerialData(); // Check for serial commands
}