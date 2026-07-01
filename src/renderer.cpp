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
#include "history.h"
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

// =============================================================================
// HERO LAYOUT ("EInk Weather Variations" 1a) — geometry + margin-aware drawing
// All coordinates below are in the effective (post-margin) coordinate space:
//   origin (0,0) = top-left of the usable panel, size EFF_WIDTH x EFF_HEIGHT.
// The m*() helpers add the MARGIN offsets so callers stay in panel space.
// =============================================================================
namespace hero {
  const int W          = EFF_WIDTH;       // 760
  const int H          = EFF_HEIGHT;      // 456
  const int STATUS_H   = 34;              // top status bar height
  const int BODY_Y0    = STATUS_H;        // 34
  const int BODY_Y1    = 270;             // bottom of current/forecast body
  const int LCOL_W     = 380;             // left (current conditions) column — screen centre
  const int METRIC_H   = 88;              // bottom metric strip in left column
  const int METRIC_Y0  = BODY_Y1 - METRIC_H; // 182
  const int GRAPH_Y0   = BODY_Y1;         // 270 (top of hourly graph)
}

/* Margin-aware horizontal line of thickness t. */
static void mHLine(int x0, int x1, int y, int t = 1, uint16_t c = GxEPD_BLACK) {
  for (int i = 0; i < t; ++i)
    display.drawLine(x0 + MARGIN_X, y + i + MARGIN_Y,
                     x1 + MARGIN_X, y + i + MARGIN_Y, c);
}

/* Margin-aware vertical line of thickness t. */
static void mVLine(int x, int y0, int y1, int t = 1, uint16_t c = GxEPD_BLACK) {
  for (int i = 0; i < t; ++i)
    display.drawLine(x + i + MARGIN_X, y0 + MARGIN_Y,
                     x + i + MARGIN_X, y1 + MARGIN_Y, c);
}

/* Margin-aware filled rectangle. */
static void mFillRect(int x, int y, int w, int h, uint16_t c = GxEPD_BLACK) {
  display.fillRect(x + MARGIN_X, y + MARGIN_Y, w, h, c);
}

