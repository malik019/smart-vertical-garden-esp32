#include "config.h"
#include <WiFi.h>      
#include <LittleFS.h>
#include <ArduinoJson.h>

#define FileSystem LittleFS
const char* configFilePath = "/config.json";

// Allocate memory space for override flags
bool manualPumpOverride = false;
unsigned long manualPumpTimeout = 0;
bool manualLedOverride = false;
unsigned long manualLedTimeout = 0; 

// Allocate memory for structural configurations
WifiConfig wifiCfg;
NtpConfig ntpCfg;
LightingConfig lightCfg;
IrrigationConfig irrCfg;
ThresholdsConfig threshCfg;
MqttConfig mqttCfg;

bool shouldSaveConfig = false;
String ledState = "OFF";
float roomTemp = 0.0, roomHumidity = 0.0, waterTemp = 0.0, waterLevelPct = 100.0;
bool pumpState = false;
int currentLedBrightness = 0;
String alarmState = "NONE";

void loadConfiguration() {
  if (FileSystem.exists(configFilePath)) {
    File configFile = FileSystem.open(configFilePath, "r");
    if (configFile) {
      JsonDocument doc;
      DeserializationError error = deserializeJson(doc, configFile);
      if (!error) {
        wifiCfg.ssid = doc["wifi"]["ssid"] | "";
        wifiCfg.password = doc["wifi"]["password"] | "";
        
        ntpCfg.server = doc["ntp"]["server"] | "pool.ntp.org";
        ntpCfg.gmt_offset = doc["ntp"]["gmt_offset"] | 3600;
        ntpCfg.daylight_offset = doc["ntp"]["daylight_offset"] | 3600;
        
        lightCfg.start_time = doc["lighting"]["start_time"] | "06:00";
        lightCfg.end_time = doc["lighting"]["end_time"] | "22:00";
        lightCfg.fade_duration_min = doc["lighting"]["fade_duration_min"] | 15;
        
        irrCfg.on_duration_min = doc["irrigation"]["on_duration_min"] | 5;
        irrCfg.interval_min = doc["irrigation"]["interval_min"] | 45;
        
        threshCfg.min_water_level_pct = doc["thresholds"]["min_water_level_pct"] | 20.0;
        threshCfg.max_water_temp_c = doc["thresholds"]["max_water_temp_c"] | 26.0;
        threshCfg.max_room_temp_c = doc["thresholds"]["max_room_temp_c"] | 35.0;
        
        // FIXED: Replaced corrupted "://hivemq.com" prefix string fallback reference link
        mqttCfg.broker = doc["mqtt"]["broker"] | "broker.hivemq.com";
        mqttCfg.port = doc["mqtt"]["port"] | 1883;
        mqttCfg.user = doc["mqtt"]["user"] | "";              
        mqttCfg.password = doc["mqtt"]["password"] | "";      
        mqttCfg.prefix = doc["mqtt"]["prefix"] | "hydro_tower/001";
        mqttCfg.interval_sec = doc["mqtt"]["interval_sec"] | 30;
        
        Serial.println("[JSON] Configuration successfully loaded.");
      }
      configFile.close();
    }
  }
}

void saveFullConfiguration() {
  JsonDocument doc;
  if (WiFi.status() == WL_CONNECTED) {
    doc["wifi"]["ssid"] = WiFi.SSID();
    doc["wifi"]["password"] = WiFi.psk();
  } else {
    doc["wifi"]["ssid"] = wifiCfg.ssid;
    doc["wifi"]["password"] = wifiCfg.password;
  }
  
  doc["ntp"]["server"] = ntpCfg.server;
  doc["ntp"]["gmt_offset"] = ntpCfg.gmt_offset;
  doc["ntp"]["daylight_offset"] = ntpCfg.daylight_offset;
  doc["lighting"]["start_time"] = lightCfg.start_time;
  doc["lighting"]["end_time"] = lightCfg.end_time;
  doc["lighting"]["fade_duration_min"] = lightCfg.fade_duration_min;
  doc["irrigation"]["on_duration_min"] = irrCfg.on_duration_min;
  doc["irrigation"]["interval_min"] = irrCfg.interval_min;
  doc["thresholds"]["min_water_level_pct"] = threshCfg.min_water_level_pct;
  doc["thresholds"]["max_water_temp_c"] = threshCfg.max_water_temp_c;
  doc["thresholds"]["max_room_temp_c"] = threshCfg.max_room_temp_c;
  doc["mqtt"]["broker"] = mqttCfg.broker;
  doc["mqtt"]["port"] = mqttCfg.port;
  doc["mqtt"]["user"] = mqttCfg.user;
  doc["mqtt"]["password"] = mqttCfg.password;
  doc["mqtt"]["prefix"] = mqttCfg.prefix;
  doc["mqtt"]["interval_sec"] = mqttCfg.interval_sec;

  File configFile = FileSystem.open(configFilePath, "w");
  if (configFile) {
    serializeJson(doc, configFile);
    configFile.close();
    Serial.println("[JSON] Configuration saved to LittleFS.");
  }
}

String getFormattedTime() {
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){ return "NTP synchronizing..."; }
  char timeStringBuff[30]; 
  strftime(timeStringBuff, sizeof(timeStringBuff), "%H:%M:%S", &timeinfo);
  return String(timeStringBuff);
}
