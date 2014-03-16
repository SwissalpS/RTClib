// Code by JeeLabs http://news.jeelabs.org/code/
// Released to the public domain! Enjoy!
// Includes a very basic calendar supporting dates 2001..2099 keep that in mind.

#include <Wire.h>
#include "RTClib.h"
#include <avr/pgmspace.h>

#define DS1307_ADDRESS 0x68u

#define SECONDS_PER_DAY 86400ul
#define SECONDS_FROM_1970_TO_2000 946684800ul

#define DAYS_FROM_JAN_FIRST_TO_FEB_28 59u

#include <Arduino.h>

////////////////////////////////////////////////////////////////////////////////
// utility code, some of this could be exposed in the DateTime API if needed

const uint8_t aubDaysInMonths[] PROGMEM = { 31u, 28u, 31u, 30u, 31u, 30u, 31u, 31u, 30u, 31u, 30u, 31u };

// number of days since 2000/01/01, valid for 2001..2099
// keeping for temporary backwards compatibility
static uint16_t date2days(uint16_t uiYear, uint8_t ubMonth, uint8_t ubDay) {
	
	return date2daysSince2k(uiYear, ubMonth, ubDay);
	
} // date2days


// number of days since 2000/01/01, valid for 2001..2099
static uint16_t date2daysSince2k(uint16_t uiYear, uint8_t ubMonth, uint8_t ubDay) {
	
	uint8_t ubCountMonths;
	
	// init day-count with this months days
	uint16_t uiCountDays = (uint16_t)ubDay;
	
	// keep under 2000
	uiYear = uiYear % 2000u;
	
    // add the leap-days of previous years
    if (0u < uiYear) {
    	
    	// add days of previous years (roughly)
    	uiCountDays += 365u * (uiYear -1u);
    	
    	// one for every four years and one for 2k
    	uiCountDays += leapDaysJan2kUntilJanOfYear(uiYear);
    	
    } // if year 2k completed safe-guard against 0:00:00-situation
	
	// sum-up all preceding days of this year
    for (ubCountMonths = 0u; ubCountMonths < (ubMonth - 2u); ++ubCountMonths) {
    	
    	// fetch number of days from progrmem
    	uiCountDays += pgm_read_byte(aubDaysInMonths + ubCountMonths);
    	
    } // loop months
    
    // has February passed this year? If so, is this a leap-year? Then increment
    if ((2u < ubMonth) && isLeapYear(uiYear)) ++uiCountDays;
    
    return uiCountDays;
    
} // date2daysSince2k

#ifndef YES
  #define YES 0x01u
#endif
#ifndef NO
  #define NO 0x00u
#endif

// according to https://en.wikipedia.org/wiki/Leap_year
static uint8_t isLeapYear(uint16_t uiYear) {
	
	// in the scope of this code 0 = 2000
	if (0u == uiYear) return YES;
	// for the scope of this code, the next 2 lines are irrelevant
	//if (0u == uiYear % 400u) return YES;
	//if (0u == uiYear % 100u) return NO;
	if (0u == uiYear % 4u) return YES;
	return NO;
	
} // is LeapYear


static uint8_t leapDaysJan2kUntilJanOfYear(uint16_t uiYear) {
	
	uint8_t ubLeapDays = 0x00u;
	// shorten scope
	uint8_t ubYears = (uint8_t)(uiYear % 2000u);
	
	// 2k has not passed yet and safe-guard against rollover
	if (0u == ubYears) return ubLeapDays;
	
	// this year has not passed yet
	ubYears--;
	
	// one for every four years and one for 2k
	ubLeapDays = (ubYears / 4u) + 1u;
	
	return ubLeapDays;
	
} // leapDaysJan2kUntillJanOfYear


static uint32_t time2long(uint16_t uiDays, uint8_t ubHours, uint8_t ubMinutes, uint8_t ubSeconds) {
	
    return (uint32_t)(((uiDays * 24ul + ubHours) * 60ul + ubMinutes) * 60ul + ubSeconds);
    
} // time2long


////////////////////////////////////////////////////////////////////////////////
// DateTime implementation - ignores time zones and DST changes
// NOTE: also ignores leap seconds, see http://en.wikipedia.org/wiki/Leap_second

