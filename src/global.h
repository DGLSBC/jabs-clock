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
#include <gtk/gtk.h> // platform specific
#include "config.h"  // for Settings struct
#include "basecpp.h" // some useful macros and functions
#include "themes.h"  // for ThemeList, ThemeEntry definitions

// -----------------------------------------------------------------------------
#define   MIN_CWIDTH        32
#define   MIN_CHEIGHT       32
#define   MIN_REFRESH_RATE   1
#define   MAX_REFRESH_RATE  60
#define   INTERNAL_THEME   "<simple>" // TODO: should be <default> instead?

#define  _USEMTHREADS // working for the most part but not quite perfect yet
#undef   _USEWKSPACE  // working? appears to cause system hangs, so, no

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
enum ClockElement
{
	CLOCK_DROP_SHADOW = 0,
	CLOCK_FACE,
	CLOCK_MARKS,
	CLOCK_MARKS_24H,
	CLOCK_HOUR_HAND_SHADOW,
	CLOCK_MINUTE_HAND_SHADOW,
	CLOCK_SECOND_HAND_SHADOW,
	CLOCK_HOUR_HAND,
	CLOCK_MINUTE_HAND,
	CLOCK_SECOND_HAND,
	CLOCK_FACE_SHADOW,
	CLOCK_GLASS,
	CLOCK_FRAME,
	CLOCK_MASK,
	CLOCK_ELEMENTS
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
	const  gchar* appName;
	const  gchar* appVersion;
	GtkWidget*    pMainWindow;
	guint         drawTimerId;
	guint         infoTimerId;

	bool   appStart;
	bool   appStop;
	bool   appDebug;
	bool   hasParms;
	bool   renderUp;
	bool   renderIt;
	bool   portably;
	bool   scrsaver;
	bool   maximize;
	bool   clickthru;
	bool   nodisplay;
	bool   textonly;
	bool   evalDraws;
	bool   optHorHand;
	bool   optMinHand;
	bool   optSecHand;
	bool   useHorSurf;
	bool   useMinSurf;
	bool   useSecSurf;
	bool   updateSurfs;
	gint   lockDims;
	gint   waitSecs1;
	gint   waitSecs2;
	gint   refreshFrame;
	double drawScaleX;
	double drawScaleY;

	gchar  acDate[64];
	gchar  acTime[64];
	gchar  acTxt1[64]; // 1st line of clock text (offset up from 2nd line and defaults to nothing)
	gchar  acTxt2[64]; // 2nd line of clock text (offset just above center and defaults to nothing)
	gchar  acTxt3[64]; // 3rd line of clock text (offset just below center and defaults to dow, mon, & dom)
	gchar  acTxt4[64]; // 4th line of clock text (offset down from 3rd line and defaults to year)

	gint   hours;
	gint   minutes;
	gint   seconds;
	double angleHour;
	double angleMinute;
	double angleSecond;

	gint   prevWinX;
	gint   prevWinY;
	gint   prevWinW;
	gint   prevWinH;

	double bboxHorHand[4];
	double bboxMinHand[4];
	double bboxSecHand[4];

	tm     timeCtm; // current time as retrieved by the 1/2 sec 'tooltip' timer
};

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
void change_ani_rate(GtkWidget* pWindow, int refreshRate, bool force, bool setgui);

void change_theme(ThemeEntry* pEntry, GtkWidget* pWindow);

const gchar* get_theme_icon_filename(bool check=true, bool update=false);
void         set_window_icon(GtkWidget* pWindow);

void make_theme_icon(GtkWidget* pWindow);

void update_colormap(GtkWidget* pWidget);
void update_input_shape(GtkWidget* pWindow, int width, int height, bool dosurfs, bool domask, bool lock);
void update_ts_info(bool forceTTip=false, bool forceDate=false, bool forceTime=false);
void update_wnd_dim(GtkWidget* pWindow, int width, int height, bool async);
void update_wnd_pos(GtkWidget* pWindow, int x, int y);

gboolean draw_time_handler(GtkWidget* pWidget);
gboolean info_time_handler(GtkWidget* pWidget);

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
extern Runtime  gRun;

#endif // __global_h__

