/* API response deserialization for esp32-weather-epd.
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

#include <algorithm>
#include <cmath>
#include <vector>
#include <ArduinoJson.h>
#include "api_response.h"
#include "config.h"

DeserializationError deserializeOneCall(WiFiClient &json,
                                        owm_resp_onecall_t &r)
{
  int i;

  JsonDocument filter;
  filter["current"]  = true;
  filter["minutely"] = false;
  filter["hourly"]   = true;
  filter["daily"]    = true;
#if !DISPLAY_ALERTS
  filter["alerts"]   = false;
#else
  // description can be very long so they are filtered out to save on memory
  // along with sender_name
  for (int i = 0; i < OWM_NUM_ALERTS; ++i)
  {
    filter["alerts"][i]["sender_name"] = false;
    filter["alerts"][i]["event"]       = true;
    filter["alerts"][i]["start"]       = true;
    filter["alerts"][i]["end"]         = true;
    filter["alerts"][i]["description"] = false;
    filter["alerts"][i]["tags"]        = true;
  }
#endif

  JsonDocument doc;

  DeserializationError error = deserializeJson(doc, json,
                                         DeserializationOption::Filter(filter));
#if DEBUG_LEVEL >= 1
  Serial.println("[debug] doc.overflowed() : "
                 + String(doc.overflowed()));
#endif
#if DEBUG_LEVEL >= 2
  serializeJsonPretty(doc, Serial);
#endif
  if (error) {
    return error;
  }

  r.lat             = doc["lat"]            .as<float>();
  r.lon             = doc["lon"]            .as<float>();
  r.timezone        = doc["timezone"]       .as<const char *>();
  r.timezone_offset = doc["timezone_offset"].as<int>();

  JsonObject current = doc["current"];
  r.current.dt         = current["dt"]        .as<int64_t>();
  r.current.sunrise    = current["sunrise"]   .as<int64_t>();
  r.current.sunset     = current["sunset"]    .as<int64_t>();
  r.current.temp       = current["temp"]      .as<float>();
  r.current.feels_like = current["feels_like"].as<float>();
  r.current.pressure   = current["pressure"]  .as<int>();
  r.current.humidity   = current["humidity"]  .as<int>();
  r.current.dew_point  = current["dew_point"] .as<float>();
  r.current.clouds     = current["clouds"]    .as<int>();
  r.current.uvi        = current["uvi"]       .as<float>();
  r.current.visibility = current["visibility"].as<int>();
  r.current.wind_speed = current["wind_speed"].as<float>();
  r.current.wind_gust  = current["wind_gust"] .as<float>();
  r.current.wind_deg   = current["wind_deg"]  .as<int>();
  r.current.rain_1h    = current["rain"]["1h"].as<float>();
  r.current.snow_1h    = current["snow"]["1h"].as<float>();
  JsonObject current_weather = current["weather"][0];
  r.current.weather.id          = current_weather["id"]         .as<int>();
  r.current.weather.main        = current_weather["main"]       .as<const char *>();
  r.current.weather.description = current_weather["description"].as<const char *>();
  r.current.weather.icon        = current_weather["icon"]       .as<const char *>();

  // minutely forecast is currently unused
  // i = 0;
  // for (JsonObject minutely : doc["minutely"].as<JsonArray>())
  // {
  //   r.minutely[i].dt            = minutely["dt"]           .as<int64_t>();
  //   r.minutely[i].precipitation = minutely["precipitation"].as<float>();

  //   if (i == OWM_NUM_MINUTELY - 1)
  //   {
  //     break;
  //   }
  //   ++i;
  // }

  i = 0;
  for (JsonObject hourly : doc["hourly"].as<JsonArray>())
  {
    r.hourly[i].dt         = hourly["dt"]        .as<int64_t>();
    r.hourly[i].temp       = hourly["temp"]      .as<float>();
    r.hourly[i].feels_like = hourly["feels_like"].as<float>();
    r.hourly[i].pressure   = hourly["pressure"]  .as<int>();
    r.hourly[i].humidity   = hourly["humidity"]  .as<int>();
    r.hourly[i].dew_point  = hourly["dew_point"] .as<float>();
    r.hourly[i].clouds     = hourly["clouds"]    .as<int>();
    r.hourly[i].uvi        = hourly["uvi"]       .as<float>();
    r.hourly[i].visibility = hourly["visibility"].as<int>();
    r.hourly[i].wind_speed = hourly["wind_speed"].as<float>();
    r.hourly[i].wind_gust  = hourly["wind_gust"] .as<float>();
    r.hourly[i].wind_deg   = hourly["wind_deg"]  .as<int>();
    r.hourly[i].pop        = hourly["pop"]       .as<float>();
    r.hourly[i].rain_1h    = hourly["rain"]["1h"].as<float>();
    r.hourly[i].snow_1h    = hourly["snow"]["1h"].as<float>();
    JsonObject hourly_weather = hourly["weather"][0];
    r.hourly[i].weather.id          = hourly_weather["id"]         .as<int>();
    r.hourly[i].weather.main        = hourly_weather["main"]       .as<const char *>();
    r.hourly[i].weather.description = hourly_weather["description"].as<const char *>();
    r.hourly[i].weather.icon        = hourly_weather["icon"]       .as<const char *>();

    if (i == OWM_NUM_HOURLY - 1)
    {
      break;
    }
    ++i;
  }

  i = 0;
  for (JsonObject daily : doc["daily"].as<JsonArray>())
  {
    r.daily[i].dt         = daily["dt"]        .as<int64_t>();
    r.daily[i].sunrise    = daily["sunrise"]   .as<int64_t>();
    r.daily[i].sunset     = daily["sunset"]    .as<int64_t>();
    r.daily[i].moonrise   = daily["moonrise"]  .as<int64_t>();
    r.daily[i].moonset    = daily["moonset"]   .as<int64_t>();
    r.daily[i].moon_phase = daily["moon_phase"].as<float>();
    JsonObject daily_temp = daily["temp"];
    r.daily[i].temp.morn  = daily_temp["morn"] .as<float>();
    r.daily[i].temp.day   = daily_temp["day"]  .as<float>();
    r.daily[i].temp.eve   = daily_temp["eve"]  .as<float>();
    r.daily[i].temp.night = daily_temp["night"].as<float>();
    r.daily[i].temp.min   = daily_temp["min"]  .as<float>();
    r.daily[i].temp.max   = daily_temp["max"]  .as<float>();
    JsonObject daily_feels_like = daily["feels_like"];
    r.daily[i].feels_like.morn  = daily_feels_like["morn"] .as<float>();
    r.daily[i].feels_like.day   = daily_feels_like["day"]  .as<float>();
    r.daily[i].feels_like.eve   = daily_feels_like["eve"]  .as<float>();
    r.daily[i].feels_like.night = daily_feels_like["night"].as<float>();
    r.daily[i].pressure   = daily["pressure"]  .as<int>();
    r.daily[i].humidity   = daily["humidity"]  .as<int>();
    r.daily[i].dew_point  = daily["dew_point"] .as<float>();
    r.daily[i].clouds     = daily["clouds"]    .as<int>();
    r.daily[i].uvi        = daily["uvi"]       .as<float>();
    r.daily[i].visibility = daily["visibility"].as<int>();
    r.daily[i].wind_speed = daily["wind_speed"].as<float>();
    r.daily[i].wind_gust  = daily["wind_gust"] .as<float>();
    r.daily[i].wind_deg   = daily["wind_deg"]  .as<int>();
    r.daily[i].pop        = daily["pop"]       .as<float>();
    r.daily[i].rain       = daily["rain"]      .as<float>();
    r.daily[i].snow       = daily["snow"]      .as<float>();
    JsonObject daily_weather = daily["weather"][0];
    r.daily[i].weather.id          = daily_weather["id"]         .as<int>();
    r.daily[i].weather.main        = daily_weather["main"]       .as<const char *>();
    r.daily[i].weather.description = daily_weather["description"].as<const char *>();
    r.daily[i].weather.icon        = daily_weather["icon"]       .as<const char *>();

    if (i == OWM_NUM_DAILY - 1)
    {
      break;
    }
    ++i;
  }

#if DISPLAY_ALERTS
  i = 0;
  for (JsonObject alerts : doc["alerts"].as<JsonArray>())
  {
    owm_alerts_t new_alert = {};
    // new_alert.sender_name = alerts["sender_name"].as<const char *>();
    new_alert.event       = alerts["event"]      .as<const char *>();
    new_alert.start       = alerts["start"]      .as<int64_t>();
    new_alert.end         = alerts["end"]        .as<int64_t>();
    // new_alert.description = alerts["description"].as<const char *>();
    new_alert.tags        = alerts["tags"][0]    .as<const char *>();
    r.alerts.push_back(new_alert);

    if (i == OWM_NUM_ALERTS - 1)
    {
      break;
    }
    ++i;
  }
#endif

  return error;
} // end deserializeOneCall

DeserializationError deserializeAirQuality(WiFiClient &json,
                                           owm_resp_air_pollution_t &r)
{
  int i = 0;

  JsonDocument doc;

  DeserializationError error = deserializeJson(doc, json);
#if DEBUG_LEVEL >= 1
  Serial.println("[debug] doc.overflowed() : "
                 + String(doc.overflowed()));
#endif
#if DEBUG_LEVEL >= 2
  serializeJsonPretty(doc, Serial);
#endif
  if (error) {
    return error;
  }

  r.coord.lat = doc["coord"]["lat"].as<float>();
  r.coord.lon = doc["coord"]["lon"].as<float>();

  for (JsonObject list : doc["list"].as<JsonArray>())
  {

    r.main_aqi[i] = list["main"]["aqi"].as<int>();

    JsonObject list_components = list["components"];
    r.components.co[i]    = list_components["co"].as<float>();
    r.components.no[i]    = list_components["no"].as<float>();
    r.components.no2[i]   = list_components["no2"].as<float>();
    r.components.o3[i]    = list_components["o3"].as<float>();
    r.components.so2[i]   = list_components["so2"].as<float>();
    r.components.pm2_5[i] = list_components["pm2_5"].as<float>();
    r.components.pm10[i]  = list_components["pm10"].as<float>();
    r.components.nh3[i]   = list_components["nh3"].as<float>();

    r.dt[i] = list["dt"].as<int64_t>();

    if (i == OWM_NUM_AIR_POLLUTION - 1)
    {
      break;
    }
    ++i;
  }

  return error;
} // end deserializeAirQuality

/* Deserializes OpenWeatherMap's free-tier "Current Weather" API response
 * (/data/2.5/weather) into the "current" portion of owm_resp_onecall_t.
 * Fields not provided by this endpoint (uvi, dew_point) are left as 0,
 * which all downstream rendering code already treats as a safe default.
 */
