/* Client side utilities for esp32-weather-epd.
 * Copyright (C) 2022-2024  Luke Marzen
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

// built-in C++ libraries
#include <cstring>
#include <vector>

// arduino/esp32 libraries
#include <Arduino.h>
#include <esp_sntp.h>
#include <HTTPClient.h>
#include <SPI.h>
#include <time.h>
#include <WiFi.h>

// additional libraries
#include <Adafruit_BusIO_Register.h>
#include <ArduinoJson.h>

// header files
#include "_locale.h"
#include "api_response.h"
#include "aqi.h"
#include "client_utils.h"
#include "config.h"
#include "display_utils.h"
#include "renderer.h"
#ifndef USE_HTTP
  #include <WiFiClientSecure.h>
#endif

// WU/TWC always uses HTTPS port 443 regardless of USE_HTTP
// (USE_HTTP is only kept for WiFiClient/WiFiClientSecure selection)

/* Power-on and connect WiFi.
 * Takes int parameter to store WiFi RSSI, or "Received Signal Strength
 * Indicator"
 *
 * Returns WiFi status.
 */
wl_status_t startWiFi(int &wifiRSSI)
{
  WiFi.mode(WIFI_STA);
  Serial.printf("%s '%s'", TXT_CONNECTING_TO, WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  // timeout if WiFi does not connect in WIFI_TIMEOUT ms from now
  unsigned long timeout = millis() + WIFI_TIMEOUT;
  wl_status_t connection_status = WiFi.status();

  while ((connection_status != WL_CONNECTED) && (millis() < timeout))
  {
    Serial.print(".");
    delay(50);
    connection_status = WiFi.status();
  }
  Serial.println();

  if (connection_status == WL_CONNECTED)
  {
    wifiRSSI = WiFi.RSSI(); // get WiFi signal strength now, because the WiFi
                            // will be turned off to save power!
    Serial.println("IP: " + WiFi.localIP().toString());
    Serial.println("DNS: " + WiFi.dnsIP().toString());
    
    // Wait for DNS to be ready - DHCP provides IP but DNS may lag behind
    // This prevents DNS resolution failures on first API calls
    Serial.print("Waiting for DNS");
    int dnsWaitAttempts = 0;
    const int maxDnsWait = 20; // 2 seconds max
    while (WiFi.dnsIP() == IPAddress(0, 0, 0, 0) && dnsWaitAttempts < maxDnsWait)
    {
      Serial.print(".");
      delay(100);
      dnsWaitAttempts++;
    }
    Serial.println();
    
    if (WiFi.dnsIP() == IPAddress(0, 0, 0, 0))
    {
      Serial.println("Warning: DNS not available, using fallback");
      // Set Google DNS as fallback
      WiFi.config(WiFi.localIP(), WiFi.gatewayIP(), WiFi.subnetMask(),
                  IPAddress(8, 8, 8, 8), IPAddress(8, 8, 4, 4));
      Serial.println("DNS (fallback): " + WiFi.dnsIP().toString());
    }
    
    // Additional stabilization delay for network stack
    delay(500);
    Serial.println("Network ready");
  }
  else
  {
    Serial.printf("%s '%s'\n", TXT_COULD_NOT_CONNECT_TO, WIFI_SSID);
  }
  return connection_status;
} // startWiFi

/* Disconnect and power-off WiFi.
 */
void killWiFi()
{
  WiFi.disconnect();
  WiFi.mode(WIFI_OFF);
} // killWiFi

/* Prints the local time to serial monitor.
 *
 * Returns true if getting local time was a success, otherwise false.
 */
bool printLocalTime(tm *timeInfo)
{
  int attempts = 0;
  while (!getLocalTime(timeInfo) && attempts++ < 3)
  {
    Serial.println(TXT_FAILED_TO_GET_TIME);
    return false;
  }
  Serial.println(timeInfo, "%A, %B %d, %Y %H:%M:%S");
  return true;
} // printLocalTime

/* Waits for NTP server time sync, adjusted for the time zone specified in
 * config.cpp.
 *
 * Returns true if time was set successfully, otherwise false.
 *
 * Note: Must be connected to WiFi to get time from NTP server.
 */
bool waitForSNTPSync(tm *timeInfo)
{
  // Wait for SNTP synchronization to complete
  unsigned long timeout = millis() + NTP_TIMEOUT;
  if ((sntp_get_sync_status() == SNTP_SYNC_STATUS_RESET)
      && (millis() < timeout))
  {
    Serial.print(TXT_WAITING_FOR_SNTP);
    delay(100); // ms
    while ((sntp_get_sync_status() == SNTP_SYNC_STATUS_RESET)
        && (millis() < timeout))
    {
      Serial.print(".");
      delay(100); // ms
    }
    Serial.println();
  }
  return printLocalTime(timeInfo);
} // waitForSNTPSync

static const char *WU_HOST = "api.weather.com";
static const uint16_t WU_PORT = 443;

/* Helper: perform a single HTTP GET, parse JSON with the supplied deserializer.
 * Retries up to 3 times. Returns HTTP status code (or negative error code).
 */
template <typename ClientT, typename DeserFn>
static int wuGet(ClientT &client, const String &sanitizedUri,
                 const String &uri, DeserFn deserFn, owm_resp_onecall_t &r)
{
  int attempts = 0;
  bool rxSuccess = false;
  DeserializationError jsonErr = {};
  int httpResponse = 0;

  Serial.print(TXT_ATTEMPTING_HTTP_REQ);
  Serial.println(": " + sanitizedUri);

  while (!rxSuccess && attempts < 3)
  {
    if (WiFi.status() != WL_CONNECTED)
      return -512 - static_cast<int>(WiFi.status());

    HTTPClient http;
    http.setConnectTimeout(HTTP_CLIENT_TCP_TIMEOUT);
    http.setTimeout(HTTP_CLIENT_TCP_TIMEOUT);
    http.begin(client, WU_HOST, WU_PORT, uri);
    httpResponse = http.GET();
    if (httpResponse == HTTP_CODE_OK)
    {
      jsonErr = deserFn(http.getStream(), r);
      if (jsonErr)
        httpResponse = -256 - static_cast<int>(jsonErr.code());
      rxSuccess = !jsonErr;
    }
    client.stop();
    http.end();
    Serial.println("  " + String(httpResponse, DEC) + " "
                   + getHttpResponsePhrase(httpResponse));
    ++attempts;
  }
  return httpResponse;
}

/* Fetch WU PWS current conditions (v2) and TWC 5-day daily forecast (v3),
 * then merge both into r so the rest of the codebase sees a single unified
 * weather object (same owm_resp_onecall_t shape as before).
 *
 * Call order: forecast first (sets daily[0].sunrise/sunset), then current
 * (uses those timestamps to determine day/night for the current icon).
 *
 * Returns HTTP_CODE_OK only if both requests succeed.
 */
#ifdef USE_HTTP
  int getWUonecall(WiFiClient &client, owm_resp_onecall_t &r)
#else
  int getWUonecall(WiFiClientSecure &client, owm_resp_onecall_t &r)
#endif
{
  // --- 5-day daily forecast ---
  String fUri = "/v3/wx/forecast/daily/5day?geocode=" + LAT + "," + LON
                + "&format=json&units=m&language=" + WU_LANGUAGE
                + "&apiKey=" + WU_APIKEY;
  String fSanitized = String("/v3/wx/forecast/daily/5day?geocode=") + LAT + "," + LON
                      + "&format=json&units=m&language=" + WU_LANGUAGE
                      + "&apiKey={API key}";

  int status = wuGet(client, fSanitized, fUri,
                     [](WiFiClient &s, owm_resp_onecall_t &o) {
                       return deserializeWUForecast(s, o);
                     }, r);
  if (status != HTTP_CODE_OK) return status;

  // --- PWS current conditions ---
  String cUri = "/v2/pws/observations/current?stationId=" + WU_STATION_ID
                + "&format=json&units=m&apiKey=" + WU_APIKEY;
  String cSanitized = String("/v2/pws/observations/current?stationId=") + WU_STATION_ID
                      + "&format=json&units=m&apiKey={API key}";

  status = wuGet(client, cSanitized, cUri,
                 [](WiFiClient &s, owm_resp_onecall_t &o) {
                   return deserializeWUCurrent(s, o);
                 }, r);
  if (status != HTTP_CODE_OK) return status;

  // --- Patch current conditions using forecast data ---
  // PWS does not provide icon, sunrise/sunset, clouds or visibility
  if (r.current.sunrise == 0 && r.daily[0].sunrise != 0)
  {
    r.current.sunrise = r.daily[0].sunrise;
    r.current.sunset  = r.daily[0].sunset;
  }

  bool isDay = (r.current.sunrise != 0 && r.current.sunset != 0)
               ? (r.current.dt >= r.current.sunrise && r.current.dt < r.current.sunset)
               : true;

  int dpIdx = isDay ? 0 : 1;
  r.current.weather.id          = r.hourly[dpIdx].weather.id;
  r.current.weather.main        = r.hourly[dpIdx].weather.main;
  r.current.weather.description = r.hourly[dpIdx].weather.description;
  r.current.weather.icon        = String(isDay ? "d" : "n");
  r.current.clouds              = r.hourly[dpIdx].clouds;
  r.current.visibility          = 10000;

  return HTTP_CODE_OK;
} // getWUonecall

/* getOWMairpollution: WU/TWC does not offer an equivalent API, so this
 * always returns an error. The caller (main.cpp) already handles
 * airPollutionSuccess = false gracefully.
 */
#ifdef USE_HTTP
  int getOWMairpollution(WiFiClient &client, owm_resp_air_pollution_t &r)
#else
  int getOWMairpollution(WiFiClientSecure &client, owm_resp_air_pollution_t &r)
#endif
{
  (void)client;
  (void)r;
  Serial.println("[info] Air pollution API not available for Weather Underground");
  return HTTP_CODE_NOT_FOUND; // treated as non-critical failure in main.cpp
} // getOWMairpollution

/* Prints debug information about heap usage.
 */
void printHeapUsage() {
  Serial.println("[debug] Heap Size       : "
                 + String(ESP.getHeapSize()) + " B");
  Serial.println("[debug] Available Heap  : "
                 + String(ESP.getFreeHeap()) + " B");
  Serial.println("[debug] Min Free Heap   : "
                 + String(ESP.getMinFreeHeap()) + " B");
  Serial.println("[debug] Max Allocatable : "
                 + String(ESP.getMaxAllocHeap()) + " B");
  return;
}

