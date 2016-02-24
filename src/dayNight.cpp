/*******************************************************************************
**
**    I place this program under the "GNU General Public License".
**        http://www.gnu.org/licenses/licenses.html#GPL
**
*******************************************************************************/

#undef    DEBUGLOG
#define   DEBUGNSP   "dayn"

#include "dayNight.h"   //
#include "sunRiseSet.h" //
#include "debug.h"      // for debugging prints

#define   NOTZOFFSET  65535

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
DayNight::DayNight()
{
//	set(0, 0, NOTZOFFSET);
	set(0, 0);
}

// -----------------------------------------------------------------------------
DayNight::DayNight(double _latit, double _longit)
{
	set(_latit, _longit);
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
void DayNight::set(double _latit, double _longit)
{
	latit  = _latit;
	longit = _longit;

	calc();
}
#if 0
// -----------------------------------------------------------------------------
void DayNight::set(double _latit, double _longit, int _tzoff)
{
	tzoff = _tzoff;
	set(latit, longit);
}
#endif
// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
void DayNight::get(tm& rise, tm& set)
{
//	time_t tt    =  time(NULL);

//	rise         = *localtime(&tt);
	rise         =  lt;
	rise.tm_hour =  HOURS  (sunrise);
	rise.tm_min  =  MINUTES(sunrise);
	mktime(&rise);

	set          =  rise;
	set.tm_hour  =  HOURS  (sunset);
	set.tm_min   =  MINUTES(sunset);
	mktime(&set);
}

// -----------------------------------------------------------------------------
void DayNight::get(double& rise, double& set)
{
	rise = sunrise;
	set  = sunset;
}

// -----------------------------------------------------------------------------
void DayNight::get(double* rise, double* set, double* ra, double* dec)
{
	if( rise ) *rise = sunrise;
	if( set  ) *set  = sunset;
	if( ra   ) *ra   = sunRA;
	if( dec  ) *dec  = sunDec;
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
void DayNight::calc()
{
	tzset();

	time_t tt   =  time(NULL);
//	tm     lt   = *localtime(&tt);
	lt          = *localtime(&tt);

	int    year =  lt.tm_year + 1900;
	int    mon  =  lt.tm_mon  + 1;
//	int    day  =  lt.tm_mday + 1;
	int    day  =  lt.tm_mday;

//	long   tzo  =  tzoff == NOTZOFFSET ? lt.tm_gmtoff : tzoff;
	long   tzo  =  lt.tm_gmtoff;       // local time zone gmt offset (sec)
	double off  =  double(tzo)/3600.0; // in hours

	DEBUGLOGP("gmt offset used was %d (secs) %2.2f (hrs)\n", tzo, off);

	if( true ) // if sun's position is desired
	{
		tm   gt = *gmtime(&tt);
		long y  =  gt.tm_year+1900;
		long m  =  gt.tm_mon+1;
		long d  =  gt.tm_mday;
		long so =  gt.tm_hour*3600 + gt.tm_min*60 + gt.tm_sec - (12*3600);
		double     d12h = days_since_2000_Jan_0(y, m, d) + 0.5, sunDist;
		sun_RA_dec(d12h, &sunRA, &sunDec, &sunDist); // get right ascension & declination in degrees
		DEBUGLOGP ("day=%4.4d/%2.2d/%2.2d, tm=%2.2d:%2.2d:%2.2d, d12h=%f\n", y, m, d, gt.tm_hour, gt.tm_min, gt.tm_sec, d12h);
		DEBUGLOGP ("sunRA=%f, sunDec=%f, sunDist=%f\n", (float)sunRA, (float)sunDec, (float)sunDist);
		sunRA  /=  15.0;                   // angular degrees to longitude
		sunRA  +=  double(so)/3600.0*15.0; // gmt time-of-day hours to longitude
		sunRA  *= -1.0;                    // going from east to west
		DEBUGLOGP ("sunRA=%f, sunDec=%f, so=%f\n", (float)sunRA, (float)sunDec, (float)so);
	}

	if( latit != 0.0 || longit != 0.0 )
	{
		sun_rise_set(year, mon, day, longit, latit, &sunrise, &sunset);

		DEBUGLOGP("calc'd sunrise/set are %2.2f & %2.2f\n", (float)sunrise, (float)sunset);

		sunrise = TMOD(sunrise + off);
		sunset  = TMOD(sunset  + off);

		DEBUGLOGP("offs'd sunrise/set are %2.2f & %2.2f\n", (float)sunrise, (float)sunset);
	}
	else
	{
		// unknown geo-position, so use fixed local times
		// TODO: can't we get something better than this? for equatorial gmt for the given time of year?

		sunrise =  6.0;
		sunset  = 18.0;

		DEBUGLOGP("dflt'd sunrise/set are %2.2f & %2.2f\n", (float)sunrise, (float)sunset);
	}
}

