#pragma once
#include <Arduino.h>

struct WifiConfig {
  String ssid = "";
  String password = "";
};

struct NtpConfig {
  String server = "pool.ntp.org";
  long gmt_offset = 3600;
  int daylight_offset = 3600;
};

struct LightingConfig {
  String start_time = "06:00";
  String end_time = "22:00";
  int fade_duration_min = 15;
};

struct IrrigationConfig {
  int on_duration_min = 5;
  int interval_min = 45;
};

struct ThresholdsConfig {
  float min_water_level_pct = 20.0;
  float max_water_temp_c = 26.0;
  float max_room_temp_c = 35.0;
};

struct MqttConfig {
  String broker = "://hivemq.com"; 
  int port = 1883;
  String user = "cobalt";          
  String password = "testmqtt";  
  String prefix = "hydro_tower/001";
  int interval_sec = 30;
};

// Global handles accessible by all modules
extern WifiConfig wifiCfg;
extern NtpConfig ntpCfg;
extern LightingConfig lightCfg;
extern IrrigationConfig irrCfg;
extern ThresholdsConfig threshCfg;
extern MqttConfig mqttCfg;

extern bool shouldSaveConfig;
extern String ledState;
extern float roomTemp, roomHumidity, waterTemp, waterLevelPct;
extern bool pumpState;
extern int currentLedBrightness;
extern String alarmState;

// Dynamic runtime system-override handles
extern bool manualPumpOverride;
extern unsigned long manualPumpTimeout;
extern bool manualLedOverride;
extern unsigned long manualLedTimeout;

// ESP32-C6-DevKitC-1 Pin Definitions (Safe non-strapping, non-USB GPIOs)
#define PIN_POMP            2   // Pump AO3400A MOSFET Gate (EN_MOTOR)
#define PIN_LED_PWM         3   // LED Grow Light PWM AO3400A Gate (EN_LED)

// ESP32-C6 Native I2C Pins (BMP280)
#define PIN_I2C_SDA         4   // ESP32-C6 Native SDA
#define PIN_I2C_SCL         5   // ESP32-C6 Native SCL

#define PIN_WATERTEMP       10  // 1-Wire Digital Pin (DS18B20)
#define PIN_LEVEL_TRIG      6   // HC-SR04 Ultrasonic Trigger
#define PIN_LEVEL_ECHO      7   // HC-SR04 Ultrasonic Echo (via 5.6k/1.3k divider)

// Function prototypes
void loadConfiguration();
void saveFullConfiguration();
String getFormattedTime();
