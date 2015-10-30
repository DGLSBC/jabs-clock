/*******************************************************************************
**
**    I place this program under the "GNU General Public License".
**        http://www.gnu.org/licenses/licenses.html#GPL
**
*******************************************************************************/

#ifndef __loadTheme_h__
#define __loadTheme_h__

#include "basecpp.h" // some useful macros and functions

// -----------------------------------------------------------------------------
bool loadTheme(const char* theme_file_path, float& iwidth, float& iheight, float* bbox);

bool loadTheme(const char* theme_archive_file_path, char* theme_dir=NULL, int theme_dir_size=0, char* theme_fnm=NULL, int theme_fnm_size=0);

#endif // __loadTheme_h__

