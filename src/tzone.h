/*******************************************************************************
**
**    I place this program under the "GNU General Public License".
**    If you don't know what that means take a look a here...
**
**               http://www.gnu.org/licenses/licenses.html#GPL
**
*******************************************************************************/

#ifndef __tzone_h__
#define __tzone_h__

#include "basecpp.h" // some useful macros and functions
#include "tz.h"      // for TzLocation declaration

// -----------------------------------------------------------------------------
namespace tzn
{

typedef TzLocation loc_t;

void beg();
void end();

bool got(const char* tzcode, bool exact=true);
bool got(long        tm_gmtoff);

bool get(const char* tzcode,    int* loc_offset, int* utc_offset, double* latitude=NULL, double* longitude=NULL, char* tzname=NULL, size_t tzn_len=0, char* comment=NULL, size_t cmt_len=0, bool exact=true);
bool get(long        tm_gmtoff, int* loc_offset, int* utc_offset, double* latitude=NULL, double* longitude=NULL, char* tzname=NULL, size_t tzn_len=0, char* comment=NULL, size_t cmt_len=0);

GPtrArray* getLocs();

} // tzn

#endif // __tzone_h__

