/*******************************************************************************
**
**    I place this program under the "GNU General Public License".
**        http://www.gnu.org/licenses/licenses.html#GPL
**
*******************************************************************************/

#ifndef __basecpp_h__
#define __basecpp_h__
#include <cstring>

#define   foreach(i, n) for( int i = 0; i < n; i++ )
#define   vectsz(a)     sizeof(a)/sizeof(a[0])

// strvXXX and strnxxx_safe functions based on the idea presented at:
// https://randomascii.wordpress.com/2013/04/03/stop-using-strncpy-already/
//

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
static inline char* strncpy_safe(char* dst, const char* src, size_t lim)
{
	// TODO: change this to something more robust/efficient

	if( lim )
	{
		dst[lim-1] = '\0';
		strncpy(dst, src, lim);
	}

	return dst;
}

// -----------------------------------------------------------------------------
template <size_t dlen>
	char* strvcpy(char (&dest)[dlen], const char* sorc)
{
	return strncpy_safe(dest, sorc, dlen);
}

// -----------------------------------------------------------------------------
static inline char* strvcpy(char* dest, const char* sorc, size_t dlen)
{
	return strncpy_safe(dest, sorc, dlen);
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
static inline char* strncat_safe(char* dst, const char* src, size_t lim)
{
	// TODO: change this to something more robust/efficient

	if( lim )
	{
		dst[lim-1] = '\0';
		strncat(dst, src, lim);
	}

	return dst;
}

// -----------------------------------------------------------------------------
template <size_t dlen>
	char* strvcat(char (&dest)[dlen], const char* sorc)
{
	size_t slen = strlen(dest);
	size_t llen = slen < dlen ? dlen - slen : 0; // slen "should" never be > lim, but could be == ?
	return strncat_safe(dest, sorc, llen);
}

// -----------------------------------------------------------------------------
static inline char* strvcat(char* dest, const char* sorc, size_t dlen)
{
	return strncat_safe(dest, sorc, dlen);
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
#define strncpy 0
#define strncat 0
#define strcpy  0
#define strcat  0

#endif

