#include <Arduino.h>
#include <DHTesp.h>
#include <BH1750.h>
#include <Firebase_ESP_Client.h>
#include "devices.h"

#define FIREBASE_HOST "https://ce-binus-iot-course-default-rtdb.asia-southeast1.firebasedatabase.app/"
#define FIREBASE_AUTH "Du6OLGVeZMdhMce3NFlnh4V7JYFTXM6S1el4U5pH"
#define WIFI_SSID "Steff-IoT"
#define WIFI_PASSWORD "steffiot123"

DHTesp dht;
BH1750 lightMeter;

FirebaseData fbdo;
FirebaseConfig fbConfig;
FirebaseData fbdoStream;

void WifiConnect()
{
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }  
  Serial.print("System connected with IP address: ");
  Serial.println(WiFi.localIP());
  Serial.printf("RSSI: %d\n", WiFi.RSSI());
}

void onFirebaseRecv(FirebaseStream data)
{
  //onFirebaseStream: /cmd /ledGreen int 0
  Serial.printf("onFirebaseStream: %s %s %s %s\n", data.streamPath().c_str(),
                data.dataPath().c_str(), data.dataType().c_str(),
                data.stringData().c_str());
  if (data.dataType() == "int")
  {
    int value = data.intData();
    if (data.dataPath() == "/ledGreen")
      digitalWrite(LED_GREEN, value);
    else if (data.dataPath() == "/ledYellow")
      digitalWrite(LED_YELLOW, value);
    else if (data.dataPath() == "/ledRed")
      digitalWrite(LED_RED, value);
  }
}

void Firebase_Init(const String& streamPath)
{
  FirebaseAuth fbAuth;
  fbConfig.host = FIREBASE_HOST;
  fbConfig.signer.tokens.legacy_token = FIREBASE_AUTH;
  Firebase.begin(&fbConfig, &fbAuth);
  Firebase.reconnectWiFi(true);

  fbdo.setResponseSize(2048);
  Firebase.RTDB.setwriteSizeLimit(&fbdo, "medium");
  while (!Firebase.ready())
  {
    Serial.println("Connecting to firebase...");
    delay(1000);
  }
  String path = streamPath;
  if (Firebase.RTDB.beginStream(&fbdoStream, path.c_str()))
  {
    Serial.println("Firebase stream on "+ path);
    Firebase.RTDB.setStreamCallback(&fbdoStream, onFirebaseRecv, 0);
  }
  else
    Serial.println("Firebase stream failed: "+fbdoStream.errorReason());
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_YELLOW, OUTPUT);
  dht.setup(DHT_PIN, DHTesp::DHT11);
  pinMode(PUSH_BUTTON, INPUT_PULLUP);
  Wire.begin();
  lightMeter.begin(); 
  WifiConnect();
  Serial.println("Connecting to Firebase...");
  Firebase_Init("Binus_Syahdan/L1A/cmd");
  Serial.println("System ready.");
}

unsigned long nTimerHelpButton = 0;
unsigned long nTimerSendData = 0;
void loop() {
  unsigned long nNow;

  nNow = millis();
  if ((nNow - nTimerHelpButton)>5000) // tiap 5 detik lakukan:
  {
    nTimerHelpButton = nNow;
    if (digitalRead(PUSH_BUTTON)==LOW)
    {
      Serial.println("Button pressed!");
      Firebase.RTDB.setInt(&fbdo,  "Binus_Syahdan/L1A/data/current/helpButton", 1);
    }
    else
    {
      Firebase.RTDB.setInt(&fbdo,  "Binus_Syahdan/L1A/data/current/helpButton", 0);
    }
  }

  nNow = millis();
  if (nNow - nTimerSendData>10000) // tiap 10 detik lakukan:
  {
    digitalWrite(LED_BUILTIN, HIGH);
    nTimerSendData = nNow;
    Serial.println("Send Data to FIrebase");

    float fHumidity = dht.getHumidity();
    float fTemperature = dht.getTemperature();
    float lux = lightMeter.readLightLevel();
    Serial.printf("Humidity: %.2f, Temperature: %.2f, Light: %.2f \n",
        fHumidity, fTemperature, lux);
    Firebase.RTDB.setInt(&fbdo,   "Binus_Syahdan/L1A/data/current/milis", millis());
    Firebase.RTDB.setFloat(&fbdo, "Binus_Syahdan/L1A/data/current/temperature", fTemperature);
    Firebase.RTDB.setFloat(&fbdo, "Binus_Syahdan/L1A/data/current/humidity", fHumidity);
    Firebase.RTDB.setFloat(&fbdo, "Binus_Syahdan/L1A/data/current/lux", lux);        

    Firebase.RTDB.pushFloat(&fbdo, "Binus_Syahdan/L1A/data/history/temperature", fTemperature);
    Firebase.RTDB.pushFloat(&fbdo, "Binus_Syahdan/L1A/data/history/humidity", fHumidity);
    Firebase.RTDB.pushFloat(&fbdo, "Binus_Syahdan/L1A/data/history/lux", lux);
    digitalWrite(LED_BUILTIN, LOW);
  }
}
