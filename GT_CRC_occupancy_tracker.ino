/*******************************************************************
    Reads live crowd data from waitz.io/live/gatech and displays 
    busyness stat of Recreation Center serially on an LCD screen 
    using an ESP32-C5.

    Updates every 15 seconds.

    Adapted from TeamTreesArduino by Brian Lough:
    https://github.com/witnessmenow/TeamTreesArduino/blob/e1242e3a9af3e29f30de396ccc58deeafccda129/PrintToSerial/PrintToSerial.ino

 *******************************************************************/

#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include "LCDi2c.h"

#define SDA 2
#define SCL 3

LCDi2c lcd;

char ssid[] = "GTother";       // your network SSID (name)
char password[] = "GeorgeP@1927";  // your network key

const char* serverUrl = "https://waitz.io/live/gatech";

unsigned long lastTime = 0;
unsigned long timerDelay = 15000;  // every 15 seconds

void setup() {

  Serial.begin(115200);
  Wire.begin(SDA, SCL, 1000000);
  lcd.begin();
  lcd.display(DISPLAY_ON);
  lcd.display(BACKLIGHT_ON);
  lcd.locate(1, 1);
  lcd.print("CRC Occupancy:");
  lcd.locate(2, 13);

  // Setup ESP32 as WiFi station
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);

  // Attempt to connect to Wifi network:
  Serial.print("Connecting Wifi: ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(250);
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  IPAddress ip = WiFi.localIP();
  Serial.println(ip);
  //Serial.println(WiFi.macAddress());
}

void loop() {
  if ((millis() - lastTime) > timerDelay) {
    if (WiFi.status() == WL_CONNECTED) {
      
      WiFiClientSecure client;
      client.setInsecure(); 
      
      HTTPClient http;
      http.setTimeout(15000);
      
      if (http.begin(client, serverUrl)) {
        int httpCode = http.GET();
        
        if (httpCode == 200) {
          // Get the payload as a String first
          String payload = http.getString();
          
          Serial.printf("Response size: %d bytes\n", payload.length());
          
          DynamicJsonDocument doc(98304);
          
          DeserializationError err = deserializeJson(doc, payload);
          
          if (!err) {
            JsonArray locations = doc["data"].as<JsonArray>();
            bool found = false;
            
            for (JsonObject loc : locations) {
              const char* name = loc["name"];
              if (name) {
                String locName = String(name);
                if (locName.indexOf("Recreation") != -1) {
                  int busyness = loc["busyness"];
                  lcd.clp(2, 13, 4);
                  delay(500);
                  lcd.printf("%d%%", busyness);
                  found = true;
                  break;
                }
              }
            }
            if(!found) {
              Serial.println("Fitness Center match not found in data.");
              Serial.printf("Total locations found: %d\n", locations.size());
            }
          } else {
            Serial.print("JSON Parse Error: ");
            Serial.println(err.c_str());
            Serial.println("First 500 chars of response:");
            Serial.println(payload.substring(0, 500));
          }
        } else {
          Serial.printf("HTTP GET failed, error: %s (Code: %d)\n", http.errorToString(httpCode).c_str(), httpCode);
        }
        http.end();
      }
    } else {
      Serial.println("WiFi Disconnected");
    }
    lastTime = millis();
  }
}