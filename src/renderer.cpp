/* Renderer for esp32-weather-epd - Modified for TRMNL OG
 * Original Copyright (C) 2022-2025 Luke Marzen
 * TRMNL OG modifications for display margins
 */

#include "renderer.h"
#include "_locale.h"
#include "_strftime.h"
#include "api_response.h"
#include "config.h"
#include "conversions.h"
#include "display_utils.h"
#include <SPI.h>

// fonts
#include FONT_HEADER

// icon header files (minimal set for TRMNL OG)
#include "icons/icons_minimal_16x16.h"
#include "icons/icons_minimal_196x196.h"
#include "icons/icons_minimal_24x24.h"
#include "icons/icons_minimal_32x32.h"
#include "icons/icons_minimal_48x48.h"
#include "icons/icons_minimal_64x64.h"

#ifdef DISP_BW_V2
GxEPD2_BW<GxEPD2_750_T7, GxEPD2_750_T7::HEIGHT>
    display(GxEPD2_750_T7(PIN_EPD_CS, PIN_EPD_DC, PIN_EPD_RST, PIN_EPD_BUSY));
#endif
#ifdef DISP_3C_B
GxEPD2_3C<GxEPD2_750c_Z08, GxEPD2_750c_Z08::HEIGHT / 2>
    display(GxEPD2_750c_Z08(PIN_EPD_CS, PIN_EPD_DC, PIN_EPD_RST, PIN_EPD_BUSY));
#endif
#ifdef DISP_7C_F
GxEPD2_7C<GxEPD2_730c_GDEY073D46, GxEPD2_730c_GDEY073D46::HEIGHT / 4> display(
    GxEPD2_730c_GDEY073D46(PIN_EPD_CS, PIN_EPD_DC, PIN_EPD_RST, PIN_EPD_BUSY));
#endif
#ifdef DISP_BW_V1
GxEPD2_BW<GxEPD2_750, GxEPD2_750::HEIGHT>
    display(GxEPD2_750(PIN_EPD_CS, PIN_EPD_DC, PIN_EPD_RST, PIN_EPD_BUSY));
#endif

#ifndef ACCENT_COLOR
#define ACCENT_COLOR GxEPD_BLACK
#endif

// =============================================================================
// TRMNL OG MARGIN SYSTEM
// The physical frame covers edge pixels. We apply offsets to create margins.
// =============================================================================
#define MARGIN_X 20 // Left/right margin
#define MARGIN_Y 12 // Top/bottom margin

// Effective display area after margins
#define EFF_WIDTH (DISP_WIDTH - 2 * MARGIN_X)
#define EFF_HEIGHT (DISP_HEIGHT - 2 * MARGIN_Y)

/* Returns the string width in pixels */
uint16_t getStringWidth(const String &text) {
  int16_t x1, y1;
  uint16_t w, h;
  display.getTextBounds(text, 0, 0, &x1, &y1, &w, &h);
  return w;
}

/* Returns the string height in pixels */
uint16_t getStringHeight(const String &text) {
  int16_t x1, y1;
  uint16_t w, h;
  display.getTextBounds(text, 0, 0, &x1, &y1, &w, &h);
  return h;
}

/* Draws a string with alignment - applies MARGIN offsets */
void drawString(int16_t x, int16_t y, const String &text, alignment_t alignment,
                uint16_t color) {
  // Apply margin offsets
  x += MARGIN_X;
  y += MARGIN_Y;

  int16_t x1, y1;
  uint16_t w, h;
  display.setTextColor(color);
  display.getTextBounds(text, x, y, &x1, &y1, &w, &h);
  if (alignment == RIGHT) {
    x = x - w;
  }
  if (alignment == CENTER) {
    x = x - w / 2;
  }
  display.setCursor(x, y);
  display.print(text);
  return;
}

/* Helper to draw bitmap with margin offsets */
void drawBmp(int16_t x, int16_t y, const uint8_t *bitmap, int16_t w, int16_t h,
             uint16_t color) {
  display.drawInvertedBitmap(x + MARGIN_X, y + MARGIN_Y, bitmap, w, h, color);
}

/* Draws a string that will flow into the next line when max_width is reached.
 */
