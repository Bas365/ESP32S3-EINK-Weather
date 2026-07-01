/* Per-day hourly temp/precip history for the outlook graph.
 * Stores one reading per hour of the current local day in NVS so the graph
 * can show actual readings for elapsed hours plus the forecast for the rest
 * of the day. Resets automatically when the calendar day changes.
 */
#ifndef __HISTORY_H__
#define __HISTORY_H__

#include <time.h>

// Load today's record from NVS, store the current reading into this hour's
// slot, and persist. Resets the record when the day changes.
//   currentTempK   - current temperature in Kelvin (owm current.temp)
//   currentPopPct  - current precip probability in percent (0..100)
//   dailyMaxTempK  - today's forecast high in Kelvin (owm daily[0].temp.max)
//   nowLocal       - current local time
void historyUpdate(float currentTempK, float currentPopPct,
                   float dailyMaxTempK, const tm &nowLocal);

// Accessors for the in-RAM record populated by historyUpdate().
bool  historyHas(int hour);       // true if hour (0..23) has a recorded reading
float historyTempK(int hour);     // recorded temperature in Kelvin
int   historyPop(int hour);       // recorded precip probability in percent
float historyDayMaxTempK();       // running maximum high temp seen today in Kelvin

#endif // __HISTORY_H__
