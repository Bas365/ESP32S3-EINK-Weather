/* Per-day hourly temp/precip history (see history.h). */
#include "history.h"
#include "config.h"
#include <Preferences.h>
#include <cmath>
#include <cstring>

static const char *HIST_KEY = "dayhist";

// Persisted record. 80 bytes — trivially small for NVS.
struct DayHistory {
  int32_t  dayKey;       // year * 1000 + yday (identifies the calendar day)
  uint32_t filled;       // bit per hour (0..23) set when a reading is stored
  int16_t  tempK10[24];  // temperature, Kelvin * 10
  uint8_t  pop[24];      // precip probability, 0..100
};

static DayHistory g_hist;
static bool g_loaded = false;

static int32_t dayKeyOf(const tm &t) {
  return (int32_t)t.tm_year * 1000 + t.tm_yday;
}

void historyUpdate(float currentTempK, float currentPopPct, const tm &nowLocal) {
  Preferences p;
  p.begin(NVS_NAMESPACE, false);

  size_t n = p.getBytesLength(HIST_KEY);
  bool ok = false;
  if (n == sizeof(DayHistory)) {
    p.getBytes(HIST_KEY, &g_hist, sizeof(g_hist));
    ok = true;
  }

  int32_t today = dayKeyOf(nowLocal);
  if (!ok || g_hist.dayKey != today) {
    memset(&g_hist, 0, sizeof(g_hist));
    g_hist.dayKey = today;
  }

  int h = nowLocal.tm_hour;
  if (h >= 0 && h < 24) {
    g_hist.tempK10[h] = (int16_t)lroundf(currentTempK * 10.0f);
    int pop = (int)lroundf(currentPopPct);
    if (pop < 0) pop = 0;
    if (pop > 100) pop = 100;
    g_hist.pop[h] = (uint8_t)pop;
    g_hist.filled |= (1UL << h);
  }

  p.putBytes(HIST_KEY, &g_hist, sizeof(g_hist));
  p.end();
  g_loaded = true;
}

bool historyHas(int hour) {
  return g_loaded && hour >= 0 && hour < 24 && (g_hist.filled & (1UL << hour));
}

float historyTempK(int hour) {
  if (hour < 0 || hour >= 24) return 0.0f;
  return g_hist.tempK10[hour] / 10.0f;
}

int historyPop(int hour) {
  if (hour < 0 || hour >= 24) return 0;
  return g_hist.pop[hour];
}