void drawMultiLnString(int16_t x, int16_t y, const String &text,
                       alignment_t alignment, uint16_t max_width,
                       uint16_t max_lines, int16_t line_spacing,
                       uint16_t color) {
  uint16_t current_line = 0;
  String textRemaining = text;
  while (current_line < max_lines && !textRemaining.isEmpty()) {
    int16_t x1, y1;
    uint16_t w, h;

    display.getTextBounds(textRemaining, 0, 0, &x1, &y1, &w, &h);

    int endIndex = textRemaining.length();
    String subStr = textRemaining;
    int splitAt = 0;
    int keepLastChar = 0;
    while (w > max_width && splitAt != -1) {
      if (keepLastChar) {
        subStr.remove(subStr.length() - 1);
      }

      if (current_line < max_lines - 1) {
        splitAt = std::max(subStr.lastIndexOf(" "), subStr.lastIndexOf("-"));
      } else {
        splitAt = subStr.lastIndexOf(" ");
      }

      if (splitAt != -1) {
        endIndex = splitAt;
        subStr = subStr.substring(0, endIndex + 1);

        char lastChar = subStr.charAt(endIndex);
        if (lastChar == ' ') {
          keepLastChar = 0;
          subStr.remove(endIndex);
          --endIndex;
        } else if (lastChar == '-') {
          keepLastChar = 1;
        }

        if (current_line < max_lines - 1) {
          display.getTextBounds(subStr, 0, 0, &x1, &y1, &w, &h);
        } else {
          display.getTextBounds(subStr + "...", 0, 0, &x1, &y1, &w, &h);
          if (w <= max_width) {
            subStr = subStr + "...";
          }
        }
      }
    }

    drawString(x, y + (current_line * line_spacing), subStr, alignment, color);
    textRemaining = textRemaining.substring(endIndex + 2 - keepLastChar);
    ++current_line;
  }
  return;
}

/* Initialize e-paper display - TRMNL OG specific */
void initDisplay() {
  // Power on display (if applicable)
  if (PIN_EPD_PWR != 255 && PIN_EPD_PWR != (uint8_t)-1) {
    pinMode(PIN_EPD_PWR, OUTPUT);
    digitalWrite(PIN_EPD_PWR, HIGH);
    delay(10);
  }

  // Reset sequence
  pinMode(PIN_EPD_RST, OUTPUT);
  digitalWrite(PIN_EPD_RST, LOW);
  delay(100);
  digitalWrite(PIN_EPD_RST, HIGH);
  delay(100);

  // Initialize SPI for ESP32-C3
  SPI.end();
  SPI.begin(PIN_EPD_SCK, PIN_EPD_MISO, PIN_EPD_MOSI, -1);

#ifdef DRIVER_WAVESHARE
  display.init(115200, true, 2, true, SPI,
               SPISettings(8000000, MSBFIRST, SPI_MODE0));
#endif
#ifdef DRIVER_DESPI_C02
  display.init(115200, true, 10, true, SPI,
               SPISettings(8000000, MSBFIRST, SPI_MODE0));
#endif

  display.setRotation(0);
  display.setTextSize(1);
  display.setTextColor(GxEPD_BLACK);
  display.setTextWrap(false);
  display.setFullWindow();
  display.firstPage();
  return;
}

/* Power-off e-paper display */
void powerOffDisplay() {
  display.hibernate();
  if (PIN_EPD_PWR != 255 && PIN_EPD_PWR != (uint8_t)-1) {
    digitalWrite(PIN_EPD_PWR, LOW);
  }
  return;
}

