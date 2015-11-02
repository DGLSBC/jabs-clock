/*******************************************************************************
**
**    I place this program under the "GNU General Public License".
**        http://www.gnu.org/licenses/licenses.html#GPL
**
*******************************************************************************/

#ifndef __draw_h__
#define __draw_h__

#include <gtk/gtk.h> // platform specific
#include "basecpp.h" // some useful macros and functions

// -----------------------------------------------------------------------------
enum AnimType
{
	ANIM_ORIG = 0, // MacSlow's cairo-clock animation
	ANIM_FLICK,    // my curve from watching an analog clock for a bit
	ANIM_SWEEP,    // continuous/smooth/swept movement
	ANIM_CUSTOM    // user-created animation curve
};

// -----------------------------------------------------------------------------
namespace draw
{

typedef void (*UpdateThemeDone)(gpointer data, const char* path, const char* file, bool valid);

void init();
void beg(bool init);
void chg();
void end(bool init=false);

bool make_icon(const char* iconPath);

GdkBitmap* make_mask(int width, int height, bool shaped);

void render(GtkWidget* pWidget, bool   forceDraw);
void render(GtkWidget* pWidget, double scaleX, double scaleY, bool renderIt, bool appStart);
void render(GtkWidget* pWidget, double scaleX, double scaleY, bool renderIt, bool appStart, int width, int height, bool animate);

void reset_ani();

void update_ani();
void update_bkgnd();
void update_date_surf();
void update_surfs(GtkWidget* pWidget, int width, int height);
void update_surfs_swap(int width, int height);

bool update_theme(const char* path, const char* name, UpdateThemeDone callBack, gpointer cbData);

extern double refreshTime[];
extern double refreshFrac[];
extern double refreshCumm[];
extern int    refreshCount;

}

#endif // __draw_h__

