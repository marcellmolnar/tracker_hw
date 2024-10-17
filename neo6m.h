#ifndef __neo6m_h__
#define __neo6m_h__

#include "pico/time.h"

#include <ctype.h>
#include <cinttypes>
#include <stdlib.h>
#include <string.h>

#define GPS_UART_TX_PIN 0
#define GPS_UART_RX_PIN 1

#define GPS_UART_ID   uart0
#define GPS_BAUD_RATE 9600
#define GPS_DATA_BITS 8
#define GPS_STOP_BITS 1
#define GPS_PARITY    UART_PARITY_NONE

#define _GPS_MPH_PER_KNOT 1.15077945
#define _GPS_MPS_PER_KNOT 0.51444444
#define _GPS_KMPH_PER_KNOT 1.852
#define _GPS_MILES_PER_METER 0.00062137112
#define _GPS_KM_PER_METER 0.001
#define _GPS_FEET_PER_METER 3.2808399
#define _GPS_MAX_FIELD_SIZE 15
#define _GPS_EARTH_MEAN_RADIUS 6371009 // old: 6372795

static inline uint32_t millis()
{
    return to_ms_since_boot(get_absolute_time());
}

struct RawDegrees
{
   uint16_t deg;
   uint32_t billionths;
   bool negative;

   RawDegrees() : deg(0), billionths(0), negative(false) {}
};

struct GPSLocation
{
   friend struct GPSPlus;
public:
   enum Quality { Invalid = '0', GPS = '1', DGPS = '2', PPS = '3', RTK = '4', FloatRTK = '5', Estimated = '6', Manual = '7', Simulated = '8' };
   enum Mode { N = 'N', A = 'A', D = 'D', E = 'E'};

   bool isValid() const    { return valid; }
   bool isUpdated() const  { return updated; }
   uint32_t age() const    { return valid ? millis() - lastCommitTime : (uint32_t)UINT32_MAX; }
   const RawDegrees &rawLat()     { updated = false; return rawLatData; }
   const RawDegrees &rawLng()     { updated = false; return rawLngData; }
   double lat();
   double lng();
   Quality FixQuality()           { updated = false; return fixQuality; }
   Mode FixMode()                 { updated = false; return fixMode; }

   GPSLocation() : valid(false), updated(false), fixQuality(Invalid), fixMode(N)
   {}

private:
   bool valid, updated;
   RawDegrees rawLatData, rawLngData, rawNewLatData, rawNewLngData;
   Quality fixQuality, newFixQuality;
   Mode fixMode, newFixMode;
   uint32_t lastCommitTime;
   void commit();
   void setLatitude(const char *term);
   void setLongitude(const char *term);
};

struct GPSDate
{
   friend struct GPSPlus;
public:
   bool isValid() const       { return valid; }
   bool isUpdated() const     { return updated; }
   uint32_t age() const       { return valid ? millis() - lastCommitTime : (uint32_t)UINT32_MAX; }

   uint32_t value()           { updated = false; return date; }
   uint16_t year();
   uint8_t month();
   uint8_t day();

   GPSDate() : valid(false), updated(false), date(0)
   {}

private:
   bool valid, updated;
   uint32_t date, newDate;
   uint32_t lastCommitTime;
   void commit();
   void setDate(const char *term);
};

struct GPSTime
{
   friend struct GPSPlus;
public:
   bool isValid() const       { return valid; }
   bool isUpdated() const     { return updated; }
   uint32_t age() const       { return valid ? millis() - lastCommitTime : (uint32_t)UINT32_MAX; }

   uint32_t value()           { updated = false; return time; }
   uint8_t hour();
   uint8_t minute();
   uint8_t second();
   uint8_t centisecond();

   GPSTime() : valid(false), updated(false), time(0)
   {}

private:
   bool valid, updated;
   uint32_t time, newTime;
   uint32_t lastCommitTime;
   void commit();
   void setTime(const char *term);
};

struct GPSDecimal
{
   friend struct GPSPlus;
public:
   bool isValid() const    { return valid; }
   bool isUpdated() const  { return updated; }
   uint32_t age() const    { return valid ? millis() - lastCommitTime : (uint32_t)UINT32_MAX; }
   int32_t value()         { updated = false; return val; }

