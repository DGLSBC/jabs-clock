/*******************************************************************************
**
**    I place this program under the "GNU General Public License".
**        http://www.gnu.org/licenses/licenses.html#GPL
**
*******************************************************************************/

#undef    DEBUGLOG
#define   DEBUGNSP   "tzn"

// unfortunately, the tzm library is not generic enough to be useful in this case
//#include <timezonemap/tz.h>

#include <stdio.h>   //
#include "tzone.h"   //
#include "utility.h" // for ?
#include "debug.h"   // for debugging prints
#include "tz.h"      // for zone.tab info extracting structs & funcs

// -----------------------------------------------------------------------------
namespace tzn
{

static bool  got(const char* tzcode,    TzLocation& locret, TzInfo& nforet, bool exact);
static bool  got(long        tm_gmtoff, TzLocation& locret, TzInfo& nforet);

static void  get(const TzLocation& loc, const TzInfo& nfo, int* loc_offset, int* utc_offset, double* latitude, double* longitude, char* tzname, size_t tzn_len, char* comment, size_t cmt_len);

#if  0
static char  tzz[64], tzZ[64];
#endif
static int   tzo  =   0;
static TzDB* tzdb =   NULL;

}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
void tzn::beg()
{
	DEBUGLOGB;

	tzn::end();

	tzdb = tz_load_db();

	if( tzdb )
	{
		int    tzh =  65535;
		int    tzm =  65535;
		time_t ct  =  time(NULL);
		tm     lt  = *localtime(&ct);
		tzo        =  lt.tm_gmtoff;
#if  0
		strfmtdt(tzz, sizeof(tzz), "%z", &lt);
		strfmtdt(tzZ, sizeof(tzZ), "%Z", &lt);

		if( *tzz )
		{
			sscanf(tzz, "%3d%2d", &tzh, &tzm);
			tzo =  tzh*3600 + tzm*60; // # of seconds offset from utc
		}

		DEBUGLOGP("tzz=*%s*, tzZ=*%s*, tzh=%d, tzm=%d, tzo=%d\n", tzz, tzZ, tzh, tzm, tzo);
#endif
	}

	DEBUGLOGE;
}