/* Draw current conditions */
void drawCurrentConditions(const owm_current_t &current,
                           const owm_daily_t &today,
                           const owm_resp_air_pollution_t &owm_air_pollution,
                           bool airPollutionSuccess, float inTemp,
                           float inHumidity) {
  String dataStr, unitStr;

  // current weather icon
  drawBmp(0, 0, getCurrentConditionsBitmap196(current, today), 196, 196,
          GxEPD_BLACK);

  // current temp
#ifdef UNITS_TEMP_KELVIN
  dataStr = String(static_cast<int>(std::round(current.temp)));
  unitStr = TXT_UNITS_TEMP_KELVIN;
#endif
#ifdef UNITS_TEMP_CELSIUS
  dataStr =
      String(static_cast<int>(std::round(kelvin_to_celsius(current.temp))));
  unitStr = TXT_UNITS_TEMP_CELSIUS;
#endif
#ifdef UNITS_TEMP_FAHRENHEIT
  dataStr =
      String(static_cast<int>(std::round(kelvin_to_fahrenheit(current.temp))));
  unitStr = TXT_UNITS_TEMP_FAHRENHEIT;
#endif
  display.setFont(&FONT_48pt8b_temperature);
#ifndef DISP_BW_V1
  drawString(196 + 164 / 2 - 20, 196 / 2 + 69 / 2, dataStr, CENTER);
#else
  drawString(156 + 164 / 2 - 20, 196 / 2 + 69 / 2, dataStr, CENTER);
#endif
  display.setFont(&FONT_14pt8b);
  drawString(display.getCursorX() - MARGIN_X, 196 / 2 - 69 / 2 + 20, unitStr,
             LEFT);

  // current feels like
#ifdef UNITS_TEMP_KELVIN
  dataStr = String(TXT_FEELS_LIKE) + ' ' +
            String(static_cast<int>(std::round(current.feels_like)));
#endif
#ifdef UNITS_TEMP_CELSIUS
  dataStr = String(TXT_FEELS_LIKE) + ' ' +
            String(static_cast<int>(
                std::round(kelvin_to_celsius(current.feels_like)))) +
            '\260';
#endif
#ifdef UNITS_TEMP_FAHRENHEIT
  dataStr = String(TXT_FEELS_LIKE) + ' ' +
            String(static_cast<int>(
                std::round(kelvin_to_fahrenheit(current.feels_like)))) +
            '\260';
#endif
  display.setFont(&FONT_12pt8b);
#ifndef DISP_BW_V1
  drawString(196 + 164 / 2, 98 + 69 / 2 + 12 + 17, dataStr, CENTER);
#else
  drawString(156 + 164 / 2, 98 + 69 / 2 + 12 + 17, dataStr, CENTER);
#endif

  // current weather data icons (moved up 17px total)
  const int lowerY = 187; // was 204, moved up 17px
  drawBmp(0, lowerY + (48 + 8) * 0, wi_sunrise_48x48, 48, 48, GxEPD_BLACK);
  drawBmp(0, lowerY + (48 + 8) * 1, wi_strong_wind_48x48, 48, 48, GxEPD_BLACK);
  drawBmp(0, lowerY + (48 + 8) * 2, wi_day_sunny_48x48, 48, 48, GxEPD_BLACK);
  drawBmp(170, lowerY + (48 + 8) * 0, wi_sunset_48x48, 48, 48, GxEPD_BLACK);
  drawBmp(170, lowerY + (48 + 8) * 1, wi_humidity_48x48, 48, 48, GxEPD_BLACK);
  drawBmp(170, lowerY + (48 + 8) * 2, wi_barometer_48x48, 48, 48, GxEPD_BLACK);

  // current weather data labels
  display.setFont(&FONT_7pt8b);
  drawString(48, lowerY + 10 + (48 + 8) * 0, TXT_SUNRISE, LEFT);
  drawString(48, lowerY + 10 + (48 + 8) * 1, TXT_WIND, LEFT);
  drawString(48, lowerY + 10 + (48 + 8) * 2, TXT_UV_INDEX, LEFT);
  drawString(170 + 48, lowerY + 10 + (48 + 8) * 0, TXT_SUNSET, LEFT);
  drawString(170 + 48, lowerY + 10 + (48 + 8) * 1, TXT_HUMIDITY, LEFT);
  drawString(170 + 48, lowerY + 10 + (48 + 8) * 2, TXT_PRESSURE, LEFT);

  // sunrise
  display.setFont(&FONT_12pt8b);
  char timeBuffer[12] = {};
  time_t ts = current.sunrise;
  tm *timeInfo = localtime(&ts);
  _strftime(timeBuffer, sizeof(timeBuffer), TIME_FORMAT, timeInfo);
  drawString(48, lowerY + 17 / 2 + (48 + 8) * 0 + 48 / 2, timeBuffer, LEFT);

  // wind
#ifdef WIND_INDICATOR_ARROW
  drawBmp(48, lowerY + 24 / 2 + (48 + 8) * 1, getWindBitmap24(current.wind_deg),
          24, 24, GxEPD_BLACK);
#endif
#ifdef UNITS_SPEED_METERSPERSECOND
  dataStr = String(static_cast<int>(std::round(current.wind_speed)));
  unitStr = String(" ") + TXT_UNITS_SPEED_METERSPERSECOND;
#endif
#ifdef UNITS_SPEED_MILESPERHOUR
  dataStr = String(static_cast<int>(
      std::round(meterspersecond_to_milesperhour(current.wind_speed))));
  unitStr = String(" ") + TXT_UNITS_SPEED_MILESPERHOUR;
#endif
#ifdef UNITS_SPEED_KILOMETERSPERHOUR
  dataStr = String(static_cast<int>(
      std::round(meterspersecond_to_kilometersperhour(current.wind_speed))));
  unitStr = String(" ") + TXT_UNITS_SPEED_KILOMETERSPERHOUR;
#endif
#ifdef UNITS_SPEED_KNOTS
  dataStr = String(static_cast<int>(
      std::round(meterspersecond_to_knots(current.wind_speed))));
  unitStr = String(" ") + TXT_UNITS_SPEED_KNOTS;
#endif
#ifdef UNITS_SPEED_FEETPERSECOND
  dataStr = String(static_cast<int>(
      std::round(meterspersecond_to_feetpersecond(current.wind_speed))));
  unitStr = String(" ") + TXT_UNITS_SPEED_FEETPERSECOND;
#endif
#ifdef UNITS_SPEED_BEAUFORT
  dataStr = String(meterspersecond_to_beaufort(current.wind_speed));
  unitStr = String(" ") + TXT_UNITS_SPEED_BEAUFORT;
#endif

#ifdef WIND_INDICATOR_ARROW
  drawString(48 + 24, lowerY + 17 / 2 + (48 + 8) * 1 + 48 / 2, dataStr, LEFT);
#else
  drawString(48, lowerY + 17 / 2 + (48 + 8) * 1 + 48 / 2, dataStr, LEFT);
#endif
  display.setFont(&FONT_8pt8b);
  drawString(display.getCursorX() - MARGIN_X,
             lowerY + 17 / 2 + (48 + 8) * 1 + 48 / 2, unitStr, LEFT);

  // uv index
  const int sp = 8;
  display.setFont(&FONT_12pt8b);
  unsigned int uvi =
      static_cast<unsigned int>(std::max(std::round(current.uvi), 0.0f));
  dataStr = String(uvi);
  drawString(48, lowerY + 17 / 2 + (48 + 8) * 2 + 48 / 2, dataStr, LEFT);
  display.setFont(&FONT_7pt8b);
  dataStr = String(getUVIdesc(uvi));
  int max_w = 170 - (display.getCursorX() - MARGIN_X + sp);
  if (getStringWidth(dataStr) <= max_w) {
    drawString(display.getCursorX() - MARGIN_X + sp,
               lowerY + 17 / 2 + (48 + 8) * 2 + 48 / 2, dataStr, LEFT);
  } else {
    display.setFont(&FONT_5pt8b);
    if (getStringWidth(dataStr) <= max_w) {
      drawString(display.getCursorX() - MARGIN_X + sp,
                 lowerY + 17 / 2 + (48 + 8) * 2 + 48 / 2, dataStr, LEFT);
    } else {
      drawMultiLnString(display.getCursorX() - MARGIN_X + sp,
                        lowerY + 17 / 2 + (48 + 8) * 2 + 48 / 2 - 10, dataStr,
                        LEFT, max_w, 2, 10);
    }
  }

  // sunset
  memset(timeBuffer, '\0', sizeof(timeBuffer));
  ts = current.sunset;
  timeInfo = localtime(&ts);
  _strftime(timeBuffer, sizeof(timeBuffer), TIME_FORMAT, timeInfo);
  drawString(170 + 48, lowerY + 17 / 2 + (48 + 8) * 0 + 48 / 2, timeBuffer,
             LEFT);

  // humidity
  dataStr = String(current.humidity);
  drawString(170 + 48, lowerY + 17 / 2 + (48 + 8) * 1 + 48 / 2, dataStr, LEFT);
  display.setFont(&FONT_8pt8b);
  drawString(display.getCursorX() - MARGIN_X,
             lowerY + 17 / 2 + (48 + 8) * 1 + 48 / 2, "%", LEFT);

  // pressure
#ifdef UNITS_PRES_HECTOPASCALS
  dataStr = String(current.pressure);
  unitStr = String(" ") + TXT_UNITS_PRES_HECTOPASCALS;
#endif
#ifdef UNITS_PRES_MILLIBARS
  dataStr = String(static_cast<int>(
      std::round(hectopascals_to_millibars(current.pressure))));
  unitStr = String(" ") + TXT_UNITS_PRES_MILLIBARS;
#endif
#ifdef UNITS_PRES_INCHESOFMERCURY
  dataStr = String(
      std::round(1e1f * hectopascals_to_inchesofmercury(current.pressure)) /
          1e1f,
      1);
  unitStr = String(" ") + TXT_UNITS_PRES_INCHESOFMERCURY;
#endif
  display.setFont(&FONT_12pt8b);
  drawString(170 + 48, lowerY + 17 / 2 + (48 + 8) * 2 + 48 / 2, dataStr, LEFT);
  display.setFont(&FONT_8pt8b);
  drawString(display.getCursorX() - MARGIN_X,
             lowerY + 17 / 2 + (48 + 8) * 2 + 48 / 2, unitStr, LEFT);

  return;
}