DeserializationError deserializeCurrentWeather(WiFiClient &json,
                                               owm_resp_onecall_t &r)
{
  JsonDocument doc;

  DeserializationError error = deserializeJson(doc, json);
#if DEBUG_LEVEL >= 1
  Serial.println("[debug] doc.overflowed() : "
                 + String(doc.overflowed()));
#endif
#if DEBUG_LEVEL >= 2
  serializeJsonPretty(doc, Serial);
#endif
  if (error) {
    return error;
  }

  r.lat             = doc["coord"]["lat"].as<float>();
  r.lon             = doc["coord"]["lon"].as<float>();
  r.timezone_offset = doc["timezone"]    .as<int>();

  r.current.dt         = doc["dt"]             .as<int64_t>();
  r.current.sunrise    = doc["sys"]["sunrise"] .as<int64_t>();
  r.current.sunset     = doc["sys"]["sunset"]  .as<int64_t>();
  r.current.temp       = doc["main"]["temp"]      .as<float>();
  r.current.feels_like = doc["main"]["feels_like"].as<float>();
  r.current.pressure   = doc["main"]["pressure"]  .as<int>();
  r.current.humidity   = doc["main"]["humidity"]  .as<int>();
  r.current.dew_point  = 0.f; // unavailable on free tier
  r.current.clouds     = doc["clouds"]["all"]  .as<int>();
  r.current.uvi        = 0.f; // unavailable on free tier (requires One Call)
  r.current.visibility = doc["visibility"]     .as<int>();
  r.current.wind_speed = doc["wind"]["speed"]  .as<float>();
  r.current.wind_gust  = doc["wind"]["gust"]   .as<float>();
  r.current.wind_deg   = doc["wind"]["deg"]    .as<int>();
  r.current.rain_1h    = doc["rain"]["1h"]     .as<float>();
  r.current.snow_1h    = doc["snow"]["1h"]     .as<float>();
  JsonObject current_weather = doc["weather"][0];
  r.current.weather.id          = current_weather["id"]         .as<int>();
  r.current.weather.main        = current_weather["main"]       .as<const char *>();
  r.current.weather.description = current_weather["description"].as<const char *>();
  r.current.weather.icon        = current_weather["icon"]       .as<const char *>();

  return error;
} // end deserializeCurrentWeather