// -----------------------------------------------------------------------------
void tzn::end()
{
	DEBUGLOGB;

	if( tzdb )
		tz_db_free(tzdb);

	 tzdb =  NULL;
#if  0
	*tzz  = *tzZ = '\0';
#endif
	 tzo  =  0;

	DEBUGLOGE;
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
bool tzn::got(const char* tzcode, bool exact)
{
	DEBUGLOGB;
	TzInfo     nfo;
	TzLocation loc;
	bool       ret = got(tzcode, loc, nfo, exact);
	DEBUGLOGE;
	return ret;
}

// -----------------------------------------------------------------------------
bool tzn::got(long tm_gmtoff)
{
	DEBUGLOGB;
	TzInfo     nfo;
	TzLocation loc;
	bool       ret = got(tm_gmtoff, loc, nfo);
	DEBUGLOGE;
	return ret;
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
bool tzn::get(const char* tzcode, int* loc_offset, int* utc_offset, double* latitude, double* longitude, char* tzname, size_t tzn_len, char* comment, size_t cmt_len, bool exact)
{
	DEBUGLOGB;

	TzInfo     nfo;
	TzLocation loc;
	bool       ret = got(tzcode, loc, nfo, exact);

	if( ret )
		get(loc, nfo, loc_offset, utc_offset, latitude, longitude, tzname, tzn_len, comment, cmt_len);

	DEBUGLOGE;
	return ret;
}

// -----------------------------------------------------------------------------
bool tzn::get(long tm_gmtoff, int* loc_offset, int* utc_offset, double* latitude, double* longitude, char* tzname, size_t tzn_len, char* comment, size_t cmt_len)
{
	DEBUGLOGB;

	TzInfo     nfo;
	TzLocation loc;
	bool       ret = got(tm_gmtoff, loc, nfo);

	if( ret )
		get(loc, nfo, loc_offset, utc_offset, latitude, longitude, tzname, tzn_len, comment, cmt_len);

	DEBUGLOGE;
	return ret;
}

// -----------------------------------------------------------------------------
void tzn::get(const TzLocation& loc, const TzInfo& nfo, int* loc_offset, int* utc_offset, double* latitude, double* longitude, char* tzname, size_t tzn_len, char* comment, size_t cmt_len)
{
	DEBUGLOGB;

	if(  loc_offset )
		*loc_offset = nfo.utc_offset - tzo;

	if(  utc_offset )
		*utc_offset = nfo.utc_offset;

	if(  latitude )
		*latitude = loc.latitude;

	if(  longitude )
		*longitude = loc.longitude;

	if(  tzname && tzn_len )
		 strvcpy(tzname, loc.zone ? loc.zone : "", tzn_len);

	if(  comment && cmt_len )
		 strvcpy(comment, loc.comment ? loc.comment : "", cmt_len);

	DEBUGLOGE;
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
bool tzn::got(const char* tzcode, TzLocation& locret, TzInfo& nforet, bool exact)
{
	DEBUGLOGB;
	int  fnd = 0;
	bool ret = false;

	DEBUGLOGP("requested entry tzcode is %s, exact is %s\n", tzcode, exact ? "on" : "off");

	GPtrArray* locs = tz_get_locations(tzdb);

	if( locs )
	{
		char*       tzn = tz_info_get_clean_name(tzdb, tzcode);
		const char* tzs = tzn ? tzn : (!exact ? tzcode : NULL);

		if( tzs )
		{
			DEBUGLOGP("requested entry tzcode (cleaned) is %s\n", tzs);

			TzInfo*     nfo;
			TzLocation* loc;
			bool        got;
			bool        cont_ok,  city_ok;
			const char* cont_db, *city_db;
			const char* cont_in = strchr (tzs, '/'); // end of continent string for search string
			const char* city_in = strrchr(tzs, '/'); // begin of city string for search string
			int         cont_il = cont_in ? cont_in-tzs     : 0;
			int         city_il = city_in ? strlen(city_in) : 0;

			DEBUGLOGP("cont_in is %s\n", cont_in);
			DEBUGLOGP("city_in is %s\n", city_in);

			if( !exact || (exact && g_quark_try_string(tzs)) ) // NOTE: according to tz.c comments, this will work
			{
				for( size_t i = 0; i < locs->len; i++ )
				{
					if( !(loc   = (TzLocation*)locs->pdata[i]) ) continue;
					if( !(nfo   =  tz_info_from_location(loc)) ) continue;

					if( !(got   =  stricmp(loc->zone, tzs) == 0) && !exact )
					{
						cont_db =  strchr (loc->zone, '/'); // end of continent string in db
						city_db =  strrchr(loc->zone, '/'); // begin of city string in db

						if( cont_db != city_db )
						{
							// just compare the 1st and last partitions, i.e., continent and city,
							// disregarding the 'middle' part, i.e., country or state

							DEBUGLOGS("tzcode (cleaned) not found and exact is off");
							DEBUGLOGS("comparing just the continent and city strings");
							DEBUGLOGP("cont_db is %s\n", cont_db);
							DEBUGLOGP("city_db is %s\n", city_db);

							cont_ok = cont_in && cont_db && cont_il == (cont_db-loc->zone) && strnicmp(tzs,     loc->zone, cont_il) == 0;
							city_ok = city_in && city_db && city_il == strlen(city_db)     && strnicmp(city_in, city_db,   city_il) == 0;
							got     = cont_ok && city_ok;

							DEBUGLOGP("cont_ok is %s, city_ok is %s\n", cont_ok ? "on" : "off", city_ok ? "on" : "off");
						}
					}

					if( got )
					{
//						DEBUGLOGP("%s/%s/%s\n", loc->country, loc->zone, loc->comment);
						DEBUGLOGP("%s\n", loc->zone);
						locret = *loc;
						nforet = *nfo;
						ret    = true;
						fnd++;
					}

					tz_info_free(nfo);

					if( fnd > 1 ) break;
//					if( ret ) break;
				}
			}
#ifdef DEBUGLOG
			else
				DEBUGLOGS("requested entry tzcode (cleaned) not found");
#endif
		}

		if( tzn )
			g_free(tzn);
	}

	DEBUGLOGE;
	return ret && fnd == 1;
}

// -----------------------------------------------------------------------------
bool tzn::got(long tm_gmtoff, TzLocation& locret, TzInfo& nforet)
{
	DEBUGLOGB;
	int  fnd = 0;
	bool ret = false;

	DEBUGLOGP("requested entry gmt offset is %d\n", (int)tm_gmtoff);

	GPtrArray* locs = tz_get_locations(tzdb);

	if( locs )
	{
		TzInfo*     nfo;
		TzLocation* loc;

		for( size_t i = 0; i < locs->len; i++ )
		{
			if( loc = (TzLocation*)locs->pdata[i] )
			{
				if( nfo = tz_info_from_location(loc) )
				{
					if( nfo->utc_offset == tm_gmtoff )
					{
						DEBUGLOGP("%s/%s/%s\n", loc->country, loc->zone, loc->comment);
						locret = *loc;
						nforet = *nfo;
						ret    = true;
						fnd++;
					}

					tz_info_free(nfo);

					if( fnd > 1 ) break;
//					if( ret ) break;
				}
			}
		}
	}

	DEBUGLOGE;
	return ret && fnd == 1;
}

// -----------------------------------------------------------------------------
GPtrArray* tzn::getLocs()
{
	DEBUGLOGB;
	GPtrArray* locs = tz_get_locations(tzdb);
	DEBUGLOGE;
	return     locs;
}