/* Margin-aware single pixel. */
static inline void mPixel(int x, int y, uint16_t c = GxEPD_BLACK) {
  display.drawPixel(x + MARGIN_X, y + MARGIN_Y, c);
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

/* Convert a Kelvin temperature to an int in the configured unit. */
static int tempToUnitInt(float kelvin) {
#ifdef UNITS_TEMP_CELSIUS
  return static_cast<int>(std::round(kelvin_to_celsius(kelvin)));
#elif defined(UNITS_TEMP_FAHRENHEIT)
  return static_cast<int>(std::round(kelvin_to_fahrenheit(kelvin)));
#else
  return static_cast<int>(std::round(kelvin));
#endif
}

/* Convert a m/s wind speed to an int in the configured unit. */
static int windToUnitInt(float ms) {
#ifdef UNITS_SPEED_KILOMETERSPERHOUR
  return static_cast<int>(std::round(meterspersecond_to_kilometersperhour(ms)));
#elif defined(UNITS_SPEED_MILESPERHOUR)
  return static_cast<int>(std::round(meterspersecond_to_milesperhour(ms)));
#elif defined(UNITS_SPEED_KNOTS)
  return static_cast<int>(std::round(meterspersecond_to_knots(ms)));
#elif defined(UNITS_SPEED_BEAUFORT)
  return static_cast<int>(meterspersecond_to_beaufort(ms));
#elif defined(UNITS_SPEED_FEETPERSECOND)
  return static_cast<int>(std::round(meterspersecond_to_feetpersecond(ms)));
#else
  return static_cast<int>(std::round(ms));
#endif
}

/* Draw current conditions — Hero layout left column.
 * Big-number current temperature + icon, HIGH/LOW, condition + feels-like, and
 * a 4-cell metric strip (wind / humidity / UV / sun). Also draws the structural
 * dividers for the body: the vertical split to the 5-day column and the metric
 * strip's top rule.
 */
void drawCurrentConditions(const owm_current_t &current,
                           const owm_daily_t &today,
                           const owm_resp_air_pollution_t &owm_air_pollution,
                           bool airPollutionSuccess, float inTemp,
                           float inHumidity) {
  using namespace hero;
  String dataStr;
  char timeBuffer[12] = {};

  // ---- structural dividers ------------------------------------------------
  // vertical split between current column and 5-day column
  mVLine(LCOL_W, BODY_Y0, BODY_Y1, 2);
  // metric strip top rule (left column only)
  mHLine(0, LCOL_W, METRIC_Y0, 2);

  // ---- weather icon + big temperature ------------------------------------
  drawBmp(8, 56, getCurrentConditionsBitmap64(current, today), 64, 64,
          GxEPD_BLACK);

  const int tempBaseY = 128;
  display.setFont(&FONT_48pt8b_temperature);
  dataStr = String(tempToUnitInt(current.temp));
  drawString(82, tempBaseY, dataStr, LEFT);
  int afterTempX = display.getCursorX() - MARGIN_X;
  display.setFont(&FONT_14pt8b);
#if defined(UNITS_TEMP_CELSIUS) || defined(UNITS_TEMP_FAHRENHEIT)
  drawString(afterTempX + 4, tempBaseY - 36, String("\260"), LEFT);
  int degX = display.getCursorX() - MARGIN_X;
  display.setFont(&FONT_12pt8b);
#ifdef UNITS_TEMP_CELSIUS
  drawString(degX, tempBaseY - 36, "C", LEFT);
#else
  drawString(degX, tempBaseY - 36, "F", LEFT);
#endif
#endif

  // ---- HIGH / LOW (right edge of left column) -----------------------------
  String hiStr = String(tempToUnitInt(today.temp.max)) + "\260";
  String loStr = String(tempToUnitInt(today.temp.min)) + "\260";
  const int hlRight = LCOL_W - 10;
  display.setFont(&FONT_6pt8b);
  drawString(hlRight, 62, "HIGH", RIGHT);
  display.setFont(&FONT_18pt8b);
  drawString(hlRight, 90, hiStr, RIGHT);
  display.setFont(&FONT_6pt8b);
  drawString(hlRight, 118, "LOW", RIGHT);
  display.setFont(&FONT_18pt8b);
  drawString(hlRight, 146, loStr, RIGHT);

  // ---- condition + feels like (row above metric strip) --------------------
  String condStr = current.weather.main;
  if (!current.weather.description.isEmpty())
    condStr = current.weather.description;
  toTitleCase(condStr);
  String feelsStr =
      String(TXT_FEELS_LIKE) + " " + String(tempToUnitInt(current.feels_like)) +
      "\260";
  display.setFont(&FONT_11pt8b);
  int feelsW = getStringWidth(feelsStr);
  display.setFont(&FONT_14pt8b);
  drawMultiLnString(12, METRIC_Y0 - 12, condStr, LEFT,
                    LCOL_W - 24 - feelsW - 8, 1, 0);
  display.setFont(&FONT_11pt8b);
  drawString(LCOL_W - 12, METRIC_Y0 - 12, feelsStr, RIGHT);

  // ---- metric strip (4 cells) --------------------------------------------
  const int cellW = LCOL_W / 4; // 88
  const int iconY = METRIC_Y0 + 8;
  const int valY = METRIC_Y0 + 58;
  const int lblY = METRIC_Y0 + 76;
  for (int i = 1; i < 4; ++i)
    mVLine(i * cellW, METRIC_Y0 + 6, BODY_Y1 - 6, 1, GxEPD_BLACK);

  // cell 0: wind
  int cx = 0 * cellW + cellW / 2;
  drawBmp(cx - 16, iconY, wi_strong_wind_32x32, 32, 32, GxEPD_BLACK);
  display.setFont(&FONT_14pt8b);
  drawString(cx, valY, String(windToUnitInt(current.wind_speed)), CENTER);
  display.setFont(&FONT_6pt8b);
#ifdef UNITS_SPEED_KILOMETERSPERHOUR
  dataStr = String("KM/H ");
#elif defined(UNITS_SPEED_MILESPERHOUR)
  dataStr = String("MPH ");
#else
  dataStr = String("WIND ");
#endif
  dataStr += String(getCompassPointNotation(current.wind_deg));
  drawString(cx, lblY, dataStr, CENTER);

  // cell 1: humidity
  cx = 1 * cellW + cellW / 2;
  drawBmp(cx - 16, iconY, wi_humidity_32x32, 32, 32, GxEPD_BLACK);
  display.setFont(&FONT_14pt8b);
  drawString(cx, valY, String(current.humidity) + "%", CENTER);
  display.setFont(&FONT_6pt8b);
  drawString(cx, lblY, "HUMIDITY", CENTER);

  // cell 2: UV index
  cx = 2 * cellW + cellW / 2;
  drawBmp(cx - 16, iconY, wi_day_sunny_32x32, 32, 32, GxEPD_BLACK);
  unsigned int uvi =
      static_cast<unsigned int>(std::max(std::round(current.uvi), 0.0f));
  display.setFont(&FONT_14pt8b);
  drawString(cx, valY, String(uvi), CENTER);
  display.setFont(&FONT_6pt8b);
  dataStr = String("UV ") + String(getUVIdesc(uvi));
  dataStr.toUpperCase();
  drawString(cx, lblY, dataStr, CENTER);

  // cell 3: sunrise / sunset
  int cellX = 3 * cellW;
  drawBmp(cellX + 4, METRIC_Y0 + 8, wi_sunrise_32x32, 32, 32, GxEPD_BLACK);
  drawBmp(cellX + 4, METRIC_Y0 + 46, wi_sunset_32x32, 32, 32, GxEPD_BLACK);
  display.setFont(&FONT_10pt8b);
  time_t ts = current.sunrise;
  tm *ti = localtime(&ts);
  _strftime(timeBuffer, sizeof(timeBuffer), TIME_FORMAT, ti);
  drawString(cellX + 38, METRIC_Y0 + 30, timeBuffer, LEFT);
  memset(timeBuffer, '\0', sizeof(timeBuffer));
  ts = current.sunset;
  ti = localtime(&ts);
  _strftime(timeBuffer, sizeof(timeBuffer), TIME_FORMAT, ti);
  drawString(cellX + 38, METRIC_Y0 + 68, timeBuffer, LEFT);

  return;
}

/* Draw 5-day forecast — Hero layout right column (vertical list).
 * Each row: weekday, condition icon, hi/lo, condition text, precip %, wind.
 */
void drawForecast(const owm_daily_t *daily, tm timeInfo) {
  using namespace hero;
  const int colX = LCOL_W;             // 354
  const int colR = W;                  // 760
  const int padL = colX + 14;          // left text inset
  const int padR = colR - 6;           // right inset
  const int headY0 = BODY_Y0;          // 34
  const int headH = 24;
  const int rowsY0 = headY0 + headH;   // 58
  // row height set so the divider under row 3 aligns with METRIC_Y0 (Feels Like line)
  const float rowH = (METRIC_Y0 - rowsY0) / 3.0f;  // ~41.3

  // header — wind right-aligned to right edge, precip 90px to its left
  const int windX   = padR;         // right edge (text right-aligned)
  const int precipX = windX - 120;   // precip right-aligned 90px left of wind
  display.setFont(&FONT_8pt8b);
  drawString(padL, headY0 + 16, "5-DAY FORECAST", LEFT);
  display.setFont(&FONT_6pt8b);
  drawString(precipX, headY0 + 16, "PRECIP", RIGHT);
  drawString(windX, headY0 + 16, "WIND", RIGHT);

  String dataStr;
  for (int i = 0; i < 5; ++i) {
    int rowTop = rowsY0 + static_cast<int>(std::round(i * rowH));
    int rowBot = rowsY0 + static_cast<int>(std::round((i + 1) * rowH));
    int yc = (rowTop + rowBot) / 2;     // vertical center
    int baseY = yc + 6;                 // text baseline approx
    mHLine(colX, colR, rowTop, 1, GxEPD_BLACK);

    // weekday
    display.setFont(&FONT_11pt8b);
    char dayBuffer[8] = {};
    _strftime(dayBuffer, sizeof(dayBuffer), "%a", &timeInfo);
    drawString(padL, baseY, dayBuffer, LEFT);
    timeInfo.tm_wday = (timeInfo.tm_wday + 1) % 7;

    // condition icon
    drawBmp(colX + 72, yc - 16, getDailyForecastBitmap32(daily[i]), 32, 32,
            GxEPD_BLACK);

    // hi / lo (fixed columns so the lows align vertically across rows)
    String hiStr = String(tempToUnitInt(daily[i].temp.max)) + "\260";
    String loStr = String(tempToUnitInt(daily[i].temp.min)) + "\260";
    display.setFont(&FONT_12pt8b);
    drawString(colX + 120, baseY, hiStr, LEFT);
    display.setFont(&FONT_9pt8b);
    drawString(colX + 156, baseY, loStr, LEFT);

    // precip probability
    int pop = static_cast<int>(std::round(daily[i].pop * 100.0f));
    display.setFont(&FONT_9pt8b);
    drawString(precipX, baseY, String(pop) + "%", RIGHT);

    // wind: direction + speed + unit
    dataStr = String(getCompassPointNotation(daily[i].wind_deg)) + " " +
              String(windToUnitInt(daily[i].wind_speed)) + " km/h";
    drawString(windX, baseY, dataStr, RIGHT);
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

  // Hero layout has no dedicated alert band. Show at most one alert as a
  // single compact line just under the status bar, on the left, so it never
  // collides with the current-conditions block.
  using namespace hero;
  if (num_valid_alerts >= 1) {
    owm_alerts_t &cur_alert = alerts[alert_indices[0]];
    toTitleCase(cur_alert.event);
    drawBmp(8, STATUS_H + 4, getAlertBitmap32(cur_alert), 32, 32, ACCENT_COLOR);
    display.setFont(&FONT_11pt8b);
    drawMultiLnString(46, STATUS_H + 25, cur_alert.event, LEFT, LCOL_W - 60, 1,
                      0, ACCENT_COLOR);
  }

  free(ignore_list);
  free(alert_indices);
  return;
}

/* Draw status bar left + center — Hero layout.
 * City (left) and date (center) inside the top status bar, plus the status
 * bar's bottom rule that spans the full panel width.
 */
void drawLocationDate(const String &city, const String &date) {
  using namespace hero;
  // status bar bottom rule
  mHLine(0, W, STATUS_H, 2, GxEPD_BLACK);
  // city (left)
  display.setFont(&FONT_12pt8b);
  drawString(4, 24, city, LEFT, ACCENT_COLOR);
  // date (center)
  display.setFont(&FONT_11pt8b);
  drawString(W / 2, 24, date, CENTER);
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

/* Draw outlook graph — Hero layout full-width hourly temp + precip.
 * Temperature as a line (left axis, °), precipitation as hatched bars
 * (right axis, %), hourly tick labels along the bottom, with a header row.
 */
void drawOutlookGraph(const owm_hourly_t *hourly, const owm_daily_t *daily,
                      tm timeInfo) {
  using namespace hero;

  // top rule + header row
  mHLine(0, W, GRAPH_Y0, 2, GxEPD_BLACK);
  display.setFont(&FONT_6pt8b);
  // left legend: filled square + TEMP
  mFillRect(8, GRAPH_Y0 + 11, 9, 9, GxEPD_BLACK);
  drawString(22, GRAPH_Y0 + 19, "TEMP \260", LEFT);
  // center title
  display.setFont(&FONT_8pt8b);
  drawString(W / 2, GRAPH_Y0 + 19, "TODAY \267 HOURLY", CENTER);
  // right legend: hatched square + PRECIP
  display.setFont(&FONT_6pt8b);
  for (int yy = 0; yy < 9; ++yy)
    for (int xx = 0; xx < 9; ++xx)
      if (((xx + yy) & 3) == 0)
        mPixel(W - 86 + xx, GRAPH_Y0 + 11 + yy, GxEPD_BLACK);
  display.drawRect(W - 86 + MARGIN_X, GRAPH_Y0 + 11 + MARGIN_Y, 9, 9,
                   GxEPD_BLACK);
  drawString(W - 74, GRAPH_Y0 + 19, "PRECIP %", LEFT);

  // plot area
  const int xPos0 = 34;            // room for left temp labels
  int xPos1 = W - 54;              // room for right precip labels
  const int yPos0 = GRAPH_Y0 + 32; // top of plot
  const int yPos1 = H - 18;        // baseline (room for hour labels)

  // --- assemble TODAY's curve: recorded actuals (past) + forecast (future) --
  const int todayYday = timeInfo.tm_yday;
  const int todayYear = timeInfo.tm_year;
  std::vector<float> hHour;    // hour-of-day, 0..24
  std::vector<float> hTemp;    // temperature in display unit
  std::vector<float> hPrecip;  // precip value (% for POP, else mm)

  // 1) recorded history for hours already elapsed today
  int lastRecHour = -1;
  for (int h = 0; h < 24; ++h) {
    if (!historyHas(h))
      continue;
    hHour.push_back((float)h);
#ifdef UNITS_TEMP_CELSIUS
    hTemp.push_back(kelvin_to_celsius(historyTempK(h)));
#elif defined(UNITS_TEMP_FAHRENHEIT)
    hTemp.push_back(kelvin_to_fahrenheit(historyTempK(h)));
#else
    hTemp.push_back(historyTempK(h));
#endif
    hPrecip.push_back((float)historyPop(h));
    lastRecHour = h;
  }

  // 2) forecast for the rest of today (+ next midnight as hour 24)
  for (int i = 0; i < OWM_NUM_HOURLY; ++i) {
    time_t ts = hourly[i].dt;
    tm lt = *localtime(&ts);
    float hod = lt.tm_hour + lt.tm_min / 60.0f;
    bool sameDay = (lt.tm_yday == todayYday && lt.tm_year == todayYear);
    bool nextMidnight = (!sameDay) && (lt.tm_hour == 0) &&
                        ((lt.tm_year > todayYear) || (lt.tm_yday == todayYday + 1));
    if (sameDay) {
      if (hod <= lastRecHour + 0.01f)
        continue; // already have a recorded actual for this hour
      hHour.push_back(hod);
    } else if (nextMidnight) {
      hHour.push_back(24.0f); // close the curve at the right edge
    } else if (!hHour.empty()) {
      break; // past the end of today
    } else {
      continue;
    }
#ifdef UNITS_TEMP_CELSIUS
    hTemp.push_back(kelvin_to_celsius(hourly[i].temp));
#elif defined(UNITS_TEMP_FAHRENHEIT)
    hTemp.push_back(kelvin_to_fahrenheit(hourly[i].temp));
#else
    hTemp.push_back(hourly[i].temp);
#endif
#ifdef UNITS_HOURLY_PRECIP_POP
    hPrecip.push_back(hourly[i].pop * 100.0f);
#else
    hPrecip.push_back(hourly[i].rain_1h + hourly[i].snow_1h);
#endif
    if (nextMidnight) break;
  }

  // --- temperature scale: tight bounds around plotted hourly data only ------
  float tempMin = hTemp.empty() ? 0.0f : hTemp[0];
  float tempMax = hTemp.empty() ? 0.0f : hTemp[0];
  for (size_t k = 1; k < hTemp.size(); ++k) {
    tempMin = std::min(tempMin, hTemp[k]);
    tempMax = std::max(tempMax, hTemp[k]);
  }
  // Choose step, then snap bounds to the nearest step just outside the data.
  int range = static_cast<int>(std::ceil(tempMax)) -
              static_cast<int>(std::floor(tempMin));
  int step = (range <= 8) ? 2 : (range <= 20 ? 5 : 10);
  int tempBoundMin = static_cast<int>(std::floor(tempMin / (float)step)) * step;
  int tempBoundMax = static_cast<int>(std::ceil(tempMax  / (float)step)) * step;
  // Ensure at least 2 ticks of separation so labels don't collide
  if (tempBoundMax - tempBoundMin < 2 * step) tempBoundMax = tempBoundMin + 2 * step;
  int yMajorTicks    = (tempBoundMax - tempBoundMin) / step;
  int yTempMajorTicks = step;

#ifdef UNITS_HOURLY_PRECIP_POP
  float precipBoundMax = 100.0f; // always show the precip %% axis
#else
  float precipBoundMax = 1.0f;
  for (size_t k = 0; k < hPrecip.size(); ++k)
    precipBoundMax = std::max(precipBoundMax, hPrecip[k]);
  precipBoundMax = std::ceil(precipBoundMax);
#endif

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
      dataStr = String(100 - (i * (100 / yMajorTicks))) + "%";
#else
      dataStr = String(static_cast<int>(precipBoundMax -
                                        (i * precipBoundMax / yMajorTicks))) +
                String(" mm");
#endif
      drawString(xPos1 + 6, yTick + 4, dataStr, LEFT);
    }

    if (i < yMajorTicks) {
      for (int x = xPos0 + MARGIN_X; x <= xPos1 + 1 + MARGIN_X; x += 3) {
        display.drawPixel(x, yTick + (yTick % 2) + MARGIN_Y, GxEPD_BLACK);
      }
    }
  }

  // hour-of-day (0..24) -> x ; unit temp -> y
  const float xSpan = static_cast<float>(xPos1 - xPos0);
  auto xOf = [&](float hod) { return xPos0 + (hod / 24.0f) * xSpan; };
  float yPxPerUnit =
      (yPos1 - yPos0) / static_cast<float>(tempBoundMax - tempBoundMin);
  auto yOf = [&](float t) {
    return static_cast<int>(std::round(yPos1 - yPxPerUnit * (t - tempBoundMin)));
  };

  // precip bars (one per 3h slot), diagonal-hatch fill + outline
  float yPxPerPrecip =
      (precipBoundMax > 0) ? (yPos1 - yPos0) / precipBoundMax : 0;
  int barHalf = static_cast<int>((xSpan / 24.0f) * 1.5f * 0.8f);
  for (size_t k = 0; k < hHour.size(); ++k) {
    float pv = hPrecip[k];
    if (!(precipBoundMax > 0 && pv > 0))
      continue;
    int cxp = static_cast<int>(std::round(xOf(hHour[k])));
    int bx0 = cxp - barHalf + MARGIN_X;
    int bx1 = cxp + barHalf + MARGIN_X;
    if (bx0 < xPos0 + MARGIN_X) bx0 = xPos0 + MARGIN_X;
    if (bx1 > xPos1 + MARGIN_X) bx1 = xPos1 + MARGIN_X;
    int y0p = static_cast<int>(std::round(yPos1 - yPxPerPrecip * pv)) + MARGIN_Y;
    int y1p = yPos1 + MARGIN_Y;
    if (bx1 > bx0) {
      for (int y = y0p; y < y1p; ++y)
        for (int x = bx0; x < bx1; ++x)
          if (((x + y) & 3) == 0)
            display.drawPixel(x, y, GxEPD_BLACK);
      display.drawLine(bx0, y0p, bx1 - 1, y0p, GxEPD_BLACK);
      display.drawLine(bx0, y0p, bx0, y1p - 1, GxEPD_BLACK);
      display.drawLine(bx1 - 1, y0p, bx1 - 1, y1p - 1, GxEPD_BLACK);
    }
  }

  // temperature polyline (thick)
  for (size_t k = 1; k < hHour.size(); ++k) {
    int x0 = static_cast<int>(std::round(xOf(hHour[k - 1]))) + MARGIN_X;
    int x1 = static_cast<int>(std::round(xOf(hHour[k]))) + MARGIN_X;
    int y0 = yOf(hTemp[k - 1]) + MARGIN_Y;
    int y1 = yOf(hTemp[k]) + MARGIN_Y;
    display.drawLine(x0, y0, x1, y1, ACCENT_COLOR);
    display.drawLine(x0, y0 + 1, x1, y1 + 1, ACCENT_COLOR);
    display.drawLine(x0 - 1, y0, x1 - 1, y1, ACCENT_COLOR);
  }

  // x-axis hour ticks every 6h: 00 06 12 18 24
  display.setFont(&FONT_8pt8b);
  for (int hh = 0; hh <= 24; hh += 3) {
    int xt = static_cast<int>(std::round(xOf((float)hh)));
    display.drawLine(xt + MARGIN_X, yPos1 + 1 + MARGIN_Y, xt + MARGIN_X,
                     yPos1 + 4 + MARGIN_Y, GxEPD_BLACK);
    display.drawLine(xt + 1 + MARGIN_X, yPos1 + 1 + MARGIN_Y, xt + 1 + MARGIN_X,
                     yPos1 + 4 + MARGIN_Y, GxEPD_BLACK);
    char tb[6];
    snprintf(tb, sizeof(tb), "%02d", hh);
    drawString(xt, yPos1 + 1 + 12 + 4 + 3, String(tb), CENTER);
  }
  return;
}

/* Draw status bar right side — Hero layout.
 * Inside the top status bar, right-aligned: "Updated <time>", WiFi icon and
 * battery icon + percentage.
 */
void drawStatusBar(const String &statusStr, const String &refreshTimeStr,
                   int rssi, uint32_t batVoltage) {
  using namespace hero;
  String dataStr;
  uint16_t dataColor = GxEPD_BLACK;
  const int yMid = STATUS_H / 2;     // 17
  const int textBase = yMid + 5;     // baseline for small text
  const int sp = 6;
  int pos = W - 4;

#if BATTERY_MONITORING
  uint32_t batPercent =
      calcBatPercent(batVoltage, MIN_BATTERY_VOLTAGE, MAX_BATTERY_VOLTAGE);
#if STATUS_BAR_EXTRAS_BAT_PERCENTAGE
  display.setFont(&FONT_8pt8b);
  dataStr = String(batPercent) + "%";
  drawString(pos, textBase, dataStr, RIGHT, dataColor);
  pos -= getStringWidth(dataStr) + 4;
#endif
  pos -= 24;
  drawBmp(pos, yMid - 12, getBatBitmap24(batPercent), 24, 24, dataColor);
  pos -= sp;
#endif

  // WiFi
  dataColor = rssi >= -70 ? GxEPD_BLACK : ACCENT_COLOR;
  pos -= 16;
  drawBmp(pos, yMid - 8, getWiFiBitmap16(rssi), 16, 16, dataColor);
  pos -= sp;

  // Updated time
  dataColor = GxEPD_BLACK;
  display.setFont(&FONT_8pt8b);
  dataStr = String("Updated ") + refreshTimeStr;
  drawString(pos, textBase, dataStr, RIGHT, dataColor);

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
