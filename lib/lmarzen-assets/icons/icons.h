// Minimal icons.h for TRMNL OG Weather Station
// Only includes icons needed for weather display

#ifndef __ICONS_H__
#define __ICONS_H__

#include <cstddef>

// Include minimal icon sets
#include "icons_minimal_16x16.h"
#include "icons_minimal_24x24.h"
#include "icons_minimal_32x32.h"
#include "icons_minimal_48x48.h"
#include "icons_minimal_64x64.h"
#include "icons_196x196.h"

// Icon name enum - only icons we actually use
typedef enum icon_name {
  // Weather icons
  wi_thunderstorm,
  wi_day_thunderstorm,
  wi_night_alt_thunderstorm,
  wi_storm_showers,
  wi_day_storm_showers,
  wi_night_alt_storm_showers,
  wi_showers,
  wi_day_showers,
  wi_night_alt_showers,
  wi_rain,
  wi_rain_wind,
  wi_day_rain,
  wi_day_rain_wind,
  wi_night_alt_rain,
  wi_night_alt_rain_wind,
  wi_rain_mix,
  wi_day_rain_mix,
  wi_night_alt_rain_mix,
  wi_snow,
  wi_snow_wind,
  wi_day_snow,
  wi_day_snow_wind,
  wi_night_alt_snow,
  wi_night_alt_snow_wind,
  wi_sleet,
  wi_day_sleet,
  wi_night_alt_sleet,
  wi_fog,
  wi_day_fog,
  wi_night_fog,
  wi_smoke,
  wi_dust,
  wi_day_haze,
  wi_sandstorm,
  wi_cloudy_gusts,
  wi_tornado,
  wi_day_sunny,
  wi_night_clear,
  wi_stars,
  wi_day_sunny_overcast,
  wi_night_alt_partly_cloudy,
  wi_day_cloudy,
  wi_day_cloudy_gusts,
  wi_night_alt_cloudy,
  wi_night_alt_cloudy_gusts,
  wi_cloud,
  wi_cloudy,
  wi_na,
  wi_strong_wind,
  wi_volcano,
  wi_hot,
  wi_snowflake_cold,
  wi_hurricane,
  // Status bar
  wi_refresh,
  error_icon,
  // Alert icons  
  wi_earthquake,
  wi_fire,
  wi_flood,
  wi_gale_warning,
  wi_hurricane_warning,
  wi_lightning,
  wi_meteor,
  wi_small_craft_advisory,
  wi_smog,
  wi_storm_warning,
  wi_thermometer,
  wi_tsunami,
  warning_icon,
  // Current conditions
  visibility_icon,
  air_filter,
  house_humidity,
  house_thermometer,
  wi_barometer,
  wi_humidity,
  wi_sunrise,
  wi_sunset,
  // Error screens
  battery_alert_0deg,
  wi_cloud_down,
  wi_time_4,
  wifi_x,
} icon_name_t;

