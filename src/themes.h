/*******************************************************************************
**
**    I place this program under the "GNU General Public License".
**        http://www.gnu.org/licenses/licenses.html#GPL
**
*******************************************************************************/

#ifndef __themes_h__
#define __themes_h__

#include <gtk/gtk.h>
#include "basecpp.h" // some useful macros and functions

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
struct ThemeEntry
{
	GString* pPath;
	GString* pFile;
	GString* pName;
	GString* pAuthor;
	GString* pVersion;
	GString* pInfo;
};

// -----------------------------------------------------------------------------
struct ThemeList;

int         theme_list_get(ThemeList*& tl);
ThemeEntry& theme_list_beg(ThemeList*& tl);
int         theme_list_cnt(ThemeList*& tl);
void        theme_list_del(ThemeList*& tl);
bool        theme_list_end(ThemeList*& tl);
ThemeEntry& theme_list_fnd(ThemeList*& tl, const char* path, const char* file, const char* name, int* index=NULL);
ThemeEntry& theme_list_nth(ThemeList*& tl, int nth);
ThemeEntry& theme_list_nxt(ThemeList*& tl);

#endif // __themes_h__

