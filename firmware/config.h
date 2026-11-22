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
  String broker = "broker.hivemq.com"; 
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

// CLEANED: Single definition block for all dynamic runtime system-override handles
extern bool manualPumpOverride;
extern unsigned long manualPumpTimeout;
extern bool manualLedOverride;
extern unsigned long manualLedTimeout;

// Pin definitions
#define PIN_POMP            18  // Safe Output Jumper
#define PIN_LED_PWM         12  // Safe PWM Dimmer Jumper (Moved off 19)

// Native ESP32 Hardware I2C Pins (Bypasses chip memory bus conflicts)
#define PIN_I2C_SDA         21  // ESP32 Native SDA
#define PIN_I2C_SCL         22  // ESP32 Native SCL

#define PIN_WATERTEMP       14  // Safe digital pin (for when your DS18B20 arrives)
#define PIN_LEVEL_TRIG      15  // Safe Output for Ultrasonic Trigger
#define PIN_LEVEL_ECHO      13

// Function prototypes
void loadConfiguration();
void saveFullConfiguration();
String getFormattedTime();
