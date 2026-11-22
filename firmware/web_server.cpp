#include "web_server.h"
#include "config.h"
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <Update.h>

extern AsyncWebServer server;

void initWebServer() {
  // 1. FIXED: Manual Root Dashboard Router (Bypasses broken serveStatic/Stream)
  server.on("/", WebRequestMethod::HTTP_GET, [](AsyncWebServerRequest *request){
    if (LittleFS.exists("/index.html")) {
      File file = LittleFS.open("/index.html", "r");
      String html = file.readString();
      file.close();
      request->send(200, "text/html", html);
    } else {
      request->send(404, "text/plain", "index.html missing from LittleFS partition!");
    }
  });

  // 2. FIXED: Manual CSS Resource Router
  server.on("/css/style.css", WebRequestMethod::HTTP_GET, [](AsyncWebServerRequest *request){
    if (LittleFS.exists("/css/style.css")) {
      File file = LittleFS.open("/css/style.css", "r");
      String css = file.readString();
      file.close();
      request->send(200, "text/css", css);
    } else {
      request->send(404, "text/plain", "style.css missing");
    }
  });

  // 3. FIXED: Manual JavaScript Logic Router
  server.on("/js/main.js", WebRequestMethod::HTTP_GET, [](AsyncWebServerRequest *request){
    if (LittleFS.exists("/js/main.js")) {
      File file = LittleFS.open("/js/main.js", "r");
      String js = file.readString();
      file.close();
      request->send(200, "application/javascript", js);
    } else {
      request->send(404, "text/plain", "main.js missing");
    }
  });

  // 4. Live Data JSON Telemetry Stream Endpoint
  server.on("/api/live", WebRequestMethod::HTTP_GET, [](AsyncWebServerRequest *request){
    AsyncResponseStream *response = request->beginResponseStream("application/json");
    JsonDocument doc;
    doc["time"] = getFormattedTime();
    doc["room_temp"] = roomTemp;
    doc["room_humidity"] = roomHumidity;
    doc["water_temp"] = waterTemp;
    doc["water_level"] = waterLevelPct;
    doc["pump"] = pumpState ? "ON" : "OFF";
    doc["light_brightness"] = (int)currentLedBrightness; 
    doc["alarm"] = alarmState;
    
    serializeJson(doc, *response);
    request->send(response);
  });

  // 5. Overrides Endpoint for Pump and LED Dimmer
  server.on("/api/override", WebRequestMethod::HTTP_POST, [](AsyncWebServerRequest *request){}, NULL,
    [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total){
      JsonDocument doc;
      DeserializationError error = deserializeJson(doc, data, len);
      
      if (!error) {
        int timeoutSec = doc["timeout"] | 300; 
        
        if(doc.containsKey("pump")) {
          String state = doc["pump"];
          manualPumpOverride = true;
          manualPumpTimeout = millis() + ((unsigned long)timeoutSec * 1000);
          pumpState = (state == "ON");
          digitalWrite(PIN_POMP, pumpState ? HIGH : LOW);
          Serial.printf("[Override] Pump forced to %s\n", state.c_str());
        }
        
        if(doc.containsKey("brightness")) {
          int targetBrightness = doc["brightness"];
          manualLedOverride = true;
          manualLedTimeout = millis() + ((unsigned long)timeoutSec * 1000);
          currentLedBrightness = constrain(targetBrightness, 0, 255);
          analogWrite(PIN_LED_PWM, currentLedBrightness);
          ledState = (currentLedBrightness > 0) ? "ON" : "OFF";
          Serial.printf("[Override] LED Dimmer locked at: %d/255\n", currentLedBrightness);
        }
        
        AsyncResponseStream *response = request->beginResponseStream("application/json");
        response->print("{\"ok\":1}");
        request->send(response);
      } else {
        request->send(400, "application/json", "{\"error\":\"bad_json\"}");
      }
  });

  // 6. Save Configuration Profiles Endpoint
  server.on("/api/saveconfig", WebRequestMethod::HTTP_POST, [](AsyncWebServerRequest *request){}, NULL,
    [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total){
      JsonDocument doc;
      deserializeJson(doc, data, len);
      
      if(doc.containsKey("light_start")) lightCfg.start_time = doc["light_start"].as<String>();
      if(doc.containsKey("light_end")) lightCfg.end_time = doc["light_end"].as<String>();
      if(doc.containsKey("pump_on")) irrCfg.on_duration_min = doc["pump_on"];
      if(doc.containsKey("pump_interval")) irrCfg.interval_min = doc["pump_interval"];
      if(doc.containsKey("min_water")) threshCfg.min_water_level_pct = doc["min_water"];
      
      saveFullConfiguration();
      request->send(200, "application/json", "{\"ok\":1}");
  });

  // 7. Config Inspection Endpoint
  server.on("/config", WebRequestMethod::HTTP_GET, [](AsyncWebServerRequest *request){
    if (LittleFS.exists("/config.json")) {
      File file = LittleFS.open("/config.json", "r");
      String configData = file.readString();
      file.close();
      request->send(200, "application/json", configData);
    } else {
      request->send(404, "application/json", "{\"error\":\"config_missing\"}");
    }
  });


// Serve the OTA Update Page
  server.on("/update", WebRequestMethod::HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(LittleFS, "/update.html", "text/html");
  });

  // Handle Firmware (.bin) and Filesystem (littlefs.bin) Uploads
  server.on("/do-update", WebRequestMethod::HTTP_POST, [](AsyncWebServerRequest *request){
    bool error = Update.hasError();
    AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", error ? "FAIL" : "OK");
    response->addHeader("Connection", "close");
    request->send(response);
    delay(1000);
    if (!error) ESP.restart();
  }, [](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final){
    if (!index) {
      Serial.printf("[OTA] Starting update: %s\n", filename.c_str());
      int cmd = (filename.indexOf("littlefs") >= 0 || filename.indexOf("spiffs") >= 0) ? U_SPIFFS : U_FLASH;
      if (!Update.begin(UPDATE_SIZE_UNKNOWN, cmd)) {
        Update.printError(Serial);
      }
    }
    if (!Update.hasError()) {
      if (Update.write(data, len) != len) {
        Update.printError(Serial);
      }
    }
    if (final) {
      if (Update.end(true)) {
        Serial.printf("[OTA] Success! %u bytes written. Rebooting...\n", index + len);
      } else {
        Update.printError(Serial);
      }
    }
  });

    // Manual OTA CSS Resource Router
  server.on("/css/ota.css", WebRequestMethod::HTTP_GET, [](AsyncWebServerRequest *request){
    if (LittleFS.exists("/css/ota.css")) {
      File file = LittleFS.open("/css/ota.css", "r");
      String css = file.readString();
      file.close();
      request->send(200, "text/css", css);
    } else {
      request->send(404, "text/plain", "ota.css missing");
    }
  });

  // Manual OTA JavaScript Logic Router
  server.on("/js/ota.js", WebRequestMethod::HTTP_GET, [](AsyncWebServerRequest *request){
    if (LittleFS.exists("/js/ota.js")) {
      File file = LittleFS.open("/js/ota.js", "r");
      String js = file.readString();
      file.close();
      request->send(200, "application/javascript", js);
    } else {
      request->send(404, "text/plain", "ota.js missing");
    }
  });

    // Helper function to recursively list all files in LittleFS (including /css and /js subfolders)
  auto listDirToJson = [](JsonArray &arr, const char* dirname, auto& self) -> void {
    File root = LittleFS.open(dirname);
    if (!root || !root.isDirectory()) return;
    File file = root.openNextFile();
    while (file) {
      String path = String(file.path());
      if (!path.startsWith("/")) path = "/" + path;
      if (file.isDirectory()) {
        self(arr, path.c_str(), self);
      } else {
        JsonObject obj = arr.add<JsonObject>();
        obj["name"] = path;
        obj["size"] = file.size();
      }
      file = root.openNextFile();
    }
  };

  // 1. List all files stored in LittleFS
  server.on("/api/files", WebRequestMethod::HTTP_GET, [listDirToJson](AsyncWebServerRequest *request){
    JsonDocument doc;
    JsonArray arr = doc.to<JsonArray>();
    listDirToJson(arr, "/", listDirToJson);
    String output;
    serializeJson(doc, output);
    request->send(200, "application/json", output);
  });

  // 2. Upload single files directly to LittleFS (automatically places .css in /css/ and .js in /js/)
  server.on("/api/file-upload", WebRequestMethod::HTTP_POST, [](AsyncWebServerRequest *request){
    request->send(200, "text/plain", "OK");
  }, [](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final){
    static File uploadFile;
    if (!index) {
      String path = filename;
      if (!path.startsWith("/")) {
        if (path.endsWith(".css")) path = "/css/" + path;
        else if (path.endsWith(".js")) path = "/js/" + path;
        else path = "/" + path;
      }
      Serial.printf("[FS] Uploading file: %s\n", path.c_str());
      uploadFile = LittleFS.open(path, "w");
    }
    if (uploadFile) {
      uploadFile.write(data, len);
    }
    if (final && uploadFile) {
      uploadFile.close();
      Serial.printf("[FS] Upload complete: %s (%u bytes)\n", filename.c_str(), index + len);
    }
  });

  // 3. Download a file from LittleFS
  server.on("/api/file-download", WebRequestMethod::HTTP_GET, [](AsyncWebServerRequest *request){
    if (request->hasParam("file")) {
      String path = request->getParam("file")->value();
      if (!path.startsWith("/")) path = "/" + path;
      if (LittleFS.exists(path)) {
        request->send(LittleFS, path, "application/octet-stream", true);
        return;
      }
    }
    request->send(404, "text/plain", "File not found");
  });

  // 4. Delete a file from LittleFS
  server.on("/api/file-delete", WebRequestMethod::HTTP_DELETE, [](AsyncWebServerRequest *request){
    if (request->hasParam("file")) {
      String path = request->getParam("file")->value();
      if (!path.startsWith("/")) path = "/" + path;
      if (LittleFS.exists(path)) {
        LittleFS.remove(path);
        Serial.printf("[FS] Deleted: %s\n", path.c_str());
        request->send(200, "text/plain", "Deleted");
        return;
      }
    }
    request->send(404, "text/plain", "File not found");
  });

  // 5. Bekijk een bestand direct in de browser (bv. /api/file-view?file=/css/style.css)
  server.on("/api/file-view", AsyncWebRequestMethod::HTTP_GET, [](AsyncWebServerRequest *request) {
    if (!request->hasParam("file")) {
      request->send(400, "text/plain", "Parameter 'file' ontbreekt");
      return;
    }
    String path = request->getParam("file")->value();
    if (!path.startsWith("/")) path = "/" + path;

    if (!LittleFS.exists(path)) {
      request->send(404, "text/plain", "Bestand niet gevonden");
      return;
    }

    // Bepaal het juiste content-type zodat de browser het bestand direct toont
    String contentType = "text/plain";
    if (path.endsWith(".html")) contentType = "text/html";
    else if (path.endsWith(".css")) contentType = "text/css";
    else if (path.endsWith(".js")) contentType = "application/javascript";
    else if (path.endsWith(".json")) contentType = "application/json";

    request->send(LittleFS, path, contentType, false); // false = open in browser i.p.v. downloaden
  });



  // Start HTTP engine
  server.begin();
  Serial.println("[HTTP] Web Server active with explicit custom file stream injection rules.");
}