/* Draw 5-day forecast */
void drawForecast(const owm_daily_t *daily, tm timeInfo) {
  String hiStr, loStr;
  String dataStr, unitStr;
  for (int i = 0; i < 5; ++i) {
#ifndef DISP_BW_V1
    int x = 381 + (i * 82); // was 398, moved left 17px total
#else
    int x = 301 + (i * 64); // was 318, moved left 17px total
#endif
    drawBmp(x, 98 + 69 / 2 - 32 - 6, getDailyForecastBitmap64(daily[i]), 64, 64,
            GxEPD_BLACK);

    display.setFont(&FONT_11pt8b);
    char dayBuffer[8] = {};
    _strftime(dayBuffer, sizeof(dayBuffer), "%a", &timeInfo);
    drawString(x + 31 - 2, 98 + 69 / 2 - 32 - 26 - 6 + 16, dayBuffer, CENTER);
    timeInfo.tm_wday = (timeInfo.tm_wday + 1) % 7;

    display.setFont(&FONT_8pt8b);
    drawString(x + 31, 98 + 69 / 2 + 38 - 6 + 12, "|", CENTER);
#ifdef UNITS_TEMP_CELSIUS
    hiStr = String(static_cast<int>(
                std::round(kelvin_to_celsius(daily[i].temp.max)))) +
            "\260";
    loStr = String(static_cast<int>(
                std::round(kelvin_to_celsius(daily[i].temp.min)))) +
            "\260";
#endif
#ifdef UNITS_TEMP_FAHRENHEIT
    hiStr = String(static_cast<int>(
                std::round(kelvin_to_fahrenheit(daily[i].temp.max)))) +
            "\260";
    loStr = String(static_cast<int>(
                std::round(kelvin_to_fahrenheit(daily[i].temp.min)))) +
            "\260";
#endif
#ifdef UNITS_TEMP_KELVIN
    hiStr = String(static_cast<int>(std::round(daily[i].temp.max)));
    loStr = String(static_cast<int>(std::round(daily[i].temp.min)));
#endif
    drawString(x + 31 - 4, 98 + 69 / 2 + 38 - 6 + 12, hiStr, RIGHT);
    drawString(x + 31 + 5, 98 + 69 / 2 + 38 - 6 + 12, loStr, LEFT);

    // Show rain in mm (only if there's rain)
    float dailyRain =
        daily[i].rain + daily[i].snow; // Total precipitation in mm
    if (dailyRain >= 0.5f) {
      // Show rain amount if >= 0.5mm
      display.setFont(&FONT_6pt8b);
      dataStr = String(static_cast<int>(std::round(dailyRain))) + "mm";
      drawString(x + 31, 98 + 69 / 2 + 38 - 6 + 28, dataStr,
                 CENTER); // +2px down
    } else if (dailyRain > 0.0f) {
      // Show <1mm for trace amounts
      display.setFont(&FONT_6pt8b);
      dataStr = "<1mm";
      drawString(x + 31, 98 + 69 / 2 + 38 - 6 + 28, dataStr,
                 CENTER); // +2px down
    }
    // If 0mm, show nothing (blank)
  }
  return;
}

