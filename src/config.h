/*******************************************************************************
**
**    I place this program under the "GNU General Public License".
**        http://www.gnu.org/licenses/licenses.html#GPL
**
*******************************************************************************/

#ifndef __config_h__
#define __config_h__

#include "platform.h" // platform specific
#include "basecpp.h"  // some useful macros and functions
#include <limits.h>   // for PATH_MAX

// -----------------------------------------------------------------------------
enum CornerType
{
	CORNER_TOP_LEFT=0, // clock window anchor and offsets from screen corner at top left
	CORNER_TOP_RIGHT,  // clock window anchor and offsets from screen corner at top right
	CORNER_BOT_RIGHT,  // clock window anchor and offsets from screen corner at bottom right
	CORNER_BOT_LEFT,   // clock window anchor and offsets from screen corner at bottom left
	CORNER_CENTER      // clock window anchor and offsets from screen middle
};

// -----------------------------------------------------------------------------
struct Config
{
	int    clockX;              // x pos of topleft corner: < 0 means undefined (default 0)
	int    clockY;              // y pos of topleft corner: < 0 means undefined (default 0)
	int    clockW;              // clock window used width  (default 128)
	int    clockH;              // clock window used height (default 128)
	int    keepOnTop;           // "keep above other windows" toggle (default off)
	int    renderRate;          // frames-per-second render rate (1-60, default 16)
	int    show24Hrs;           // 24h hour hand toggle (default off)
	int    showDate;            // date display toggle  (default off)
	int    showSeconds;         // seconds hand toggle  (default off)
	int    showInPager;         // "show in pager" toggle (default off)
	int    showInTasks;         // "show in taskbar" toggle (default off)
	int    sticky;              // "show on every workspace" toggle (default on)
	char   themeFile[64];       // current clock theme subdir/filename (default <simple>)
	// 0.4.0 added
	int    clockC;              // offset corner: 0-topleft, 1-topright, 2-botleft, 3-botright (default 0)
	double clockR;              // rotation angle, in degrees, from the vertical (default to 0.0)
	int    clockWS;             // "unsticky" workspace #: 0-don't set, >0-workspace # (default 0)
	int    aniStartup;          // animate startup display (default off)
	int    queuedDraw;          // less efficient but flicker-free drawing via gtk's double buffered queued drawing
	int    refSkipCnt;          //
	int    refSkipCur;          //
	int    faceDate;            // date display below clock hands toggle (default off)
	int    keepOnBot;           // "keep below other windows" toggle (default off)
	int    showAlarms;          // alarm hand toggle (default on)
	int    showHours;           // hour hand toggle (default on)
	int    showMinutes;         // minute hand toggle (default on)
	int    showTTips;           // date/time tool tip popup toggle (default off)
	int    shandType;           // type of second hand animation to perform: 0-macslow's clock, 1-flick, 2-sweep, 3-custom (default 1)
	int    clickThru;           //
	int    handsOnly;           //
	int    noDisplay;           //
	int    takeFocus;           //
	int    textOnly;            //
	int    doSounds;            // play quarter-hour chimes or alarm sounds (default off)
	int    doAlarms;            // play alarm sounds (default off)
	int    doChimes;            // play quarter-hour chime sounds (default off)
	char   themePath[PATH_MAX]; // current clock theme parent full path (default "")
	double opacity;             // clock window's opaqueness 0=transparent, 1=opaque (default 1)
	double dateTxtRed;          // shown date text red   channel intensity (default 0.75)
	double dateTxtGrn;          // shown date text green channel intensity (default 0.75)
	double dateTxtBlu;          // shown date text blue  channel intensity (default 0.75)
	double dateTxtAlf;          // shown date text alpha channel intensity (default 1.00)
	double dateShdRed;          // shown date text shadow red   channel intensity (default 0.1)
	double dateShdGrn;          // shown date text shadow green channel intensity (default 0.1)
	double dateShdBlu;          // shown date text shadow blue  channel intensity (default 0.1)
	double dateShdAlf;          // shown date text shadow alpha channel intensity (default 1.0)
	double dateShdWid;          // shown date text shadow outline width (default 0.1)
	double fontOffY;            // shown date text "from center" vertical offset factor (default 1.0)
	int    fontSize;            // shown date text font size in ? units (default 10)
	char   fontFace [PATH_MAX]; // shown date text font file full path (default "")
	char   fontName [64];       //
	char   fmtDate12[64];       // window title date text format (default "%a %b %d") (also shown date text format)
	char   fmtDate24[64];       // window title date text format (default "%a %d %b") (also shown date text format)
	char   fmtTime12[64];       // window title time text format (default "%I:%M %P")
	char   fmtTime24[64];       // window title time text format (default "%H:%M")
	char   fmtTTip12[512];      // shown tooltip text 12 hour format (default "%A%n%B %d %Y%n%I:%M:%S %P")
	char   fmtTTip24[512];      // shown tooltip text 24 hour format (default "%A%n%B %d %Y%n%T")
	char   alarms   [64];       // semi-colon separated daily alarm times in 24-hour-days hourly units
	char   alarmDays[64];       // semi-colon separated daily alarm days-of-the-week (1 char/day, . is no; any other char for yes)
	char   alarmMsgs[512];      // semi-colon separated daily alarm notification messages
	char   alarmLens[64];       // semi-colon separated daily alarm notification message lengths, in seconds
	                            // ordering for the following two is alarm, hour, minute, second
	int    optHand[4];          // optimize hand rendering, i.e., don't use draw shape calls in renderer (default on for all)
	int    useSurf[4];          // render hand using a pre-drawn surface (default on for all but alarm & hour)
	int    pngIcon;             // make PNG icons if on, otherwise they're SVG (default off)
	int    auto24;              // switch # of hours shown mode based on used theme (default on)
#if 0
	double sfTxt1, hfTxt1, haTxt1;
	double sfTxt2, hfTxt2, haTxt2;
	double sfTxt3, hfTxt3, haTxt3;
	double sfTxt4, hfTxt4, haTxt4;
	double sfTxt5, hfTxt5, haTxt5;
	double sfTxt6, hfTxt6, haTxt6;
#else
	double sfTxta1, hfTxta1;
	double sfTxta2, hfTxta2;
	double sfTxta3, hfTxta3;
	double sfTxtb1, hfTxtb1;
	double sfTxtb2, hfTxtb2;
	double sfTxtb3, hfTxtb3;
#endif
	double bkgndRed;            // shown fullscreen/non-composited red   channel intensity (default 0.0)
	double bkgndGrn;            // shown fullscreen/non-composited green channel intensity (default 0.0)
	double bkgndBlu;            // shown fullscreen/non-composited blue  channel intensity (default 0.0)

	// TODO: 'move' these over from Runtime to here, somewhere?
//	int    scrsaver;            //
//	int    maximize;            //
//	int    minimize;            //

	char   tzName   [96];       // unique id
	char   tzShoName[32];       // shown override
	int    tzHorGmtOff;         // hour offset from gmt to local
	int    tzMinGmtOff;         // minute offset
	int    tzHorShoOff;         // hour offset from local to shown
	int    tzMinShoOff;         // minute offset
	double tzLatitude;          // for sunrise/set calc
	double tzLngitude;          // ditto
};

// -----------------------------------------------------------------------------
namespace cfg
{

void init();
void prep(Config& cfgData);

bool load();
bool save(bool lock=false, bool force=false);

void cnvp(Config& cfgData, bool screen, bool lock=false);

} // cfg

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
extern Config gCfg;

#endif // __config_h__

