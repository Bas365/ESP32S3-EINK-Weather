/* SECRETS TEMPLATE
 * Copy this file to secrets.h and fill in your own values.
 * secrets.h is gitignored and will not be committed.
 */

#ifndef SECRETS_H
#define SECRETS_H

// =============================================================================
// WIFI CREDENTIALS (optional - WiFiManager handles this via captive portal)
// Leave empty to use WiFiManager's captive portal for setup
// =============================================================================
#define SECRET_WIFI_SSID     ""
#define SECRET_WIFI_PASSWORD "tehe"

// =============================================================================
// OPENWEATHERMAP API
// Get your free API key at: https://openweathermap.org/api
// =============================================================================
#define SECRET_OWM_APIKEY    "your_openweathermap_api_key"

// =============================================================================
// LOCATION
// Find your coordinates at: https://www.latlong.net/
// =============================================================================
#define SECRET_LAT           "51.5074"    // e.g., London
#define SECRET_LON           "-0.1278"
#define SECRET_CITY_STRING   "London"

// =============================================================================
// HOME ASSISTANT (optional - for indoor temperature/humidity)
// Generate a Long-Lived Access Token: HA Profile -> Security -> Long-Lived Access Tokens
// Set to empty strings to disable
// =============================================================================
#define SECRET_HA_HOST            "192.168.1.100"  // Your HA IP address
#define SECRET_HA_PORT            8123
#define SECRET_HA_TOKEN           "your_long_lived_access_token"
#define SECRET_HA_TEMP_ENTITY     "sensor.living_room_temperature"
#define SECRET_HA_HUMIDITY_ENTITY "sensor.living_room_humidity"

// =============================================================================
// MQTT (optional - for publishing battery telemetry to Home Assistant)
// Set broker to empty string to disable
// =============================================================================
#define SECRET_MQTT_BROKER    "192.168.1.100"  // Your MQTT broker IP
#define SECRET_MQTT_PORT      1883
#define SECRET_MQTT_USERNAME  "mqtt_user"
#define SECRET_MQTT_PASSWORD  "mqtt_password"
#define SECRET_MQTT_CLIENT_ID "trmnl_weather_station"

// =============================================================================
// NEXTCLOUD WEBDAV (optional - for photo/cartoon display modes)
// Set URL to empty string to disable image modes
// =============================================================================
#define SECRET_NEXTCLOUD_URL      "https://your-nextcloud.com/remote.php/dav/files/username/path/"
#define SECRET_NEXTCLOUD_USER     "your_username"
#define SECRET_NEXTCLOUD_PASS     "your_password"
#define SECRET_NEXTCLOUD_PHOTO    "photo.png"
#define SECRET_NEXTCLOUD_CARTOON  "cartoon.png"

#endif // SECRETS_H

