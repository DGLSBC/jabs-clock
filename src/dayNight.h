/*******************************************************************************
**
**    I place this program under the "GNU General Public License".
**        http://www.gnu.org/licenses/licenses.html#GPL
**
*******************************************************************************/

#ifndef __dayNight_h__
#define __dayNight_h__

#include <ctime>

// -----------------------------------------------------------------------------
class DayNight
{
public:
	DayNight();
	DayNight(double latit, double longit);

	// call one of these once, or whenever the target position moves

	void set(double latit, double longit);
//	void set(double latit, double longit, int tzoff);

	// call this once a day, after calling set at least once (this is
	// automatically called by set as well)

	void calc();

	// call these to retrieve the calculated sunrise and sunset values

	void get(tm&     rise, tm&     set);
	void get(double& rise, double& set);

	void get(double* rise=NULL, double* set=NULL, double* ra=NULL, double* dec=NULL);

private:
	tm       lt;
	double   latit;
	double   longit;
//	long int tzoff;   // time zone offset in seconds

	double   sunrise;
	double   sunset;

	double   sunRA;   // sun right ascension (longitude)
	double   sunDec;  // sun declination     (latitude)
};

#endif // __dayNight_h__

