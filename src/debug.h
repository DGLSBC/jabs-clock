/*******************************************************************************
**
**    I place this program under the "GNU General Public License".
**        http://www.gnu.org/licenses/licenses.html#GPL
**
*******************************************************************************/

#ifndef __debug_h__
#define __debug_h__

#undef    DEBUGLOGALL

// -----------------------------------------------------------------------------
void DEBUGLOGL(bool force, const char* nsp, const char* fnc, const char* fmt, ...);
void DEBUGPAUZ(bool force, int msecs, const char* nsp, const char* fnc, const char* log=0);

#if       defined(DEBUGLOG) || defined(DEBUGLOGALL)

#define   DEBUGLOGP(fmt, args...)   DEBUGLOGL(false, DEBUGNSP, __func__, fmt, ##args)
#define   DEBUGLOGS(str)            DEBUGLOGP("%s\n", str)
#define   DEBUGLOGB                 DEBUGLOGS("entry")
#define   DEBUGLOGR(nbr)            DEBUGLOGP("return (%d)\n", nbr)
#define   DEBUGLOGE                 DEBUGLOGS("exit")
#define   DEBUGLOGZ(log, mss)       DEBUGPAUZ(false, mss, DEBUGNSP, __func__, log)

#else  // DEBUGLOG

#define   DEBUGLOGP(fmt, args...)
#define   DEBUGLOGS(str)
#define   DEBUGLOGB
#define   DEBUGLOGR(nbr)
#define   DEBUGLOGE
#define   DEBUGLOGZ(log, mss)

#endif // DEBUGLOG

#define   DEBUGLOGPF(fmt, args...)  DEBUGLOGL(true, DEBUGNSP, __func__, fmt, ##args)
#define   DEBUGLOGSF(str)           DEBUGLOGPF("%s\n", str)
#define   DEBUGLOGBF                DEBUGLOGSF("entry")
#define   DEBUGLOGRF(nbr)           DEBUGLOGPF("return (%d)\n", nbr)
#define   DEBUGLOGEF                DEBUGLOGSF("exit")
#define   DEBUGLOGZF(log, mss)      DEBUGPAUZ(true, mss, DEBUGNSP, __func__, log)

#endif // __debug_h__