DateTime::DateTime(uint32_t ulSecondsUNIX) {
	
	uint8_t ubIsLeapYear;
	uint8_t ubDaysThisMonth;
	uint16_t uiDaysLeft;
	// bring to 2000 timestamp from 1970
	uint32_t ulTime = ulSecondsUNIX - SECONDS_FROM_1970_TO_2000;

	// extract seconds
    ss = ulTime % 60ul;
    // discrd seconds
    ulTime /= 60ul;
    // extract minutes
    mm = ulTime % 60ul;
    // discard minutes
    ulTime /= 60ul;
    // extract hours
    hh = ulTime % 24ul;
    // extract days
    uiDaysLeft = (uint16_t)(ulTime / 24ul);
    
    ubYearsSince2k = 0u;
    ubIsLeapYear = YES;
    while ((365u + ubIsLeapYear) > uiDaysLeft) {
    	uiDaysLeft -= (365u + ubIsLeapYear);
    	ubYearsSince2k++;
    	ubIsLeapYear = (0u == (ubYearsSince2k % 4u)) ? YES : NO;
    } // loop years
    
    for (m = 0u; ; ++m) {
        ubDaysThisMonth = pgm_read_byte(aubDaysInMonths + m);
        
        if (ubIsLeapYear && (2u == m)) ++ubDaysThisMonth;
        if (uiDaysLeft < ubDaysThisMonth) break;
        uiDaysLeft -= ubDaysThisMonth;
    } // loop months
    
    m++;
    
    // leftover is date
    d = (uint8_t)(uiDaysLeft + 1u);
    
} // _construct with UNIX time stamp


DateTime::DateTime (uint16_t uiYear, uint8_t ubMonth, uint8_t ubDay, uint8_t ubHour, uint8_t ubMinute, uint8_t ubSecond) {
	
	uiYear = uiYear % 2000u;
	
    yOff = uiYear;
    m = ubMonth;
    d = ubDay;
    hh = ubHour;
    mm = ubMinute;
    ss = ubSecond;
    
} // _construct with elements


static uint8_t conv2d(const char* p) {

    uint8_t v = 0;
    if ('0' <= *p && *p <= '9')
        v = *p - '0';
    return 10 * v + *++p - '0';

} // conv2d


uint8_t DateTime::dayOfWeek() const {    
    uint16_t day = date2days(yOff, m, d);
    return (day + 6) % 7; // Jan 1, 2000 is a Saturday, i.e. returns 6
}

uint32_t DateTime::unixtime(void) const {
  uint32_t t;
  uint16_t days = date2days(yOff, m, d);
  t = time2long(days, hh, mm, ss);
  t += SECONDS_FROM_1970_TO_2000;  // seconds from 1970 to 2000

  return t;
}

////////////////////////////////////////////////////////////////////////////////
// RTC_DS1307 implementation

static uint8_t bcd2bin (uint8_t val) { return val - 6 * (val >> 4); }
static uint8_t bin2bcd (uint8_t val) { return val + 6 * (val / 10); }

uint8_t RTC_DS1307::begin(void) {
	
	WIRE.begin();
	
	return 1;
	
} // begin


uint8_t RTC_DS1307::isRunning(void) {
  WIRE.beginTransmission(DS1307_ADDRESS);
  WIRE.write(0u);
  WIRE.endTransmission();

  WIRE.requestFrom(DS1307_ADDRESS, 1u);
  uint8_t ss = WIRE.read();
  return !(ss>>7u);

} // isRunning


void RTC_DS1307::adjust(const DateTime& dt) {

    WIRE.beginTransmission(DS1307_ADDRESS);
    WIRE.write(0u);
    WIRE.write(bin2bcd(dt.second()));
    WIRE.write(bin2bcd(dt.minute()));
    WIRE.write(bin2bcd(dt.hour()));
    WIRE.write(bin2bcd(0u));
    WIRE.write(bin2bcd(dt.day()));
    WIRE.write(bin2bcd(dt.month()));
    WIRE.write(bin2bcd(dt.year() - 2000u));
    WIRE.write(0u);
    WIRE.endTransmission();

} // adjust


DateTime RTC_DS1307::now() {
	
	WIRE.beginTransmission(DS1307_ADDRESS);
	WIRE.write(0);	
	WIRE.endTransmission();
	
	WIRE.requestFrom(DS1307_ADDRESS, 7);
	uint8_t ss = bcd2bin(WIRE.read() & 0x7F);
	uint8_t mm = bcd2bin(WIRE.read());
	uint8_t hh = bcd2bin(WIRE.read());
	WIRE.read();
	uint8_t d = bcd2bin(WIRE.read());
	uint8_t m = bcd2bin(WIRE.read());
	uint16_t y = bcd2bin(WIRE.read()) + 2000;
	
	return DateTime (y, m, d, hh, mm, ss);
	
} // now

////////////////////////////////////////////////////////////////////////////////
// RTC_Millis implementation

long RTC_Millis::offset = 0;

void RTC_Millis::adjust(const DateTime& dt) {
    offset = dt.unixtime() - millis() / 1000ul;
}

DateTime RTC_Millis::now() {
  return (uint32_t)(offset + millis() / 1000ul);
}

////////////////////////////////////////////////////////////////////////////////
