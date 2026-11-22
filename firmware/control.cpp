#include "control.h"
#include "config.h"

unsigned long lastPumpToggle = 0;

void initControl() {
  pinMode(PIN_POMP, OUTPUT);
  pinMode(PIN_LED_PWM, OUTPUT);
  digitalWrite(PIN_POMP, LOW);
  digitalWrite(PIN_LED_PWM, LOW);
  lastPumpToggle = millis();
}

void handleAutomations() {
  // ====================================================
  // 1. ALARMEN & FAILSAFE (Absolute prioriteit)
  // ====================================================
  if (waterLevelPct < threshCfg.min_water_level_pct) {
    alarmState = "CRITICAL_LOW_WATER";
    digitalWrite(PIN_POMP, LOW);
    pumpState = false;
    manualPumpOverride = false;
    return; 
  } else if (waterTemp > threshCfg.max_water_temp_c || roomTemp > threshCfg.max_room_temp_c) {
    alarmState = "OVERHEATING_ALARM";
  } else {
    alarmState = "NONE";
  }

  // ====================================================
  // 2. HANDMATIGE OVERSTURING POMP (Manual Override)
  // ====================================================
  if (manualPumpOverride) {
    if (millis() > manualPumpTimeout) {
      manualPumpOverride = false;
      Serial.println("[Control] Handmatige pomp override verlopen. Terug naar schema.");
      lastPumpToggle = millis(); 
    } else {
      digitalWrite(PIN_POMP, pumpState ? HIGH : LOW);
    }
  } 
  // AUTOMATISCHE IRRIGATIE CYCLUS
  else {
    unsigned long totalCycleTimeMs = (irrCfg.on_duration_min + irrCfg.interval_min) * 60000;
    unsigned long timeInCycle = (millis() - lastPumpToggle) % totalCycleTimeMs;

    if (timeInCycle < ((unsigned long)irrCfg.on_duration_min * 60000)) {
      if (!pumpState) {
        digitalWrite(PIN_POMP, HIGH);
        pumpState = true;
        Serial.println("[Control] Automatische cyclus: POMP AAN");
      }
    } else {
      if (pumpState) {
        digitalWrite(PIN_POMP, LOW);
        pumpState = false;
        Serial.println("[Control] Automatische cyclus: POMP UIT");
      }
    }
  }

  // ====================================================
  // 3. HANDMATIGE OVERSTURING LED / SLIDER
  // ====================================================
  if (manualLedOverride) {
    if (millis() > manualLedTimeout) {
      manualLedOverride = false;
      Serial.println("[Control] Handmatige LED oversturing verlopen. Terug naar tijdschema.");
    } else {
      analogWrite(PIN_LED_PWM, currentLedBrightness);
      ledState = (currentLedBrightness > 0) ? "ON" : "OFF";
      return; 
    }
  }

  // ====================================================
  // 4. AUTOMATISCHE VERLICHTING CYCLUS (Hogere Resolutie in Seconden)
  // ====================================================
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    currentLedBrightness = 255;
    ledState = "ON";
    analogWrite(PIN_LED_PWM, currentLedBrightness);
    return; 
  }

  // Zet huidige tijd om naar absolute seconden sinds middernacht
  long currentSecsSinceMidnight = (timeinfo.tm_hour * 3600) + (timeinfo.tm_min * 60) + timeinfo.tm_sec;

  // Ontleed start- en eindtijden
  int startHour = 6, startMin = 0, endHour = 22, endMin = 0;
  sscanf(lightCfg.start_time.c_str(), "%d:%d", &startHour, &startMin);
  sscanf(lightCfg.end_time.c_str(), "%d:%d", &endHour, &endMin);
  
  long startSecs = (startHour * 3600) + (startMin * 60);
  long endSecs = (endHour * 3600) + (endMin * 60);
  long fadeSecs = lightCfg.fade_duration_min * 60; // Bereken fade window in seconden

  // Controleer of we binnen het actieve tijdsbestek vallen
  if (currentSecsSinceMidnight >= startSecs && currentSecsSinceMidnight < endSecs) {
    long elapsedSecs = currentSecsSinceMidnight - startSecs;
    long remainingSecs = endSecs - currentSecsSinceMidnight;

    // A. SUNRISE FADE-IN WINDOW
    if (elapsedSecs < fadeSecs && fadeSecs > 0) {
      currentLedBrightness = map(elapsedSecs, 0, fadeSecs, 0, 255);
    } 
    // B. SUNSET FADE-OUT WINDOW
    else if (remainingSecs < fadeSecs && fadeSecs > 0) {
      currentLedBrightness = map(remainingSecs, 0, fadeSecs, 0, 255);
    } 
    // C. VOLLEDIG AAN OVERDAG
    else {
      currentLedBrightness = 255;
    }
    ledState = "ON";
  } 
  // ABSOLUUT BUITEN DE TIJDSRANDEN: HARD SIGNAAL UIT!
  else {
    currentLedBrightness = 0;
    ledState = "OFF";
  }
  
  // Stuur de gecorrigeerde PWM-waarde fysiek naar de MOSFET pin
  analogWrite(PIN_LED_PWM, currentLedBrightness);
}
