/* API response deserialization for TWC APIs.
 *
 * Populates owm_resp_onecall_t (originally for OpenWeatherMap) from:
 *   - TWC Current Conditions v3 (geocode-based, no PWS required)
 *   - TWC Daily Forecast v3 (5-day forecast used for daily[] and hourly[])
 *
 * Mapping notes:
 *   weather.id   = TWC icon code (0-47)
 *   weather.icon = "d" (day) or "n" (night)
 *   Temperatures converted Celsius -> Kelvin (+273.15)
 *   Wind speeds converted km/h -> m/s (/3.6)
 */

#include <algorithm>
#include <cmath>
#include <vector>
#include <ArduinoJson.h>
#include "api_response.h"
#include "config.h"

/* Deserializes WU PWS Current Conditions v2 response into r.current.
 *
 * Fields not available from PWS (weather icon, sunrise/sunset, clouds,
 * visibility) are left at 0 — getWUonecall() patches these from the forecast.
 */
DeserializationError deserializeWUCurrent(WiFiClient &json,
                                          owm_resp_onecall_t &r)
{
  JsonDocument filter;
  JsonObject obsFilter = filter["observations"].add<JsonObject>();
  obsFilter["epoch"]         = true;
  obsFilter["lat"]           = true;
  obsFilter["lon"]           = true;
  obsFilter["humidity"]      = true;
  obsFilter["uv"]            = true;
  obsFilter["winddir"]       = true;
  obsFilter["metric"]["temp"]        = true;
  obsFilter["metric"]["heatIndex"]   = true;
  obsFilter["metric"]["dewpt"]       = true;
  obsFilter["metric"]["windSpeed"]   = true;
  obsFilter["metric"]["windGust"]    = true;
  obsFilter["metric"]["pressure"]    = true;
  obsFilter["metric"]["precipRate"]  = true;

  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, json,
                                               DeserializationOption::Filter(filter));
#if DEBUG_LEVEL >= 1
  Serial.println("[debug] WUCurrent doc.overflowed() : " + String(doc.overflowed()));
#endif
#if DEBUG_LEVEL >= 2
  serializeJsonPretty(doc, Serial);
#endif
  if (error) return error;

  JsonObject obs = doc["observations"][0];
  if (obs.isNull()) return error;

  r.lat = obs["lat"].as<float>();
  r.lon = obs["lon"].as<float>();

  r.current.dt         = obs["epoch"]  .as<int64_t>();
  r.current.humidity   = obs["humidity"].as<int>();
  r.current.uvi        = obs["uv"]     .as<float>();
  r.current.wind_deg   = obs["winddir"].as<int>();

  JsonObject m = obs["metric"];
  r.current.temp       = m["temp"]      .as<float>() + 273.15f;
  r.current.feels_like = m["heatIndex"] .as<float>() + 273.15f;
  r.current.dew_point  = m["dewpt"]     .as<float>() + 273.15f;
  r.current.pressure   = static_cast<int>(m["pressure"].as<float>());
  r.current.wind_speed = m["windSpeed"] .as<float>() / 3.6f;
  r.current.wind_gust  = m["windGust"]  .as<float>() / 3.6f;
  r.current.rain_1h    = m["precipRate"].as<float>();

  // clouds, visibility, sunrise, sunset, weather: patched by getWUonecall()

  return error;
} // end deserializeWUCurrent

/* Deserializes TWC Daily Forecast v3 (5-day) response into r.daily[] and
 * r.hourly[] (using daypart entries as 12-hourly buckets).
 *
 * Also fills r.current.sunrise and r.current.sunset from today's forecast.
 */
