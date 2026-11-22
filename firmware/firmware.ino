#include <Arduino.h>
#include <WiFi.h>
#include <time.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include <WiFiManager.h>
#include <ArduinoJson.h>
#include <Update.h>
#include <PubSubClient.h>

// Include our custom modules
#include "config.h"
#include "sensors.h"
#include "control.h"
#include "web_server.h"

WiFiClient espClient;
PubSubClient mqttClient(espClient);
AsyncWebServer server(80);

unsigned long lastSensorUpdate = 0;
unsigned long lastMqttPublish = 0;

void saveConfigCallback() { shouldSaveConfig = true; }

void connectMQTT() {
  if (!mqttClient.connected()) {
    Serial.print("[MQTT] Connecting to protected Mosquitto broker... ");
    
    // Check if credentials exist, otherwise connect anonymously
    bool success = false;
    if (mqttCfg.user.length() > 0) {
      // Connect using: ClientID, Username, Password
      success = mqttClient.connect("ESP32C6_HydroTower", mqttCfg.user.c_str(), mqttCfg.password.c_str());
    } else {
      success = mqttClient.connect("ESP32C6_HydroTower");
    }

    if (success) {
      Serial.println("Authenticated & Connected!");
    } else {
      Serial.printf("Auth Failed, rc=%d (Check /etc/mosquitto/passwd)\n", mqttClient.state());
    }
  }
}


void publishTelemetry() {
  if (!mqttClient.connected()) return;

  JsonDocument telemetryDoc;
  telemetryDoc["room_temp"] = roomTemp;
  telemetryDoc["room_humidity"] = roomHumidity;
  telemetryDoc["water_temp"] = waterTemp;
  telemetryDoc["water_level"] = waterLevelPct;
  telemetryDoc["pump_status"] = pumpState ? "ON" : "OFF";
  telemetryDoc["light_brightness"] = currentLedBrightness;
  telemetryDoc["alarm"] = alarmState;

  String payload;
  serializeJson(telemetryDoc, payload);
  
  // Dynamic string generation fix
  String topic = mqttCfg.prefix + "/telemetry";
  mqttClient.publish(topic.c_str(), payload.c_str());
}
void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n--- ESP32 Dev Module Hydro Tower Booting ---");

  if (!LittleFS.begin(true)) {
    Serial.println("CRITICAL ERROR: LittleFS Mount Failed!");
    return;
  }

  loadConfiguration();

  // Initialize hardware and low-level interfaces first
  initSensors();
  initControl();

  // EXPLICIT ESP32 DUAL-CORE FIX: Force mode and allow radio hardware to settle safely
  WiFi.mode(WIFI_STA); 
  delay(500);

  WiFiManager wm;
  wm.setSaveConfigCallback(saveConfigCallback);
  
  // Give WiFiManager explicit, safe connection and portal windows
  wm.setConnectTimeout(30);       // Gives Core 0 plenty of time to handshake with the router
  wm.setConfigPortalTimeout(180); // Portal stays open for 3 minutes before failsafe timeout

  Serial.println("[WiFi] Attempting automated connection...");
  
  // Start the background connection engine
  if (!wm.autoConnect("ESP32-Hydro-Setup")) {
    Serial.println("[WiFi] Connection timeout or portal closed. Rebooting...");
    delay(3000);
    ESP.restart();
  }

  // Save the Wi-Fi credentials back to your template JSON if modified via portal
  if (shouldSaveConfig) {
    saveFullConfiguration();
  }

  // Bring up network communication modules once you have a stable IP lease
  mqttClient.setServer(mqttCfg.broker.c_str(), mqttCfg.port);
  initWebServer();

  Serial.println("[NTP] Synchronising with network time server...");
  configTime(ntpCfg.gmt_offset, ntpCfg.daylight_offset, ntpCfg.server.c_str());
  
  Serial.println("[SYSTEM] Boot loop complete. Running automation loops.");
}



void loop() {
  if (WiFi.status() == WL_CONNECTED) {
    if (!mqttClient.connected()) {
      connectMQTT();
    }
    mqttClient.loop();
  }

  // Independent scheduler using clean clock intervals
  if (millis() - lastSensorUpdate >= 5000) {
    lastSensorUpdate = millis();
    readSensors();
    handleAutomations();
  }

  if (millis() - lastMqttPublish >= ((unsigned long)mqttCfg.interval_sec * 1000)) {
    lastMqttPublish = millis();
    publishTelemetry();
  }
}
