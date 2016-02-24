/*******************************************************************************
**
**    I place this program under the "GNU General Public License".
**        http://www.gnu.org/licenses/licenses.html#GPL
**
*******************************************************************************/

#ifndef __draw_h__
#define __draw_h__

#include <cairo.h>    // for drawing primitives
#include "platform.h" // platform specific
#include "basecpp.h"  // some useful macros and functions
#include "themes.h"   // for ThemeEntry

#define  _USEREFRESHER

// -----------------------------------------------------------------------------
namespace draw
{

enum AnimType
{
	ANIM_ORIG = 0, // MacSlow's clock animation
	ANIM_FLICK,    // my curve from watching an analog clock for a bit
	ANIM_SWEEP,    // continuous/smooth/swept movement
	ANIM_CUSTOM,   // user-created animation curve
	ANIM_COUNT
};

enum ClockElement
{
	CLOCK_DROP_SHADOW = 0,
	CLOCK_FACE,
	CLOCK_MARKS,
	CLOCK_MARKS_24H,
	CLOCK_ALARM_HAND_SHADOW,
	CLOCK_ALARM_HAND,
	CLOCK_HOUR_HAND_SHADOW,
	CLOCK_HOUR_HAND,
	CLOCK_MINUTE_HAND_SHADOW,
	CLOCK_MINUTE_HAND,
	CLOCK_SECOND_HAND_SHADOW,
	CLOCK_SECOND_HAND,
	CLOCK_FACE_SHADOW,
	CLOCK_GLASS,
	CLOCK_FRAME,
	CLOCK_MASK,
	CLOCK_ELEMENTS
};

typedef void (*UpdateThemeDone)(ppointer data, const ThemeEntry& te, bool valid);

// -----------------------------------------------------------------------------
void init();
void grab();

void beg(bool init);
void chg();
void end(bool init=false);

void lock(bool lock);

bool make_icon(const char* iconPath, bool full=false);

bool make_mask(PWidget* pWindow, int width, int height, bool shaped, bool input, bool lock);

int  render(                   bool clear=true, bool lock=false);
int  render(PWidget* pWidget,  bool clear=true, bool lock=false);
int  render(cairo_t* pContext, bool clear=true);

void render_set(PWidget* pWidget=NULL, bool full=true, bool lock=true, int clockW=0, int clockH=0);

void reset_ani  ();
void reset_hands();

void update_ani();
void update_bkgnd(bool yield=false);
void update_date_surf(bool toggle=false, bool yield=false, bool paths=false, bool txtchg=false);
void update_marks(bool yield=false);

void update_surfs(PWidget* pWidget, int width, int height, bool yield);
void update_surfs_swap();

bool update_theme(const ThemeEntry& te, UpdateThemeDone callBack, ppointer cbData);

#ifdef _USEREFRESHER
extern double   refreshTime[];
extern double   refreshFrac[];
extern double   refreshCumm[];
extern int      refreshCount;
#endif

extern PWidget* g_angChart;
extern bool     g_angChartDo;
extern int      g_angChartW;
extern int      g_angChartH;
extern double   g_angAdjX;
extern double   g_angAdjY;

} // namespace draw

#endif // __draw_h__