DeserializationError deserializeWUForecast(WiFiClient &json,
                                           owm_resp_onecall_t &r)
{
  // Filter to only fields we use
  JsonDocument filter;
  filter["temperatureMax"]         = true;
  filter["temperatureMin"]         = true;
  filter["qpf"]                    = true;
  filter["qpfSnow"]                = true;
  filter["moonPhaseDay"]           = true;
  filter["sunriseTimeUtc"]         = true;
  filter["sunsetTimeUtc"]          = true;
  filter["validTimeUtc"]           = true;
  JsonObject dpFilter = filter["daypart"].add<JsonObject>();
  dpFilter["cloudCover"]       = true;
  dpFilter["dayOrNight"]       = true;
  dpFilter["iconCode"]         = true;
  dpFilter["precipChance"]     = true;
  dpFilter["qpf"]              = true;
  dpFilter["relativeHumidity"] = true;
  dpFilter["temperature"]      = true;
  dpFilter["uvIndex"]          = true;
  dpFilter["windDirection"]    = true;
  dpFilter["windSpeed"]        = true;
  dpFilter["wxPhraseShort"]    = true;

  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, json,
                                               DeserializationOption::Filter(filter));
#if DEBUG_LEVEL >= 1
  Serial.println("[debug] WUForecast doc.overflowed() : " + String(doc.overflowed()));
#endif
#if DEBUG_LEVEL >= 2
  serializeJsonPretty(doc, Serial);
