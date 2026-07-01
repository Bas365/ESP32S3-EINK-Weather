/* Configuration values for TRMNL OG Weather Station
 * Based on esp32-weather-epd by Luke Marzen
 * Configured for TRMNL OG hardware
 *
 * IMPORTANT: Copy include/secrets.example.h to include/secrets.h
 *            and fill in your personal values before compiling!
 */

#include "config.h"
#include "secrets.h" // Contains all sensitive configuration

// =============================================================================
// TRMNL OG (Seeed "7.5-inch OG DIY Kit") PIN DEFINITIONS
// Hardware is a Seeed XIAO ESP32-S3 Plus module on Seeed's "XIAO ePaper
// Monitor Kit" driver board - NOT an ESP32-C3, and NOT the same pinout as
// the original square TRMNL device. Verified against both TRMNL's own
// firmware (DEV_Config.h, BOARD_XIAO_EPAPER_DISPLAY) and the kit's KiCad
// schematic (XIAO ePaper Monitor Kit v1.0).
// =============================================================================
const uint8_t PIN_BAT_ADC = 1;     // Battery ADC (GPIO1, net BAT_ADC)
const uint8_t PIN_EPD_BUSY = 4;    // E-Paper BUSY (GPIO4)
const uint8_t PIN_EPD_CS = 44;     // E-Paper CS (GPIO44, net EDP_CS)
const uint8_t PIN_EPD_RST = 38;    // E-Paper RST (GPIO38, net EDP_RES)
const uint8_t PIN_EPD_DC = 10;     // E-Paper DC (GPIO10, net EDP_DC)
const uint8_t PIN_EPD_SCK = 7;     // E-Paper SCK (GPIO7)
const uint8_t PIN_EPD_MISO = -1;   // Not used (display is write-only)
const uint8_t PIN_EPD_MOSI = 9;    // E-Paper MOSI (GPIO9)
const uint8_t PIN_EPD_PWR = 43;    // E-paper +3.3V rail enable (GPIO43,
                                    // net PWR_EN) - the panel's power rail
                                    // is switched and MUST be driven HIGH
                                    // before SPI init or the panel gets no
                                    // power at all.
const uint8_t PIN_VBAT_SWITCH = 6; // Battery ADC divider load-switch
                                    // enable (GPIO6, net ADC_EN), active
                                    // HIGH. Must be enabled briefly before
                                    // sampling PIN_BAT_ADC.

// BME sensor pins - not used, we use Home Assistant
const uint8_t PIN_BME_SDA = -1;
const uint8_t PIN_BME_SCL = -1;
const uint8_t PIN_BME_PWR = -1;
const uint8_t BME_ADDRESS = 0x76;

// =============================================================================
// WIFI CONFIGURATION
// Using WiFiManager for captive portal setup - these are fallback/defaults
// =============================================================================
const char *WIFI_SSID = SECRET_WIFI_SSID;
const char *WIFI_PASSWORD = SECRET_WIFI_PASSWORD;
const unsigned long WIFI_TIMEOUT = 15000; // 15 seconds

// =============================================================================
// HTTP CLIENT
// =============================================================================
const unsigned HTTP_CLIENT_TCP_TIMEOUT = 15000; // 15 seconds

// =============================================================================
// THE WEATHER COMPANY (TWC) API
// Current conditions: PWS Observations v2 (stationId-based)
// Forecast: TWC Daily Forecast v3 (geocode-based, 5-day)
// =============================================================================
const String WU_APIKEY     = SECRET_WU_APIKEY;
const String WU_STATION_ID = SECRET_WU_STATION_ID;
const String WU_LANGUAGE   = "en-GB";

// =============================================================================
// LOCATION
// =============================================================================
const String LAT = SECRET_LAT;
const String LON = SECRET_LON;
const String CITY_STRING = SECRET_CITY_STRING;

// =============================================================================
// TIMEZONE AND TIME FORMATS 
// =============================================================================
const char *TIMEZONE = "NZST-12NZDT,M9.5.0,M4.1.0/3";
const char *TIME_FORMAT = "%H:%M";               // 24-hour: 14:30
const char *HOUR_FORMAT = "%H";                  // 24-hour: 14
const char *DATE_FORMAT = "%a %d %B %Y";           // Sat, 28 Nov
const char *REFRESH_TIME_FORMAT = "%H:%M"; // 28/11 14:30

// =============================================================================
// NTP SERVERS
// =============================================================================
const char *NTP_SERVER_1 = "pool.ntp.org";
const char *NTP_SERVER_2 = "time.nist.gov";
const unsigned long NTP_TIMEOUT = 20000; // 20 seconds

// =============================================================================
// SLEEP SETTINGS
// =============================================================================
const int SLEEP_DURATION = 60;   // Minutes between updates
const int BED_TIME = 23;         // Hour to start extended sleep (11 PM)
const int WAKE_TIME = 6;         // Hour to resume normal updates (6 AM)
const int HOURLY_GRAPH_MAX = 8; // Hours to show in outlook graph

// =============================================================================
// BATTERY THRESHOLDS (millivolts)
// Tuned for TRMNL OG's LiPo battery
// =============================================================================
const uint32_t MAX_BATTERY_VOLTAGE = 4200;           // 100%
const uint32_t MIN_BATTERY_VOLTAGE = 3000;           // 0%
const uint32_t WARN_BATTERY_VOLTAGE = 3500;          // ~20% - show warning
const uint32_t LOW_BATTERY_VOLTAGE = 3400;           // ~10% - extend sleep
const uint32_t VERY_LOW_BATTERY_VOLTAGE = 3350;      // ~5% - extend sleep more
const uint32_t CRIT_LOW_BATTERY_VOLTAGE = 3300;      // ~2% - hibernate
const unsigned long LOW_BATTERY_SLEEP_INTERVAL = 60; // minutes
const unsigned long VERY_LOW_BATTERY_SLEEP_INTERVAL = 180; // minutes

// =============================================================================
// HOME ASSISTANT CONFIGURATION
// =============================================================================
const char *HA_HOST = SECRET_HA_HOST;
const int HA_PORT = SECRET_HA_PORT;
const char *HA_TOKEN = SECRET_HA_TOKEN;
const char *HA_TEMP_ENTITY = SECRET_HA_TEMP_ENTITY;
const char *HA_HUMIDITY_ENTITY = SECRET_HA_HUMIDITY_ENTITY;

// =============================================================================
// MQTT CONFIGURATION (for Home Assistant telemetry)
// =============================================================================
const char *MQTT_BROKER = SECRET_MQTT_BROKER;
const int MQTT_PORT = SECRET_MQTT_PORT;
const char *MQTT_USERNAME = SECRET_MQTT_USERNAME;
const char *MQTT_PASSWORD = SECRET_MQTT_PASSWORD;
const char *MQTT_CLIENT_ID = SECRET_MQTT_CLIENT_ID;

// =============================================================================
// NEXTCLOUD WEBDAV (for photo/image display modes)
// =============================================================================
const char *NEXTCLOUD_URL = SECRET_NEXTCLOUD_URL;
const char *NEXTCLOUD_USER = SECRET_NEXTCLOUD_USER;
const char *NEXTCLOUD_PASS = SECRET_NEXTCLOUD_PASS;
const char *NEXTCLOUD_PHOTO = SECRET_NEXTCLOUD_PHOTO;
const char *NEXTCLOUD_CARTOON = SECRET_NEXTCLOUD_CARTOON;
const char *NEXTCLOUD_FIRMWARE_PATH = "/Shared/firmware/TRMNL/firmware.bin";