/* Draw alerts */
void drawAlerts(std::vector<owm_alerts_t> &alerts, const String &city,
                const String &date) {
#if DEBUG_LEVEL >= 1
  Serial.println("[debug] alerts.size()    : " + String(alerts.size()));
#endif
  if (alerts.size() == 0)
    return;

  int *ignore_list = (int *)calloc(alerts.size(), sizeof(*ignore_list));
  int *alert_indices = (int *)calloc(alerts.size(), sizeof(*alert_indices));
  if (!ignore_list || !alert_indices) {
    Serial.println("Error: Failed to allocate memory for alerts.");
    free(ignore_list);
    free(alert_indices);
    return;
  }

  filterAlerts(alerts, ignore_list);

  display.setFont(&FONT_16pt8b);
  int city_w = getStringWidth(city);
  display.setFont(&FONT_12pt8b);
  int date_w = getStringWidth(date);
  int max_w = EFF_WIDTH - 2 - std::max(city_w, date_w) - (196 + 4) - 8;

  int num_valid_alerts = 0;
  for (int i = 0; i < alerts.size(); ++i) {
    if (!ignore_list[i]) {
      alert_indices[num_valid_alerts] = i;
      ++num_valid_alerts;
    }
  }

  if (num_valid_alerts == 1) {
    max_w -= 48;
    owm_alerts_t &cur_alert = alerts[alert_indices[0]];
    drawBmp(196, 8, getAlertBitmap48(cur_alert), 48, 48, ACCENT_COLOR);
    toTitleCase(cur_alert.event);
    display.setFont(&FONT_14pt8b);
    if (getStringWidth(cur_alert.event) <= max_w) {
      drawString(196 + 48 + 4, 24 + 8 - 12 + 20 + 1, cur_alert.event, LEFT);
    } else {
      display.setFont(&FONT_12pt8b);
      if (getStringWidth(cur_alert.event) <= max_w) {
        drawString(196 + 48 + 4, 24 + 8 - 12 + 17 + 1, cur_alert.event, LEFT);
      } else {
        drawMultiLnString(196 + 48 + 4, 24 + 8 - 12 + 17 - 11, cur_alert.event,
                          LEFT, max_w, 2, 23);
      }
    }
  } else if (num_valid_alerts >= 2) {
    max_w -= 32;
    display.setFont(&FONT_12pt8b);
    for (int i = 0; i < 2; ++i) {
      owm_alerts_t &cur_alert = alerts[alert_indices[i]];
      drawBmp(196, (i * 32), getAlertBitmap32(cur_alert), 32, 32, ACCENT_COLOR);
      toTitleCase(cur_alert.event);
      drawMultiLnString(196 + 32 + 3, 5 + 17 + (i * 32), cur_alert.event, LEFT,
                        max_w, 1, 0);
    }
  }

  free(ignore_list);
  free(alert_indices);
  return;
}

/* Draw location and date - positioned relative to right edge */
void drawLocationDate(const String &city, const String &date) {
  // These are positioned from the right edge, need to account for right margin
  display.setFont(&FONT_16pt8b);
  drawString(EFF_WIDTH - 2, 23, city, RIGHT, ACCENT_COLOR);
  display.setFont(&FONT_12pt8b);
  drawString(EFF_WIDTH - 2, 30 + 4 + 17, date, RIGHT);
  return;
}