#endif
  if (error) return error;

  JsonArray daypart = doc["daypart"][0].as<JsonArray>();

  // --- Fill daily[] from 5-day forecast entries ---
  int numDays = 0;
  JsonArray validTimeUtc   = doc["validTimeUtc"]  .as<JsonArray>();
  JsonArray sunriseUtc     = doc["sunriseTimeUtc"].as<JsonArray>();
  JsonArray sunsetUtc      = doc["sunsetTimeUtc"] .as<JsonArray>();
  JsonArray tempMax        = doc["temperatureMax"].as<JsonArray>();
  JsonArray tempMin        = doc["temperatureMin"].as<JsonArray>();
  JsonArray qpf            = doc["qpf"]           .as<JsonArray>();
  JsonArray qpfSnow        = doc["qpfSnow"]       .as<JsonArray>();
  JsonArray moonPhaseDay   = doc["moonPhaseDay"]  .as<JsonArray>();

  for (JsonVariant v : validTimeUtc)
  {
    if (numDays >= OWM_NUM_DAILY) break;

    owm_daily_t &d = r.daily[numDays];
    d.dt         = v.as<int64_t>();
    d.sunrise    = sunriseUtc[numDays].isNull()   ? 0 : sunriseUtc[numDays]  .as<int64_t>();
    d.sunset     = sunsetUtc[numDays] .isNull()   ? 0 : sunsetUtc[numDays]   .as<int64_t>();
    d.moonrise   = 0;
    d.moonset    = 0;
    d.moon_phase = moonPhaseDay[numDays].isNull() ? 0.f
                   : moonPhaseDay[numDays].as<float>() / 29.5f;

    float tmax = tempMax[numDays].isNull() ? 0.f : tempMax[numDays].as<float>();
    float tmin = tempMin[numDays].isNull() ? 0.f : tempMin[numDays].as<float>();
    d.temp.max   = tmax + 273.15f;
    d.temp.min   = tmin + 273.15f;
    d.temp.day   = (tmax + tmin) / 2.f + 273.15f;
    d.temp.morn  = d.temp.min;
    d.temp.eve   = d.temp.day;
    d.temp.night = d.temp.min;
    d.rain       = qpf[numDays]    .isNull() ? 0.f : qpf[numDays]    .as<float>();
    d.snow       = qpfSnow[numDays].isNull() ? 0.f : qpfSnow[numDays].as<float>();

    // Use the daytime daypart (index = numDays*2) for daily representative values
    int dpDay   = numDays * 2;     // daytime daypart index
    int dpNight = numDays * 2 + 1; // nighttime daypart index

    // Prefer daytime daypart; fall back to nighttime if daytime is null
    bool dayNull = !daypart || daypart[dpDay].isNull() ||
                   daypart[dpDay]["iconCode"].isNull();

    int dpIdx = dayNull ? dpNight : dpDay;

    if (daypart && !daypart[dpIdx].isNull())
    {
      JsonObject dp = daypart[dpIdx].as<JsonObject>();
      JsonVariant icV = dp["iconCode"];
      JsonVariant ccV = dp["cloudCover"];
      JsonVariant pcV = dp["precipChance"];
      JsonVariant rhV = dp["relativeHumidity"];
      JsonVariant wsV = dp["windSpeed"];
      JsonVariant wdV = dp["windDirection"];
      JsonVariant uvV = dp["uvIndex"];
      JsonVariant phV = dp["wxPhraseShort"];

      d.weather.id          = icV.isNull() ? 44 : icV.as<int>();
      d.weather.icon        = String(dayNull ? "n" : "d");
      d.weather.description = phV.isNull() ? String("") : phV.as<String>();
      d.weather.main        = d.weather.description;
      d.clouds              = ccV.isNull() ? 0 : ccV.as<int>();
      d.pop                 = pcV.isNull() ? 0.f : pcV.as<float>() / 100.f;
      d.humidity            = rhV.isNull() ? 0 : rhV.as<int>();
      d.wind_speed          = wsV.isNull() ? 0.f : wsV.as<float>() / 3.6f;
      d.wind_deg            = wdV.isNull() ? 0 : wdV.as<int>();
      d.uvi                 = uvV.isNull() ? 0.f : uvV.as<float>();
    }

    d.dew_point  = 0.f;
    d.pressure   = 0;
    d.visibility = 0;
    d.wind_gust  = 0.f;

    ++numDays;
  }

  // Propagate today's sunrise/sunset to current (getWUonecall patches this)
  if (numDays > 0 && r.daily[0].sunrise != 0)
  {
    r.current.sunrise = r.daily[0].sunrise;
    r.current.sunset  = r.daily[0].sunset;
  }

  // --- Fill hourly[] from daypart entries (2 per day, up to 10 entries) ---
  int numDP = 0;
  if (daypart)
  {
    for (JsonVariant dpV : daypart)
    {
      if (numDP >= OWM_NUM_HOURLY) break;
      if (dpV.isNull()) { ++numDP; continue; }

      JsonObject dp = dpV.as<JsonObject>();
      if (dp.isNull()) { ++numDP; continue; }

      owm_hourly_t &h = r.hourly[numDP];
      // Approximate timestamp: day's validTime + 6h (day) or +18h (night)
      int dayIdx = numDP / 2;
      bool isNightPart = (numDP % 2) == 1;
      h.dt = (dayIdx < numDays ? r.daily[dayIdx].dt : 0)
             + (isNightPart ? 18 * 3600LL : 6 * 3600LL);

      JsonVariant tempV = dp["temperature"];
      JsonVariant ccV2  = dp["cloudCover"];
      JsonVariant rhV2  = dp["relativeHumidity"];
      JsonVariant wsV2  = dp["windSpeed"];
      JsonVariant wdV2  = dp["windDirection"];
      JsonVariant pcV2  = dp["precipChance"];
      JsonVariant qfV2  = dp["qpf"];
      JsonVariant uvV2  = dp["uvIndex"];
      JsonVariant icV2  = dp["iconCode"];
      JsonVariant donV  = dp["dayOrNight"];
      JsonVariant phV2  = dp["wxPhraseShort"];

      float tempC    = tempV.isNull() ? 0.f : tempV.as<float>();
      h.temp         = tempC + 273.15f;
      h.feels_like   = h.temp;
      h.clouds       = ccV2.isNull() ? 0 : ccV2.as<int>();
      h.humidity     = rhV2.isNull() ? 0 : rhV2.as<int>();
      h.wind_speed   = wsV2.isNull() ? 0.f : wsV2.as<float>() / 3.6f;
      h.wind_deg     = wdV2.isNull() ? 0 : wdV2.as<int>();
      h.pop          = pcV2.isNull() ? 0.f : pcV2.as<float>() / 100.f;
      h.rain_1h      = qfV2.isNull() ? 0.f : qfV2.as<float>();
      h.uvi          = uvV2.isNull() ? 0.f : uvV2.as<float>();
      h.weather.id   = icV2.isNull() ? 44 : icV2.as<int>();
      h.weather.icon = (!donV.isNull() && donV.as<String>() == "D") ? "d" : "n";
      h.weather.description = phV2.isNull() ? String("") : phV2.as<String>();
      h.weather.main = h.weather.description;

      h.dew_point  = 0.f;
      h.pressure   = 0;
      h.visibility = 0;
      h.wind_gust  = 0.f;
      h.snow_1h    = 0.f;

      ++numDP;
    }
  }

  return error;
} // end deserializeWUForecast
