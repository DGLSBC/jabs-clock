/*******************************************************************************
**
**    I place this program under the "GNU General Public License".
**        http://www.gnu.org/licenses/licenses.html#GPL
**
*******************************************************************************/

#ifndef __config_h__
#define __config_h__

#include <gtk/gtk.h> // platform specific
#include "basecpp.h" // some useful macros and functions

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
struct Settings
{
	gint   clockX;          // x pos of topleft corner: < 0 means undefined (default 0)
	gint   clockY;          // y pos of topleft corner: < 0 means undefined (default 0)
	gint   clockW;          // clock window used width  (default 128)
	gint   clockH;          // clock window used height (default 128)
	gint   keepOnTop;       // "keep above other windows" toggle (default off)
	gint   refreshRate;     // frames-per-second render rate (1-60, default 16)
//	gint   show12Hrs;       // 12h hour hand toggle (default off)
	gint   show24Hrs;       // 24h hour hand toggle (default off)
	gint   showDate;        // date display toggle  (default off)
	gint   showSeconds;     // seconds hand toggle  (default off)
	gint   showInPager;     // "show in pager" toggle (default off)
	gint   showInTasks;     // "show in taskbar" toggle (default off)
	gint   sticky;          // "show on every workspace" toggle (default on)
	gchar  themeFile[64];   // current clock theme subdir/filename (default <simple>)
	// 0.4.0 added
	gint   clockC;          // offset corner: 0-topleft, 1-topright, 2-botleft, 3-botright (default 0)
	gint   clockWS;         // "unsticky" workspace #: 0-don't set, >0-workspace # (default 0)
	gint   aniStartup;      // animate startup display (default off)
	gint   faceDate;        // date display below clock hands toggle (default off)
	gint   keepOnBot;       // "keep below other windows" toggle (default off)
	gint   showHours;       // hour hand toggle (default on)
	gint   showMinutes;     // minute hand toggle (default on)
	gint   shandType;       // type of second hand animation to perform: 0-cairo-clock, 1-flick, 2-sweep (default 1)
	// TODO: 'move' these over from Runtime to here (and put in appropriate position here)
//	gint   portably;
//	gint   maximize;
//	gint   clickthru;
//	gint   nodisplay;
//	gint   textonly;
	gint   doSounds;        // play quarter-hour chimes or alarm sounds (default off)
	gint   doAlarms;        // play alarm sounds (default off)
	gint   doChimes;        // play quarter-hour chime sounds (default off)
	gchar  themePath[1024]; // current clock theme parent full path (default "")
	gfloat opacity;         // clock window's opaqueness 0=transparent, 1=opaque (default 1)
	gfloat dateTextRed;     // shown date text red   channel intensity (default 0.75)
	gfloat dateTextGrn;     // shown date text green channel intensity (default 0.75)
	gfloat dateTextBlu;     // shown date text blue  channel intensity (default 0.75)
	gfloat dateTextAlf;     // shown date text alpha channel intensity (default 1.00)
	gfloat dateShdoRed;     // shown date text shadow red   channel intensity (default 0.1)
	gfloat dateShdoGrn;     // shown date text shadow green channel intensity (default 0.1)
	gfloat dateShdoBlu;     // shown date text shadow blue  channel intensity (default 0.1)
	gfloat dateShdoAlf;     // shown date text shadow alpha channel intensity (default 1.0)
	gfloat dateShdoWid;     // shown date text shadow outline width (default 0.1)
	gfloat fontOffY;        // shown date text "from center" vertical offset factor (default 1.0)
	gint   fontSize;        // shown date text font size in ? units (default 10)
	gchar  fontFace[1024];  // shown date text font file full path (default "")
	gchar  fontName[64];    //
	gchar  fmtDate [64];    // window title date text format (default "%a %b %d") (also shown date text format)
	gchar  fmtTime [64];    // window title time text format (default "%I:%M %P")
	gchar  fmt12Hrs[64];    // shown tooltip text 12 hour format (default "%A%n%B %d %Y%n%I:%M:%S %P")
	gchar  fmt24Hrs[64];    // shown tooltip text 24 hour format (default "%A%n%B %d %Y%n%T")
};

// -----------------------------------------------------------------------------
namespace cfg
{

void init();
void prep(Settings& cfgData);

bool load();
bool save();

void cnvp(Settings& cfgData, bool screen);

} // cfg

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
extern Settings gCfg;

#endif // __config_h__