   GPSDecimal() : valid(false), updated(false), val(0)
   {}

private:
   bool valid, updated;
   uint32_t lastCommitTime;
   int32_t val, newval;
   void commit();
   void set(const char *term);
};

struct GPSInteger
{
   friend struct GPSPlus;
public:
   bool isValid() const    { return valid; }
   bool isUpdated() const  { return updated; }
   uint32_t age() const    { return valid ? millis() - lastCommitTime : (uint32_t)UINT32_MAX; }
   uint32_t value()        { updated = false; return val; }

   GPSInteger() : valid(false), updated(false), val(0)
   {}

private:
   bool valid, updated;
   uint32_t lastCommitTime;
   uint32_t val, newval;
   void commit();
   void set(const char *term);
};

struct GPSSpeed : GPSDecimal
{
   double knots()    { return value() / 100.0; }
   double mph()      { return _GPS_MPH_PER_KNOT * value() / 100.0; }
   double mps()      { return _GPS_MPS_PER_KNOT * value() / 100.0; }
   double kmph()     { return _GPS_KMPH_PER_KNOT * value() / 100.0; }
};

struct GPSCourse : public GPSDecimal
{
   double deg()      { return value() / 100.0; }
};

struct GPSAltitude : GPSDecimal
{
   double meters()       { return value() / 100.0; }
   double miles()        { return _GPS_MILES_PER_METER * value() / 100.0; }
   double kilometers()   { return _GPS_KM_PER_METER * value() / 100.0; }
   double feet()         { return _GPS_FEET_PER_METER * value() / 100.0; }
};

struct GPSHDOP : GPSDecimal
{
   double hdop() { return value() / 100.0; }
};

struct GPSPlus;
struct GPSCustom
{
public:
   GPSCustom() {};
   GPSCustom(GPSPlus &gps, const char *sentenceName, int termNumber);
   void begin(GPSPlus &gps, const char *_sentenceName, int _termNumber);

   bool isUpdated() const  { return updated; }
   bool isValid() const    { return valid; }
   uint32_t age() const    { return valid ? millis() - lastCommitTime : (uint32_t)UINT32_MAX; }
   const char *value()     { updated = false; return buffer; }

private:
   void commit();
   void set(const char *term);

   char stagingBuffer[_GPS_MAX_FIELD_SIZE + 1];
   char buffer[_GPS_MAX_FIELD_SIZE + 1];
   unsigned long lastCommitTime;
   bool valid, updated;
   const char *sentenceName;
   int termNumber;
   friend struct GPSPlus;
   GPSCustom *next;
};

struct GPSPlus
{
public:
    GPSPlus();
    bool encode(char c);

    GPSLocation location;
    GPSDate date;
    GPSTime time;
    GPSSpeed speed;
    GPSCourse course;
    GPSAltitude altitude;
    GPSInteger satellites;
    GPSHDOP hdop;

    static double distanceBetween(double lat1, double long1, double lat2, double long2);
    static double courseTo(double lat1, double long1, double lat2, double long2);
    static const char *cardinal(double course);

    static int32_t parseDecimal(const char *term);
    static void parseDegrees(const char *term, RawDegrees &deg);

    uint32_t charsProcessed()   const { return encodedCharCount; }
    uint32_t sentencesWithFix() const { return sentencesWithFixCount; }
    uint32_t failedChecksum()   const { return failedChecksumCount; }
    uint32_t passedChecksum()   const { return passedChecksumCount; }

private:
    enum {GPS_SENTENCE_GGA, GPS_SENTENCE_RMC, GPS_SENTENCE_OTHER};

    // parsing state variables
    uint8_t parity;
    bool isChecksumTerm;
    char term[_GPS_MAX_FIELD_SIZE];
    uint8_t curSentenceType;
    uint8_t curTermNumber;
    uint8_t curTermOffset;
    bool sentenceHasFix;

    // custom element support
    friend struct GPSCustom;
    GPSCustom *customElts;
    GPSCustom *customCandidates;
    void insertCustom(GPSCustom *pElt, const char *sentenceName, int index);

    // statistics
    uint32_t encodedCharCount;
    uint32_t sentencesWithFixCount;
    uint32_t failedChecksumCount;
    uint32_t passedChecksumCount;

    // internal utilities
    int fromHex(char a);
    bool endOfTermHandler();
};

#endif