// getBitmap function with 196x196 support for main weather icon
inline const unsigned char* getBitmap(icon_name_t icon, size_t size)
{
  switch (icon) {
    case wi_thunderstorm:
      if (size == 196) return wi_thunderstorm_196x196;
      if (size == 64) return wi_thunderstorm_64x64;
      if (size == 48) return wi_thunderstorm_48x48;
      return wi_thunderstorm_32x32;
    case wi_day_thunderstorm:
      if (size == 196) return wi_day_thunderstorm_196x196;
      if (size == 64) return wi_day_thunderstorm_64x64;
      if (size == 48) return wi_day_thunderstorm_48x48;
      return wi_day_thunderstorm_32x32;
    case wi_night_alt_thunderstorm:
      if (size == 196) return wi_night_alt_thunderstorm_196x196;
      if (size == 64) return wi_night_alt_thunderstorm_64x64;
      if (size == 48) return wi_night_alt_thunderstorm_48x48;
      return wi_night_alt_thunderstorm_32x32;
    case wi_storm_showers:
      if (size == 196) return wi_storm_showers_196x196;
      if (size == 64) return wi_storm_showers_64x64;
      if (size == 48) return wi_storm_showers_48x48;
      return wi_storm_showers_32x32;
    case wi_day_storm_showers:
      if (size == 196) return wi_day_storm_showers_196x196;
      if (size == 64) return wi_day_storm_showers_64x64;
      if (size == 48) return wi_day_storm_showers_48x48;
      return wi_day_storm_showers_32x32;
    case wi_night_alt_storm_showers:
      if (size == 196) return wi_night_alt_storm_showers_196x196;
      if (size == 64) return wi_night_alt_storm_showers_64x64;
      if (size == 48) return wi_night_alt_storm_showers_48x48;
      return wi_night_alt_storm_showers_32x32;
    case wi_showers:
      if (size == 196) return wi_showers_196x196;
      if (size == 64) return wi_showers_64x64;
      if (size == 48) return wi_showers_48x48;
      return wi_showers_32x32;
    case wi_day_showers:
      if (size == 196) return wi_day_showers_196x196;
      if (size == 64) return wi_day_showers_64x64;
      if (size == 48) return wi_day_showers_48x48;
      return wi_day_showers_32x32;
    case wi_night_alt_showers:
      if (size == 196) return wi_night_alt_showers_196x196;
      if (size == 64) return wi_night_alt_showers_64x64;
      if (size == 48) return wi_night_alt_showers_48x48;
      return wi_night_alt_showers_32x32;
    case wi_rain:
      if (size == 196) return wi_rain_196x196;
      if (size == 64) return wi_rain_64x64;
      if (size == 48) return wi_rain_48x48;
      return wi_rain_32x32;
    case wi_rain_wind:
      if (size == 196) return wi_rain_wind_196x196;
      if (size == 64) return wi_rain_wind_64x64;
      if (size == 48) return wi_rain_wind_48x48;
      return wi_rain_wind_32x32;
    case wi_day_rain:
      if (size == 196) return wi_day_rain_196x196;
      if (size == 64) return wi_day_rain_64x64;
      if (size == 48) return wi_day_rain_48x48;
      return wi_day_rain_32x32;
    case wi_day_rain_wind:
      if (size == 196) return wi_day_rain_wind_196x196;
      if (size == 64) return wi_day_rain_wind_64x64;
      if (size == 48) return wi_day_rain_wind_48x48;
      return wi_day_rain_wind_32x32;
    case wi_night_alt_rain:
      if (size == 196) return wi_night_alt_rain_196x196;
      if (size == 64) return wi_night_alt_rain_64x64;
      if (size == 48) return wi_night_alt_rain_48x48;
      return wi_night_alt_rain_32x32;
    case wi_night_alt_rain_wind:
      if (size == 196) return wi_night_alt_rain_wind_196x196;
      if (size == 64) return wi_night_alt_rain_wind_64x64;
      if (size == 48) return wi_night_alt_rain_wind_48x48;
      return wi_night_alt_rain_wind_32x32;
    case wi_rain_mix:
      if (size == 196) return wi_rain_mix_196x196;
      if (size == 64) return wi_rain_mix_64x64;
      if (size == 48) return wi_rain_mix_48x48;
      return wi_rain_mix_32x32;
    case wi_day_rain_mix:
      if (size == 196) return wi_day_rain_mix_196x196;
      if (size == 64) return wi_day_rain_mix_64x64;
      if (size == 48) return wi_day_rain_mix_48x48;
      return wi_day_rain_mix_32x32;
    case wi_night_alt_rain_mix:
      if (size == 196) return wi_night_alt_rain_mix_196x196;
      if (size == 64) return wi_night_alt_rain_mix_64x64;
      if (size == 48) return wi_night_alt_rain_mix_48x48;
      return wi_night_alt_rain_mix_32x32;
    case wi_snow:
      if (size == 196) return wi_snow_196x196;
      if (size == 64) return wi_snow_64x64;
      if (size == 48) return wi_snow_48x48;
      return wi_snow_32x32;
    case wi_snow_wind:
      if (size == 196) return wi_snow_wind_196x196;
      if (size == 64) return wi_snow_wind_64x64;
      if (size == 48) return wi_snow_wind_48x48;
      return wi_snow_wind_32x32;
    case wi_day_snow:
      if (size == 196) return wi_day_snow_196x196;
      if (size == 64) return wi_day_snow_64x64;
      if (size == 48) return wi_day_snow_48x48;
      return wi_day_snow_32x32;
    case wi_day_snow_wind:
      if (size == 196) return wi_day_snow_wind_196x196;
      if (size == 64) return wi_day_snow_wind_64x64;
      if (size == 48) return wi_day_snow_wind_48x48;
      return wi_day_snow_wind_32x32;
    case wi_night_alt_snow:
      if (size == 196) return wi_night_alt_snow_196x196;
      if (size == 64) return wi_night_alt_snow_64x64;
      if (size == 48) return wi_night_alt_snow_48x48;
      return wi_night_alt_snow_32x32;
    case wi_night_alt_snow_wind:
      if (size == 196) return wi_night_alt_snow_wind_196x196;
      if (size == 64) return wi_night_alt_snow_wind_64x64;
      if (size == 48) return wi_night_alt_snow_wind_48x48;
      return wi_night_alt_snow_wind_32x32;
    case wi_sleet:
      if (size == 196) return wi_sleet_196x196;
      if (size == 64) return wi_sleet_64x64;
      if (size == 48) return wi_sleet_48x48;
      return wi_sleet_32x32;
    case wi_day_sleet:
      if (size == 196) return wi_day_sleet_196x196;
      if (size == 64) return wi_day_sleet_64x64;
      if (size == 48) return wi_day_sleet_48x48;
      return wi_day_sleet_32x32;
    case wi_night_alt_sleet:
      if (size == 196) return wi_night_alt_sleet_196x196;
      if (size == 64) return wi_night_alt_sleet_64x64;
      if (size == 48) return wi_night_alt_sleet_48x48;
      return wi_night_alt_sleet_32x32;
    case wi_fog:
      if (size == 196) return wi_fog_196x196;
      if (size == 64) return wi_fog_64x64;
      if (size == 48) return wi_fog_48x48;
      return wi_fog_32x32;
    case wi_day_fog:
      if (size == 196) return wi_day_fog_196x196;
      if (size == 64) return wi_day_fog_64x64;
      if (size == 48) return wi_day_fog_48x48;
      return wi_day_fog_32x32;
    case wi_night_fog:
      if (size == 196) return wi_night_fog_196x196;
      if (size == 64) return wi_night_fog_64x64;
      if (size == 48) return wi_night_fog_48x48;
      return wi_night_fog_32x32;
    case wi_smoke:
      if (size == 196) return wi_smoke_196x196;
      if (size == 64) return wi_smoke_64x64;
      if (size == 48) return wi_smoke_48x48;
      return wi_smoke_32x32;
    case wi_dust:
      if (size == 196) return wi_dust_196x196;
      if (size == 64) return wi_dust_64x64;
      if (size == 48) return wi_dust_48x48;
      return wi_dust_32x32;
    case wi_day_haze:
      if (size == 196) return wi_day_haze_196x196;
      if (size == 64) return wi_day_haze_64x64;
      if (size == 48) return wi_day_haze_48x48;
      return wi_day_haze_32x32;
    case wi_sandstorm:
      if (size == 196) return wi_sandstorm_196x196;
      if (size == 64) return wi_sandstorm_64x64;
      if (size == 48) return wi_sandstorm_48x48;
      return wi_sandstorm_32x32;
    case wi_cloudy_gusts:
      if (size == 196) return wi_cloudy_gusts_196x196;
      if (size == 64) return wi_cloudy_gusts_64x64;
      if (size == 48) return wi_cloudy_gusts_48x48;
      return wi_cloudy_gusts_32x32;
    case wi_tornado:
      if (size == 196) return wi_tornado_196x196;
      if (size == 64) return wi_tornado_64x64;
      if (size == 48) return wi_tornado_48x48;
      return wi_tornado_32x32;
    case wi_day_sunny:
      if (size == 196) return wi_day_sunny_196x196;
      if (size == 64) return wi_day_sunny_64x64;
      if (size == 48) return wi_day_sunny_48x48;
      return wi_day_sunny_32x32;
    case wi_night_clear:
      if (size == 196) return wi_night_clear_196x196;
      if (size == 64) return wi_night_clear_64x64;
      if (size == 48) return wi_night_clear_48x48;
      return wi_night_clear_32x32;
    case wi_stars:
      // No 196x196 version - use night_clear
      if (size == 196) return wi_night_clear_196x196;
      if (size == 64) return wi_stars_64x64;
      if (size == 48) return wi_stars_48x48;
      return wi_stars_32x32;
    case wi_day_sunny_overcast:
      // No 196x196 version - use day_cloudy
      if (size == 196) return wi_day_cloudy_196x196;
      if (size == 64) return wi_day_sunny_overcast_64x64;
      if (size == 48) return wi_day_sunny_overcast_48x48;
      return wi_day_sunny_overcast_32x32;
    case wi_night_alt_partly_cloudy:
      // No 196x196 version - use night_alt_cloudy
      if (size == 196) return wi_night_alt_cloudy_196x196;
      if (size == 64) return wi_night_alt_partly_cloudy_64x64;
      if (size == 48) return wi_night_alt_partly_cloudy_48x48;
      return wi_night_alt_partly_cloudy_32x32;
    case wi_day_cloudy:
      if (size == 196) return wi_day_cloudy_196x196;
      if (size == 64) return wi_day_cloudy_64x64;
      if (size == 48) return wi_day_cloudy_48x48;
      return wi_day_cloudy_32x32;
    case wi_day_cloudy_gusts:
      // No 196x196 version - use cloudy_gusts
      if (size == 196) return wi_cloudy_gusts_196x196;
      if (size == 64) return wi_day_cloudy_gusts_64x64;
      if (size == 48) return wi_day_cloudy_gusts_48x48;
      return wi_day_cloudy_gusts_32x32;
    case wi_night_alt_cloudy:
      if (size == 196) return wi_night_alt_cloudy_196x196;
      if (size == 64) return wi_night_alt_cloudy_64x64;
      if (size == 48) return wi_night_alt_cloudy_48x48;
      return wi_night_alt_cloudy_32x32;
    case wi_night_alt_cloudy_gusts:
      // No 196x196 version - use cloudy_gusts
      if (size == 196) return wi_cloudy_gusts_196x196;
      if (size == 64) return wi_night_alt_cloudy_gusts_64x64;
      if (size == 48) return wi_night_alt_cloudy_gusts_48x48;
      return wi_night_alt_cloudy_gusts_32x32;
    case wi_cloud:
      // No 196x196 - use cloudy
      if (size == 196) return wi_cloudy_196x196;
      if (size == 64) return wi_cloud_64x64;
      if (size == 48) return wi_cloud_48x48;
      return wi_cloud_32x32;
    case wi_cloudy:
      if (size == 196) return wi_cloudy_196x196;
      if (size == 64) return wi_cloudy_64x64;
      if (size == 48) return wi_cloudy_48x48;
      return wi_cloudy_32x32;
    case wi_na:
      if (size == 196) return wi_na_196x196;
      if (size == 64) return wi_na_64x64;
      if (size == 48) return wi_na_48x48;
      return wi_na_32x32;
    case wi_strong_wind:
      if (size == 196) return wi_strong_wind_196x196;
      if (size == 64) return wi_strong_wind_64x64;
      if (size == 48) return wi_strong_wind_48x48;
      return wi_strong_wind_32x32;
    case wi_volcano:
      // No 196x196 - use na
      if (size == 196) return wi_na_196x196;
      if (size == 64) return wi_volcano_64x64;
      if (size == 48) return wi_volcano_48x48;
      return wi_volcano_32x32;
    case wi_hot:
      if (size == 196) return wi_hot_196x196;
      // No 48x48 or 32x32 - fallback to day_sunny
      return wi_day_sunny_32x32;
    case wi_snowflake_cold:
      if (size == 196) return wi_snowflake_cold_196x196;
      // No 48x48 or 32x32 - fallback to snow
      return wi_snow_32x32;
    case wi_hurricane:
      if (size == 196) return wi_hurricane_196x196;
      // No 48x48 or 32x32 - fallback to strong_wind
      return wi_strong_wind_32x32;
    default:
      return nullptr;
  }
}

#endif
