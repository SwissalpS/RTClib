// This code is based on https://github.com/adafruit/RTClib
// SwissalpS minimalized it to only support getting and setting time
// Also allows checking if clock is running. (Start by setting time)
// Note: supports dates 2000..2099
// SwissalpS comment END //
// Original comment:
// Code by JeeLabs http://news.jeelabs.org/code/
// Released to the public domain! Enjoy!
// Original comment END //

#include <Wire.h>
#include <avr/pgmspace.h>
#include <Arduino.h>

#include "RTClib.h"

////////////////////////////////////////////////////////////////////////////////
// utility code, some of this could be exposed in the DateTime API if needed

const uint8_t aubDaysInMonths[] PROGMEM = { 31u, 28u, 31u, 30u, 31u, 30u, 31u, 31u, 30u, 31u, 30u, 31u };


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


// to help calculate unix timestamp
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
    _ubYearsSince2k = 0u;

	// extract seconds
    _ubSeconds = ulTime % 60ul;
    // discrd seconds
    ulTime /= 60ul;
	
    // extract minutes
    _ubMinutes = ulTime % 60ul;
    // discard minutes
    ulTime /= 60ul;
	
    // extract hours
    _ubHours = ulTime % 24ul;
	
    // extract days
    uiDaysLeft = (uint16_t)(ulTime / 24ul);
    
	// 2000 was a leap-year
    ubIsLeapYear = YES;
	
	// loop years subtracting days
    while ((365u + ubIsLeapYear) > uiDaysLeft) {
    	uiDaysLeft -= (365u + ubIsLeapYear);
    	_ubYearsSince2k++;
    	ubIsLeapYear = isLeapYear((uint16_t)_ubYearsSince2k);
    } // loop years
    
	// loop months subtracting days
    for (_ubMonths = 0u; ; ++_ubMonths) {
		
        ubDaysThisMonth = pgm_read_byte(aubDaysInMonths + _ubMonths);
		
        // 1 == February: add leap-day if leap-year
        if ((1u == _ubMonths) && ubIsLeapYear) ++ubDaysThisMonth;
		
		// break the loop if this month hasn't passed
        if (ubDaysThisMonth > uiDaysLeft) break;
		
		// subtract days this month from days left
        uiDaysLeft -= ubDaysThisMonth;
		
    } // loop months
    
    _ubMonths++;
    
    // leftover is date
    _ubDays = (uint8_t)(uiDaysLeft + 1u);
    
} // _construct with UNIX time stamp


DateTime::DateTime (uint16_t uiYear, uint8_t ubMonth, uint8_t ubDay, uint8_t ubHour, uint8_t ubMinute, uint8_t ubSecond) {
	
	// base year at 2k
    _ubYearsSince2k	= uiYear % 2000u;
    _ubMonths		= ubMonth;
    _ubDays			= ubDay;
	_ubHours		= ubHour;
    _ubMinutes		= ubMinute;
	_ubSeconds		= ubSecond;
    
} // _construct with elements


uint8_t DateTime::dayOfWeek() const {
	
    uint16_t uiDays = date2daysSince2k(_ubYearsSince2k, _ubMonths, _ubDays);
	
	// Jan 1, 2000 is a Saturday, i.e. returns 6
    return (uint8_t)((uiDays + 6u) % 7u);

} // dayOfWeek


uint32_t DateTime::unixTime(void) const {
	
	uint32_t ulTime;
	uint16_t uiDays = date2daysSince2k(_ubYearsSince2k, _ubMonths, _ubDays);
	
	ulTime = time2long(uiDays, _ubHours, _ubMinutes, _ubSeconds);
	
	// add seconds from 1970 to 2000
	ulTime += SECONDS_FROM_1970_TO_2000;
	
	return ulTime;

} // unixTime

////////////////////////////////////////////////////////////////////////////////
// RTC_DS1307 implementation

static uint8_t bcd2bin (uint8_t ubValue) { return ubValue - 6u * (ubValue >> 4u); }
static uint8_t bin2bcd (uint8_t ubValue) { return ubValue + 6u * (ubValue / 10u); }

uint8_t RTC_DS1307::begin(void) {
	
	Wire.begin();
	
	return 1u;
	
} // begin


uint8_t RTC_DS1307::isRunning(void) {
	
  Wire.beginTransmission(DS1307_ADDRESS);
  Wire.write(0u);
  Wire.endTransmission();

  Wire.requestFrom(DS1307_ADDRESS, 1u);
  uint8_t ubByte = Wire.read();
  return !(ubByte >> 7u);

} // isRunning


void RTC_DS1307::set(const DateTime &dt) {

    Wire.beginTransmission(DS1307_ADDRESS);
    Wire.write(0u);
    Wire.write(bin2bcd(dt.second()));
    Wire.write(bin2bcd(dt.minute()));
    Wire.write(bin2bcd(dt.hour()));
    Wire.write(bin2bcd(0u));
    Wire.write(bin2bcd(dt.day()));
    Wire.write(bin2bcd(dt.month()));
    Wire.write(bin2bcd(dt.year() - 2000u));
    Wire.write(0u);
    Wire.endTransmission();

} // set


DateTime RTC_DS1307::now() {
	
	uint8_t ubMonth, ubDay, ubHour, ubMinute, ubSecond;
	uint16_t uiYear;
	
	Wire.beginTransmission(DS1307_ADDRESS);
	Wire.write(0u);
	Wire.endTransmission();
	
	Wire.requestFrom(DS1307_ADDRESS, 7u);
	ubSecond	= bcd2bin(Wire.read() & 0x7Fu);
	ubMinute	= bcd2bin(Wire.read());
	ubHour		= bcd2bin(Wire.read());
	Wire.read();
	ubDay		= bcd2bin(Wire.read());
	ubMonth		= bcd2bin(Wire.read());
	uiYear		= bcd2bin(Wire.read()) + 2000u;
	
	return DateTime(uiYear, ubMonth, ubDay, ubHour, ubMinute, ubSecond);
	
} // now