/* Modulo that works for negatives */
inline int modulo(int a, int b) {
  const int result = a % b;
  return result >= 0 ? result : result + b;
}

/* Convert temp to y coordinate */
int kelvin_to_plot_y(float kelvin, int tempBoundMin, float yPxPerUnit,
                     int yBoundMin) {
#ifdef UNITS_TEMP_KELVIN
  return static_cast<int>(
      std::round(yBoundMin - (yPxPerUnit * (kelvin - tempBoundMin))));
#endif
#ifdef UNITS_TEMP_CELSIUS
  return static_cast<int>(std::round(
      yBoundMin - (yPxPerUnit * (kelvin_to_celsius(kelvin) - tempBoundMin))));
#endif
#ifdef UNITS_TEMP_FAHRENHEIT
  return static_cast<int>(
      std::round(yBoundMin -
                 (yPxPerUnit * (kelvin_to_fahrenheit(kelvin) - tempBoundMin))));
#endif
}

/* Draw outlook graph */
void drawOutlookGraph(const owm_hourly_t *hourly, const owm_daily_t *daily,
                      tm timeInfo) {
  const int xPos0 = 350;
  int xPos1 = EFF_WIDTH;
  const int yPos0 = 216;
  const int yPos1 = EFF_HEIGHT - 46;

  // Calculate bounds
  int yMajorTicks = 5;
#ifdef UNITS_TEMP_CELSIUS
  float tempMin = kelvin_to_celsius(hourly[0].temp);
#endif
#ifdef UNITS_TEMP_FAHRENHEIT
  float tempMin = kelvin_to_fahrenheit(hourly[0].temp);
#endif
#ifdef UNITS_TEMP_KELVIN
  float tempMin = hourly[0].temp;
#endif
  float tempMax = tempMin;
#ifdef UNITS_HOURLY_PRECIP_POP
  float precipMax = hourly[0].pop;
#else
  float precipMax = hourly[0].rain_1h + hourly[0].snow_1h;
#endif
  int yTempMajorTicks = 5;
  float newTemp = 0;

  Serial.println("Calculating graph bounds...");
  for (int i = 1; i < HOURLY_GRAPH_MAX; ++i) {
#ifdef UNITS_TEMP_CELSIUS
    newTemp = kelvin_to_celsius(hourly[i].temp);
#endif
#ifdef UNITS_TEMP_FAHRENHEIT
    newTemp = kelvin_to_fahrenheit(hourly[i].temp);
#endif
#ifdef UNITS_TEMP_KELVIN
    newTemp = hourly[i].temp;
#endif
    tempMin = std::min(tempMin, newTemp);
    tempMax = std::max(tempMax, newTemp);
#ifdef UNITS_HOURLY_PRECIP_POP
    precipMax = std::max<float>(precipMax, hourly[i].pop);
#else
    precipMax =
        std::max<float>(precipMax, hourly[i].rain_1h + hourly[i].snow_1h);
#endif
  }
  Serial.printf("tempMin: %.1f, tempMax: %.1f, precipMax: %.1f\n", tempMin,
                tempMax, precipMax);

  int tempBoundMin = static_cast<int>(tempMin - 1) -
                     modulo(static_cast<int>(tempMin - 1), yTempMajorTicks);
  int tempBoundMax = static_cast<int>(tempMax + 1) +
                     (yTempMajorTicks -
                      modulo(static_cast<int>(tempMax + 1), yTempMajorTicks));

  Serial.println("Adjusting major ticks (loop 1)...");
  while ((tempBoundMax - tempBoundMin) / yTempMajorTicks > yMajorTicks) {
    yTempMajorTicks += 5;
    tempBoundMin = static_cast<int>(tempMin - 1) -
                   modulo(static_cast<int>(tempMin - 1), yTempMajorTicks);
    tempBoundMax = static_cast<int>(tempMax + 1) +
                   (yTempMajorTicks -
                    modulo(static_cast<int>(tempMax + 1), yTempMajorTicks));
    if (yTempMajorTicks > 200)
      break; // Emergency break
  }
  Serial.println("Adjusting major ticks (loop 2)...");
  while ((tempBoundMax - tempBoundMin) / yTempMajorTicks < yMajorTicks) {
    if (tempMin - tempBoundMin <= tempBoundMax - tempMax)
      tempBoundMin -= yTempMajorTicks;
    else
      tempBoundMax += yTempMajorTicks;
    if (tempBoundMax - tempBoundMin > 1000)
      break; // Emergency break
  }
  Serial.println("Loop 2 complete.");

#ifdef UNITS_HOURLY_PRECIP_POP
  xPos1 = EFF_WIDTH - 23;
  float precipBoundMax = (precipMax > 0) ? 100.0f : 0.0f;
#else
  xPos1 = EFF_WIDTH - 24;
  float precipBoundMax = std::ceil(precipMax);
#endif

  if (precipBoundMax > 0)
    xPos1 -= 23;

  // Draw axes with offset
  display.drawLine(xPos0 + MARGIN_X, yPos1 + MARGIN_Y, xPos1 + MARGIN_X,
                   yPos1 + MARGIN_Y, GxEPD_BLACK);
  display.drawLine(xPos0 + MARGIN_X, yPos1 - 1 + MARGIN_Y, xPos1 + MARGIN_X,
                   yPos1 - 1 + MARGIN_Y, GxEPD_BLACK);

  float yInterval = (yPos1 - yPos0) / static_cast<float>(yMajorTicks);
  for (int i = 0; i <= yMajorTicks; ++i) {
    String dataStr;
    int yTick = static_cast<int>(yPos0 + (i * yInterval));
    display.setFont(&FONT_8pt8b);
    dataStr = String(tempBoundMax - (i * yTempMajorTicks));
#if defined(UNITS_TEMP_CELSIUS) || defined(UNITS_TEMP_FAHRENHEIT)
    dataStr += "\260";
#endif
    drawString(xPos0 - 8, yTick + 4, dataStr, RIGHT, ACCENT_COLOR);

    if (precipBoundMax > 0) {
#ifdef UNITS_HOURLY_PRECIP_POP
      dataStr = String(100 - (i * 20));
      String precipUnit = "%";
#else
      dataStr = String(static_cast<int>(precipBoundMax -
                                        (i * precipBoundMax / yMajorTicks)));
      String precipUnit = " mm";
#endif
      drawString(xPos1 + 8, yTick + 4, dataStr, LEFT);
      display.setFont(&FONT_5pt8b);
      drawString(display.getCursorX() - MARGIN_X, yTick + 4, precipUnit, LEFT);
    }

    if (i < yMajorTicks) {
      for (int x = xPos0 + MARGIN_X; x <= xPos1 + 1 + MARGIN_X; x += 3) {
        display.drawPixel(x, yTick + (yTick % 2) + MARGIN_Y, GxEPD_BLACK);
      }
    }
  }

  int xMaxTicks = 8;
  int hourInterval =
      static_cast<int>(ceil(HOURLY_GRAPH_MAX / static_cast<float>(xMaxTicks)));
  float xInterval = (xPos1 - xPos0 - 1) / static_cast<float>(HOURLY_GRAPH_MAX);
  display.setFont(&FONT_8pt8b);

  float yPxPerUnit =
      (yPos1 - yPos0) / static_cast<float>(tempBoundMax - tempBoundMin);
  std::vector<int> x_t(HOURLY_GRAPH_MAX);
  std::vector<int> y_t(HOURLY_GRAPH_MAX);

  for (int i = 0; i < HOURLY_GRAPH_MAX; ++i) {
    y_t[i] = kelvin_to_plot_y(hourly[i].temp, tempBoundMin, yPxPerUnit, yPos1);
    x_t[i] = static_cast<int>(
        std::round(xPos0 + (i * xInterval) + (0.5 * xInterval)));
  }

  for (int i = 0; i < HOURLY_GRAPH_MAX; ++i) {
    int xTick = static_cast<int>(xPos0 + (i * xInterval));

    if (i > 0) {
      int x0_t = x_t[i - 1] + MARGIN_X;
      int x1_t = x_t[i] + MARGIN_X;
      int y0_t = y_t[i - 1] + MARGIN_Y;
      int y1_t = y_t[i] + MARGIN_Y;
      display.drawLine(x0_t, y0_t, x1_t, y1_t, ACCENT_COLOR);
      display.drawLine(x0_t, y0_t + 1, x1_t, y1_t + 1, ACCENT_COLOR);
      display.drawLine(x0_t - 1, y0_t, x1_t - 1, y1_t, ACCENT_COLOR);
    }

#ifdef UNITS_HOURLY_PRECIP_POP
    float precipVal = hourly[i].pop * 100;
#else
    float precipVal = hourly[i].rain_1h + hourly[i].snow_1h;
#endif
    int x0_p =
        static_cast<int>(std::round(xPos0 + 1 + (i * xInterval))) + MARGIN_X;
    int x1_p = static_cast<int>(std::round(xPos0 + 1 + ((i + 1) * xInterval))) +
               MARGIN_X;
    float yPxPerPrecip =
        (precipBoundMax > 0) ? (yPos1 - yPos0) / precipBoundMax : 0;
    int y0_p =
        static_cast<int>(std::round(yPos1 - (yPxPerPrecip * precipVal))) +
        MARGIN_Y;
    int y1_p = yPos1 + MARGIN_Y;

    // Only draw precipitation bars if there's precipitation to show
    if (precipBoundMax > 0 && precipVal > 0) {
      for (int y = y1_p - 1; y > y0_p; y -= 2) {
        for (int x = x0_p + (x0_p % 2); x < x1_p; x += 2) {
          display.drawPixel(x, y, GxEPD_BLACK);
        }
      }
    }

    if ((i % hourInterval) == 0) {
      display.drawLine(xTick + MARGIN_X, yPos1 + 1 + MARGIN_Y, xTick + MARGIN_X,
                       yPos1 + 4 + MARGIN_Y, GxEPD_BLACK);
      display.drawLine(xTick + 1 + MARGIN_X, yPos1 + 1 + MARGIN_Y,
                       xTick + 1 + MARGIN_X, yPos1 + 4 + MARGIN_Y, GxEPD_BLACK);
      char timeBuffer[12] = {};
      time_t ts = hourly[i].dt;
      tm *ti = localtime(&ts);
      _strftime(timeBuffer, sizeof(timeBuffer), HOUR_FORMAT, ti);
      drawString(xTick, yPos1 + 1 + 12 + 4 + 3, timeBuffer, CENTER);
    }
  }
  return;
}

