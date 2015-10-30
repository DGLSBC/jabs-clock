/*******************************************************************************
**
**    I place this program under the "GNU General Public License".
**        http://www.gnu.org/licenses/licenses.html#GPL
**
*******************************************************************************/

#ifndef __utility_h__
#define __utility_h__

#include "basecpp.h" // some useful macros and functions

// -----------------------------------------------------------------------------
void  g_make_nicer(int inc);

void  g_main_beg(bool block);
void  g_main_end();
bool  g_main_looped();
bool  g_main_pump(bool block);

int   g_del_dir (const char* path);
int   g_isa_dir (const char* path);
int   g_isa_file(const char* path);

// these returns all need to be delete []'d
const char* get_home_subpath(const char* sdpath, int splen);
const char* get_user_appnm_path();
const char* get_user_theme_path();

//gboolean is_power_of_two(gint iValue); // no longer needed

#ifdef _SETMEMLIMIT
void  setmemlimit();
#endif

void  strcrep(char* s, char f, char r);

#ifndef   stricmp
#define mystricmp
int   stricmp(const char* s1, const char* s2);
#endif

#ifndef   strnicmp
#define mystrnicmp
int   strnicmp(const char* s1, const char* s2, size_t cnt);
#endif

#endif // __utility_h__

