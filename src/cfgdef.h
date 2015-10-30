/*******************************************************************************
**
**    I place this program under the "GNU General Public License".
**        http://www.gnu.org/licenses/licenses.html#GPL
**
*******************************************************************************/

#ifndef __cfgdef_h__
#define __cfgdef_h__

#define  _RELEASE
#undef   _RELEASE

#define  _DEBUG
#undef   _DEBUG

#define APP_NAME            "jabs-clock"
#define APP_NAME_OLD        "cairo-clock"
#define GETTEXT_PACKAGE      APP_NAME
#define GETTEXT_PACKAGE_OLD  APP_NAME_OLD
#define DATA_DIR            "/usr/share"
#define PKGDATA_DIR          DATA_DIR "/" GETTEXT_PACKAGE
#define PKGDATA_DIR_OLD      DATA_DIR "/" GETTEXT_PACKAGE_OLD
#define APP_NAMELOCALEDIR    DATA_DIR "/locale"

#endif // __cfgdef_h__

