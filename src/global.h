/*******************************************************************************
**
**    I place this program under the "GNU General Public License".
**        http://www.gnu.org/licenses/licenses.html#GPL
**
*******************************************************************************/

#ifndef __global_h__
#define __global_h__

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
#include "platform.h" // platform specific
#include <limits.h>   // for PATH_MAX
#include "basecpp.h"  // some useful macros and functions
#include "themes.h"   // for ThemeList, ThemeEntry definitions

// -----------------------------------------------------------------------------
#define  _USEMTHREADS // working for the most part but not quite perfect yet

#define   MIN_CLOCKW         16
#define   MIN_CLOCKH          MIN_CLOCKW
#define   MIN_REFRESH_RATE    1
#define   MAX_REFRESH_RATE   60
#define   MAX_REFRESH_RATE2 300
#define   INTERNAL_THEME   "<simple>" // TODO: should be <default> instead?

// TODO:  move main funcs that use these to global?

#define   drawPrio (G_PRIORITY_HIGH)
#define   waitPrio (G_PRIORITY_HIGH_IDLE)
#define   infoPrio (G_PRIORITY_DEFAULT)

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
enum ClockHotKey
{
	HOTKEY_HELP       = '?',
	HOTKEY_TWELVE     = '2',
	HOTKEY_TWENTYFOUR = '4',
	HOTKEY_ALARMS     = 'a',
	HOTKEY_ABOVEALL   = 'A',
	HOTKEY_TASKBAR    = 'b',
	HOTKEY_BELOWALL   = 'B',
	HOTKEY_CENTER     = 'c',
	// C
	HOTKEY_DATE       = 'd',
	// D
	// e
	HOTKEY_EVALDRAWS  = 'E',
	HOTKEY_FACEDATE   = 'f',
	HOTKEY_FOCUS      = 'F',
	HOTKEY_SCRNGRAB1  = 'g', // app-generated
	HOTKEY_SCRNGRAB2  = 'G', // system-generated
	// h
	HOTKEY_HANDSONLY  = 'H',
	HOTKEY_STICKY     = 'i',
	// I
	// j
	// J
	HOTKEY_CLICKTHRU  = 'k',
	HOTKEY_NODISPLAY  = 'K',
	// l
	// L
	HOTKEY_MINIMIZE   = 'm',
	HOTKEY_MAXIMIZE   = 'M',
	// n
	// N
	HOTKEY_OPACITYDN  = 'o',
	HOTKEY_OPACITYUP  = 'O',
	HOTKEY_PAGER      = 'p',
	// P
	HOTKEY_QUIT1      = 'q',
//	HOTKEY_QUIT2      = 'Q',
	HOTKEY_DRAWQUEUED = 'Q',
	HOTKEY_REFRESHDN  = 'r',
	HOTKEY_REFRESHUP  = 'R',
	HOTKEY_SECONDS    = 's',
	HOTKEY_SQUAREUP   = 'S',
	// t
	HOTKEY_TEXTONLY   = 'T',
	// u
	// U
	HOTKEY_PASTE      = 'v'  // lowercase since only used with ctrl w/o shift
	// V
	// w
	// W
	// x
	// X
	// y
	// Y
	// z
	// Z
};

// -----------------------------------------------------------------------------
enum ClockSizeType
{
	SIZE_SMALL = 0,
	SIZE_MEDIUM,
	SIZE_LARGE,
	SIZE_CUSTOM
};

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
struct Runtime
{
	const   char* appName;
	const   char* appVersion;
	PWidget*      pMainWindow;
	unsigned int  drawTimerId;
	unsigned int  infoTimerId;
	char          appHome[PATH_MAX];

	bool   appStart;
	bool   appStop;
	bool   appDebug;
	bool   cfgSaves;
	bool   composed;
	bool   hasParms;
//	bool   marcoCity;      // TODO: no longer needed?
	bool   isMousing;      // set true on clock window's enter-notify, falsse on leave-notify

	bool   evalDraws;
	bool   maximize;
	bool   minimize;
	bool   portably;
	bool   scrsaver;
	bool   squareUp;

	int    lockDims;
	int    lockOffs;

	bool   renderUp;
	bool   renderIt;
	bool   updating;
	bool   updateSurfs;
	int    renderFrame;
	int    animWindW;
	int    animWindH;
	double animScale;
	double drawScaleX;
	double drawScaleY;
	int    waitSecs1;
	int    waitSecs2;

	char   ttlDateTxt[64]; // title date
	char   ttlTimeTxt[64]; // title time
	char   riseSetTxt[64]; // sunrise/sunset

	char   cfaceTxta3[64]; // 1st line of clock face text (3rd above center - defaults to nothing)
	char   cfaceTxta2[64]; // 2nd line of clock face text (2nd above center - defaults to nothing)
	char   cfaceTxta1[64]; // 3nd line of clock face text (1st above center - defaults to nothing)
	char   cfaceTxtb1[64]; // 4th line of clock face text (1st below center - defaults to dow, mon, & dom (12/24 locale aware))
	char   cfaceTxtb2[64]; // 5th line of clock face text (2nd below center - defaults to year)
	char   cfaceTxtb3[64]; // 6th line of clock face text (3rd below center - defaults to timezone)

	int    alhors;         // next (currently timed) alarm event's clock hour
	int    almins;         // ditto but for the minute
	char   almsg[256];     // the notification message to send
	char   allen[4];       // the display time of the message in seconds

	int    hours;          // current time's hour, minutes past the hour, and seconds past the minute
	int    minutes;        //
	int    seconds;        //

	double angleAlm;
	double angleHor;
	double angleMin;
	double angleSec;
	double secDrift;

	int    prevWinX;
	int    prevWinY;
	int    prevWinW;
	int    prevWinH;

	double bboxAlmHand[4];
	double bboxHorHand[4];
	double bboxMinHand[4];
	double bboxSecHand[4];

	tm     timeCtm;        // current time as retrieved by the 1/2 sec 'tooltip' timer
	tm     timeBtm;        // previous current timeCtm
};

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
void alarm_set(bool force=false, bool update=false);

void change_ani_rate(PWidget* pWindow, int renderRate, bool force, bool setgui);

#if _USEGTK
void change_cursor  (PWidget* pWindow, GdkCursorType type);
#endif

void change_theme   (PWidget* pWindow, const ThemeEntry& te, bool doSurfs);

void docks_send_update();

#if _USEGTK
void get_cursor_pos (PWidget* pWidget, int& x, int& y, GdkModifierType& m);
#endif

const char* get_theme_icon_filename(bool check=true, bool update=false);

void        make_theme_icon(PWidget* pWindow);
void        set_window_icon(PWidget* pWindow);

gboolean draw_anim_timer(PWidget* pWindow);
gboolean info_time_timer(PWidget* pWindow);

void update_colormap(PWidget* pWindow);
void update_shapes  (PWidget* pWindow, int width, int height, bool dosurfs, bool domask, bool lock);

bool update_ts(bool force, bool* newHour=NULL, bool* newMin=NULL);

void update_ts_info(PWidget* pWindow, bool forceTTip=false, bool forceDate=false, bool forceTime=false);

bool update_ts_text(bool newHour);

void update_wnd_dim(PWidget* pWindow, int width, int height, int clockW, int clockH, bool async, bool updgui=true);
void update_wnd_pos(PWidget* pWindow, int x, int y);

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
extern Runtime gRun;

#endif // __global_h__

