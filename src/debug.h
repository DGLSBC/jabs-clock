/*******************************************************************************
**
**    I place this program under the "GNU General Public License".
**        http://www.gnu.org/licenses/licenses.html#GPL
**
*******************************************************************************/

#ifndef __debug_h__
#define __debug_h__

// -----------------------------------------------------------------------------
void DEBUGLOGL(bool force, const char* nsp, const char* fnc, const char* fmt, ...);
/*
#define   DEBUGLOGON( nsp) \
#define   DEBUGNSP    nsp  \
#define   DEBUGLOG

#define   DEBUGLOGOFF(nsp) \
#define   DEBUGNSP    nsp  \
#undef    DEBUGLOG
*/
#ifdef    DEBUGLOG

#define   DEBUGLOGP(fmt, args...)   DEBUGLOGL(false,  DEBUGNSP, __func__, fmt, ##args)
#define   DEBUGLOGS(str)            DEBUGLOGP("%s\n", str)
#define   DEBUGLOGB                 DEBUGLOGS("entry")
#define   DEBUGLOGE                 DEBUGLOGS("exit")

#else  // DEBUGLOG

#define   DEBUGLOGP(fmt, args...)
#define   DEBUGLOGS(str)
#define   DEBUGLOGB
#define   DEBUGLOGE

#endif // DEBUGLOG

#define   DEBUGLOGF(fmt, args...)   DEBUGLOGL(true,   DEBUGNSP, __func__, fmt, ##args)

#endif