/* Deserializes OpenWeatherMap's free-tier "5 day / 3 hour Forecast" API
 * response (/data/2.5/forecast) into the "hourly" and "daily" portions of
 * owm_resp_onecall_t.
 *
 * The forecast endpoint only returns 3-hourly entries (up to 40, covering
 * ~5 days), so:
 *  - hourly[] is filled directly from those 3-hour entries (these are not
 *    truly hourly, but every field renderer.cpp reads from hourly[] is
 *    still valid/meaningful at 3-hour resolution).
 *  - daily[] is built by aggregating each calendar day's entries: min/max
 *    temp, summed rain/snow, max pop, and the weather/clouds/wind from the
 *    entry closest to local noon (used as the representative icon/
 *    condition for that day, matching how a real daily summary would
 *    read).
 *
 * Fields the free tier never provides (dew_point, uvi, moonrise, moonset,
 * moon_phase) are left at 0, which is a safe default already handled by
 * the renderer (e.g. isMoonInSky() returns false when moon_phase == 0).
 */
DeserializationError deserializeForecast(WiFiClient &json,
                                         owm_resp_onecall_t &r)
{
  JsonDocument filter;
  filter["city"]["timezone"] = true;
  filter["city"]["sunrise"]  = true;
  filter["city"]["sunset"]   = true;
  JsonObject listFilter = filter["list"].add<JsonObject>();
  listFilter["dt"]                       = true;
  listFilter["main"]["temp"]             = true;
  listFilter["main"]["feels_like"]       = true;
  listFilter["main"]["pressure"]         = true;
  listFilter["main"]["humidity"]         = true;
  listFilter["main"]["temp_min"]         = true;
  listFilter["main"]["temp_max"]         = true;
  listFilter["clouds"]["all"]            = true;
  listFilter["wind"]["speed"]            = true;
  listFilter["wind"]["gust"]             = true;
  listFilter["wind"]["deg"]              = true;
  listFilter["visibility"]               = true;
  listFilter["pop"]                      = true;
  listFilter["rain"]["3h"]               = true;
  listFilter["snow"]["3h"]               = true;
  listFilter["weather"][0]["id"]          = true;
  listFilter["weather"][0]["main"]        = true;
  listFilter["weather"][0]["description"] = true;
  listFilter["weather"][0]["icon"]        = true;

  JsonDocument doc;

  DeserializationError error = deserializeJson(doc, json,
                                         DeserializationOption::Filter(filter));
#if DEBUG_LEVEL >= 1
  Serial.println("[debug] doc.overflowed() : "
                 + String(doc.overflowed()));
#endif
#if DEBUG_LEVEL >= 2
  serializeJsonPretty(doc, Serial);
#endif
  if (error) {
    return error;
  }

  r.timezone_offset = doc["city"]["timezone"].as<int>();
  int64_t city_sunrise = doc["city"]["sunrise"].as<int64_t>();
  int64_t city_sunset  = doc["city"]["sunset"] .as<int64_t>();

  bool  dayInit[OWM_NUM_DAILY];
  float bestNoonDist[OWM_NUM_DAILY];
  for (int k = 0; k < OWM_NUM_DAILY; ++k)
  {
    dayInit[k] = false;
    bestNoonDist[k] = 1e9f;
  }
  int64_t firstLocalDayIdx = -1;

  int i = 0;
  for (JsonObject entry : doc["list"].as<JsonArray>())
  {
    if (i >= OWM_NUM_HOURLY)
    {
      break;
    }

    owm_hourly_t &h = r.hourly[i];
    h.dt         = entry["dt"]              .as<int64_t>();
    h.temp       = entry["main"]["temp"]      .as<float>();
    h.feels_like = entry["main"]["feels_like"].as<float>();
    h.pressure   = entry["main"]["pressure"]  .as<int>();
    h.humidity   = entry["main"]["humidity"]  .as<int>();
    h.dew_point  = 0.f;
    h.clouds     = entry["clouds"]["all"]   .as<int>();
    h.uvi        = 0.f;
    h.visibility = entry["visibility"]      .as<int>();
    h.wind_speed = entry["wind"]["speed"]   .as<float>();
    h.wind_gust  = entry["wind"]["gust"]    .as<float>();
    h.wind_deg   = entry["wind"]["deg"]     .as<int>();
    h.pop        = entry["pop"]             .as<float>();
    // forecast volumes are accumulated over the 3h bucket; closest
    // equivalent available on this endpoint
    h.rain_1h    = entry["rain"]["3h"]       .as<float>();
    h.snow_1h    = entry["snow"]["3h"]       .as<float>();
    JsonObject hourly_weather = entry["weather"][0];
    h.weather.id          = hourly_weather["id"]         .as<int>();
    h.weather.main        = hourly_weather["main"]       .as<const char *>();
    h.weather.description = hourly_weather["description"].as<const char *>();
    h.weather.icon        = hourly_weather["icon"]       .as<const char *>();

    // --- aggregate into daily[] ---
    int64_t localDt    = h.dt + r.timezone_offset;
    int64_t localDayIdx = localDt / 86400;
    if (firstLocalDayIdx < 0)
    {
      firstLocalDayIdx = localDayIdx;
    }
    int dayBucket = static_cast<int>(localDayIdx - firstLocalDayIdx);
    if (dayBucket >= 0 && dayBucket < OWM_NUM_DAILY)
    {
      owm_daily_t &d = r.daily[dayBucket];
      float tmin = entry["main"]["temp_min"].as<float>();
      float tmax = entry["main"]["temp_max"].as<float>();

      if (!dayInit[dayBucket])
      {
        dayInit[dayBucket] = true;
        d.dt   = h.dt;
        d.temp.min = tmin;
        d.temp.max = tmax;
        d.pop  = h.pop;
        d.rain = h.rain_1h;
        d.snow = h.snow_1h;
      }
      else
      {
        d.temp.min = std::min(d.temp.min, tmin);
        d.temp.max = std::max(d.temp.max, tmax);
        d.pop  = std::max(d.pop, h.pop);
        d.rain += h.rain_1h;
        d.snow += h.snow_1h;
      }

      // pick the entry closest to local noon as representative of the day
      int64_t secOfDay = ((localDt % 86400) + 86400) % 86400;
      float distFromNoon = std::fabs(static_cast<float>(secOfDay) - 12.f * 3600.f);
      if (distFromNoon < bestNoonDist[dayBucket])
      {
        bestNoonDist[dayBucket] = distFromNoon;
        d.weather    = h.weather;
        d.clouds     = h.clouds;
        d.wind_speed = h.wind_speed;
        d.wind_gust  = h.wind_gust;
        d.wind_deg   = h.wind_deg;
        d.humidity   = h.humidity;
        d.pressure   = h.pressure;
        d.visibility = h.visibility;
      }

      d.dew_point  = 0.f;
      d.uvi        = 0.f;
      d.moonrise   = 0;
      d.moonset    = 0;
      d.moon_phase = 0.f;
      if (dayBucket == 0)
      {
        d.sunrise = city_sunrise;
        d.sunset  = city_sunset;
      }
    }

    ++i;
  }

  return error;
} // end deserializeForecast

