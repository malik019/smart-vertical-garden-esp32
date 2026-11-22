#include "sensors.h"
#include "config.h"
#include <Wire.h>
#include <Adafruit_BMP280.h>

Adafruit_BMP280 bmp; 
bool bmpPresent = false;

void initSensors() {
  // 1. Initialiseer de HC-SR04 Ultrasoon pinnen
  pinMode(PIN_LEVEL_TRIG, OUTPUT);
  pinMode(PIN_LEVEL_ECHO, INPUT);
  digitalWrite(PIN_LEVEL_TRIG, LOW);
  Serial.println("[Sensors] HC-SR04 Ultrasoonsensor geïnitialiseerd.");

  // Watertemperatuur simulatie melding
  Serial.println("[Sensors] Watertemperatuursensor afwezig. Simulatie actief.");

  // 2. Start de hardware I2C-bus op de native WROOM32 pinnen (21 en 22)
  Serial.printf("[Sensors] I2C Bus starten op Native SDA:%d en SCL:%d...\n", PIN_I2C_SDA, PIN_I2C_SCL);
  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL, 100000); // 100kHz I2C clock speed stability

  // 3. FIXED: Removed the invalid Wire pointer argument to match your library signature
  if (!bmp.begin(0x76)) { 
    Serial.println("[Sensors] FOUT: Kon BMP280 sensor niet vinden op adres 0x76! Proberen op 0x77...");
    if (!bmp.begin(0x77)) {
      Serial.println("[Sensors] CRITISCHE FOUT: Geen BMP280 sensor gedetecteerd op de I2C bus!");
      bmpPresent = false;
    } else {
      Serial.println("[Sensors] SUCCES: BMP280 omgevingssensor gevonden op alternatief adres 0x77.");
      bmpPresent = true;
    }
  } else {
    Serial.println("[Sensors] SUCCES: BMP280 omgevingssensor geverifieerd op 0x76.");
    bmpPresent = true;
  }

  if (bmpPresent) {
    bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,     
                    Adafruit_BMP280::SAMPLING_X2,     
                    Adafruit_BMP280::SAMPLING_X16,    
                    Adafruit_BMP280::FILTER_X16,      
                    Adafruit_BMP280::STANDBY_MS_500); 
  }
}

void readSensors() {
  // ====================================================
  // 1. ECHTE METING: HC-SR04 Ultrasone Waterhoogte
  // ====================================================
  digitalWrite(PIN_LEVEL_TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(PIN_LEVEL_TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(PIN_LEVEL_TRIG, LOW);

  long duration = pulseIn(PIN_LEVEL_ECHO, HIGH, 30000); 
  float distanceCm = duration * 0.0343 / 2.0;
  if (duration == 0) {
    waterLevelPct = 0.0; 
    Serial.println("[Sensors] HC-SR04 Waarschuwing: Geen echo ontvangen.");
  } else {
    // ====================================================
    // UPDATED TANK CALIBRATION (Adjusted for WROOM32 physical space)
    // 70 cm distance = Empty Tank (0%)
    // 10 cm distance = Completely Full Tank (100%)
    // ====================================================
    float calcPct = map(distanceCm, 70, 10, 0, 100);
    waterLevelPct = constrain(calcPct, 0.0, 100.0); 
    
    Serial.printf("[Sensors] HC-SR04 -> Afstand: %.1f cm | Niveau: %.1f%%\n", distanceCm, waterLevelPct);
  }


  // ====================================================
  // 2. ECHTE METING: BMP280 Omgevingstemperatuur
  // ====================================================
  if (bmpPresent) {
    roomTemp = bmp.readTemperature();
    roomHumidity = 52.0; // BMP280 has no humidity sensor, safe placeholder
  } else {
    roomTemp = 22.0; 
    roomHumidity = 50.0;
  }

  // ====================================================
  // 3. SIMULATIE: Watertemperatuur (placeholder)
  // ====================================================
  static float simulatedWaterTemp = 21.0;
  simulatedWaterTemp += (random(-2, 2) / 10.0); 
  waterTemp = simulatedWaterTemp;
}
