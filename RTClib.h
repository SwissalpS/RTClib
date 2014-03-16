// This code is based on https://github.com/adafruit/RTClib
// SwissalpS minimalized it to only support getting and setting time
// Also allows checking if clock is running. (Start by setting time)
// Note: supports dates 2000..2099
// SwissalpS comment END //
// Original comment:
// Code by JeeLabs http://news.jeelabs.org/code/
// Released to the public domain! Enjoy!
// Original comment END //

#ifndef _RTCLIB_H_
#define _RTCLIB_H_

#ifndef YES
  #define YES 0x01u
#endif
#ifndef NO
  #define NO 0x00u
#endif

#define DS1307_ADDRESS ((uint8_t)0x68u)

#define SECONDS_PER_DAY 86400ul
#define SECONDS_FROM_1970_TO_2000 946684800ul


static uint16_t date2daysSince2k(uint16_t uiYear, uint8_t ubMonth, uint8_t ubDay);
static uint8_t isLeapYear(uint16_t uiYear);
static uint8_t leapDaysJan2kUntilJanOfYear(uint16_t uiYear);


// Simple general-purpose date/time class (no TZ / DST / leap second handling!)
class DateTime {

protected:
	uint8_t _ubYearsSince2k, _ubMonths, _ubDays, _ubHours, _ubMinutes, _ubSeconds;

public:
    // constructors
	DateTime(uint32_t ulSecondsUNIX = 0u);
	DateTime(uint16_t uiYear, uint8_t ubMonth, uint8_t ubDay, uint8_t ubHour = 0u, uint8_t ubMinute = 0u, uint8_t ubSecond = 0u);

	// getters
	uint16_t year() const       { return 2000u + _ubYearsSince2k; }
	uint8_t month() const       { return _ubMonths; }
	uint8_t day() const         { return _ubDays; }
	uint8_t hour() const        { return _ubHours; }
	uint8_t minute() const      { return _ubMinutes; }
	uint8_t second() const      { return _ubSeconds; }
	uint8_t dayOfWeek() const;

	// 32-bit times as seconds since 1/1/1970
	uint32_t unixTime(void) const;

};

// RTC based on the DS1307 chip connected via I2C and the Wire library
class RTC_DS1307 {

public:
	static uint8_t begin(void);
	uint8_t isRunning(void);
	static DateTime now();
	static void set(const DateTime &dt);

};

// shared instance
extern RTC_DS1307 RTC;

#endif // _RTCLIB_H_