/* Draw status bar - positioned relative to bottom edge */
void drawStatusBar(const String &statusStr, const String &refreshTimeStr,
                   int rssi, uint32_t batVoltage) {
  String dataStr;
  uint16_t dataColor = GxEPD_BLACK;
  display.setFont(&FONT_6pt8b);
  int pos = EFF_WIDTH - 2;
  const int sp = 2;
  const int yBottom = EFF_HEIGHT - 1;

#if BATTERY_MONITORING
  uint32_t batPercent =
      calcBatPercent(batVoltage, MIN_BATTERY_VOLTAGE, MAX_BATTERY_VOLTAGE);
#if defined(DISP_3C_B) || defined(DISP_7C_F)
  if (batVoltage < WARN_BATTERY_VOLTAGE)
    dataColor = ACCENT_COLOR;
#endif
#if STATUS_BAR_EXTRAS_BAT_PERCENTAGE || STATUS_BAR_EXTRAS_BAT_VOLTAGE
  dataStr = "";
#if STATUS_BAR_EXTRAS_BAT_PERCENTAGE
  dataStr += String(batPercent) + "%";
#endif
#if STATUS_BAR_EXTRAS_BAT_VOLTAGE
  dataStr += " (" + String(std::round(batVoltage / 10.f) / 100.f, 2) + "v)";
#endif
  drawString(pos, yBottom - 2, dataStr, RIGHT, dataColor);
  pos -= getStringWidth(dataStr) + 1;
#endif
  pos -= 24;
  drawBmp(pos, yBottom - 17, getBatBitmap24(batPercent), 24, 24, dataColor);
  pos -= sp + 9;
#endif

  dataColor = rssi >= -70 ? GxEPD_BLACK : ACCENT_COLOR;
#if STATUS_BAR_EXTRAS_WIFI_STRENGTH || STATUS_BAR_EXTRAS_WIFI_RSSI
  dataStr = "";
#if STATUS_BAR_EXTRAS_WIFI_STRENGTH
  dataStr += String(getWiFidesc(rssi));
#endif
#if STATUS_BAR_EXTRAS_WIFI_RSSI
  if (rssi != 0)
    dataStr += " (" + String(rssi) + "dBm)";
#endif
  drawString(pos, yBottom - 2, dataStr, RIGHT, dataColor);
  pos -= getStringWidth(dataStr) + 1;
#endif
  pos -= 18;
  drawBmp(pos, yBottom - 13, getWiFiBitmap16(rssi), 16, 16, dataColor);
  pos -= sp + 8;

  dataColor = GxEPD_BLACK;
  drawString(pos, yBottom - 2, refreshTimeStr, RIGHT, dataColor);
  pos -= getStringWidth(refreshTimeStr) + 25;
  drawBmp(pos, yBottom - 21, wi_refresh_32x32, 32, 32, dataColor);
  pos -= sp;

  dataColor = ACCENT_COLOR;
  if (!statusStr.isEmpty()) {
    drawString(pos, yBottom - 2, statusStr, RIGHT, dataColor);
    pos -= getStringWidth(statusStr) + 24;
    drawBmp(pos, yBottom - 18, error_icon_24x24, 24, 24, dataColor);
  }
  return;
}

/* Draw error screen */
void drawError(const uint8_t *bitmap_196x196, const String &errMsgLn1,
               const String &errMsgLn2) {
  display.setFont(&FONT_26pt8b);
  if (!errMsgLn2.isEmpty()) {
    drawString(EFF_WIDTH / 2, EFF_HEIGHT / 2 + 196 / 2 + 21, errMsgLn1, CENTER);
    drawString(EFF_WIDTH / 2, EFF_HEIGHT / 2 + 196 / 2 + 21 + 55, errMsgLn2,
               CENTER);
  } else {
    drawMultiLnString(EFF_WIDTH / 2, EFF_HEIGHT / 2 + 196 / 2 + 21, errMsgLn1,
                      CENTER, EFF_WIDTH - 200, 2, 55);
  }
  drawBmp(EFF_WIDTH / 2 - 196 / 2, EFF_HEIGHT / 2 - 196 / 2 - 21,
          bitmap_196x196, 196, 196, ACCENT_COLOR);
  return;
}
