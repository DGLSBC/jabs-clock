/*******************************************************************************
**
**    I place this program under the "GNU General Public License".
**        http://www.gnu.org/licenses/licenses.html#GPL
**
*******************************************************************************/

#undef    DEBUGLOG
#define   DEBUGNSP   "util"

#include <stdio.h>   // for file functions
#include <unistd.h>  // for file functions
#include <gtk/gtk.h>

#include "cfgdef.h"  // for APP_NAME
#include "utility.h"
#include "debug.h"   // for debugging prints

#define _USEDELDIRNEW
#ifdef  _USEDELDIRNEW
// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
#include <ftw.h>

static int remfd(const char* path, const struct stat* s, int flag, struct FTW* f);

// -----------------------------------------------------------------------------
int g_del_dir(const char* path)
{
	// 32 is arbitrarily small for this app
	return nftw(path, remfd, 32, FTW_DEPTH | FTW_MOUNT | FTW_PHYS);
}

// -----------------------------------------------------------------------------
int remfd(const char* path, const struct stat* s, int flag, struct FTW* f)
{
	return flag == FTW_DP ? rmdir(path) : unlink(path);
}

#else // _USEDELDIRNEW

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
#include <limits.h> // for PATH_MAX

int g_del_dir(const char* path)
{
	DIR* dp = opendir(path);

	if( dp )
	{
		dirent* ep;
		char    pb[PATH_MAX];

		while( (ep = readdir(dp)) != NULL )
		{
			if( strcmp(ep->d_name, ".") != 0 && strcmp(ep->d_name, "..") != 0 )
			{
				strvcpy(pb, path);
				strvcat(pb, ep->d_name);

				if( g_is_dir(pb) )
				{
					strvcat  (pb, "/");
					g_del_dir(pb);
				}
				else
					unlink(pb);
			}
		}

		closedir(dp);
	}

	return rmdir(path);
}

#endif // _USEDELDIRNEW

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
#include <sys/stat.h>

// -----------------------------------------------------------------------------
int g_isa_dir(const char* path)
{
	struct stat        st;
	return stat(path, &st) ? 0 : S_ISDIR(st.st_mode);
}

// -----------------------------------------------------------------------------
int g_isa_file(const char* path)
{
	struct stat        st;
	return stat(path, &st) ? 0 : S_ISREG(st.st_mode);
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
const char* get_home_subpath(const char* sdpath, int splen)
{
	const char* hompth = (const char*)g_get_home_dir();
	const int   homlen = strlen(hompth);
	const int   pthlen = homlen + splen + 2;
	char*       pthstr = new char[pthlen];

	if( pthstr )
	{
		strvcpy(pthstr, hompth, pthlen);        // TODO: these are okay?
		strvcat(pthstr, sdpath, pthlen-homlen); //
	}

	return pthstr;
}

// -----------------------------------------------------------------------------
const char* get_user_appnm_path()
{
	static const char*      gtpath = "/." APP_NAME;
	return get_home_subpath(gtpath, strlen(gtpath));
}

// -----------------------------------------------------------------------------
const char* get_user_theme_path()
{
	static const char*      tdpath = "/." APP_NAME "/themes";
	return get_home_subpath(tdpath, strlen(tdpath));
}
/*
// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
gboolean is_power_of_two(gint iValue)
{
	gint	 iExponent = 0;
	gboolean bResult   = FALSE;

	for( iExponent = 1; iExponent <= 32; iExponent++ )
	{
		if( (unsigned long)iValue - (1L << iExponent) == 0 )
		{
			bResult = TRUE;
			break;
		}
	}

	return bResult;
}*/

#ifdef _SETMEMLIMIT
// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
#include <sys/time.h>
#include <sys/resource.h>

// -----------------------------------------------------------------------------
void setmemlimit()
{
	rlimit  rl;
	rl.rlim_cur = rl.rlim_max = 1024*1024*64;
	setrlimit(RLIMIT_AS, &rl);
}
#endif // _SETMEMLIMIT

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
void strcrep(char* s, char f, char r)
{
	if( s )
	{
		char*   p = s;
		while( (p = strchr(p, f)) )
			   *p = r;
	}
}

#ifdef mystricmp
// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
int stricmp(const char* s1, const char* s2)
{
	return g_ascii_strcasecmp(s1, s2);
}
#endif // mystricmp

#ifdef mystrnicmp
// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
int strnicmp(const char* s1, const char* s2, size_t cnt)
{
	return g_ascii_strncasecmp(s1, s2, cnt);
}
#endif // mystrnicmp

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
void g_make_nicer(int inc)
{
	nice(inc);
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
static bool g_main_looping = false;
static bool g_main_blocker = false;

// -----------------------------------------------------------------------------
void g_main_beg(bool block)
{
	DEBUGLOGB;

	g_main_looping = true;
	g_main_blocker = block;

	if( block )
	{
		DEBUGLOGS("bef entering main loop");
		gtk_main();
		DEBUGLOGS("aft entering main loop");

		g_main_blocker = false;
	}

	DEBUGLOGE;
}

// -----------------------------------------------------------------------------
void g_main_end()
{
	DEBUGLOGB;

	g_main_looping = false;

	if( g_main_blocker )
	{
		DEBUGLOGS("bef quitting main loop");
		gtk_main_quit();
		DEBUGLOGS("aft quitting main loop");
	}

	DEBUGLOGE;
}

// -----------------------------------------------------------------------------
bool g_main_looped()
{
	return g_main_looping;
}

// -----------------------------------------------------------------------------
bool g_main_pump(bool block)
{
//	while( gtk_events_pending() )
	{
//		gtk_main_iteration();
		gtk_main_iteration_do(FALSE);
	}

	return g_main_looping;
}

/*******************************************************************************
**
** Miscellani for possible use later
**
*******************************************************************************/
/*
// -----------------------------------------------------------------------------
/usr/share/linuxmint/mintinstall/installed/cairo-clock.png
/usr/share/linuxmint/mintinstall/icons/cairo-clock.png
/usr/share/pixmaps/cairo-clock.png
/usr/share/icons/Mint-X/apps/48/cairo-clock.png
/usr/share/icons/Mint-X/apps/16/cairo-clock.png
/usr/share/icons/Mint-X/apps/22/cairo-clock.png
/usr/share/icons/Mint-X/apps/32/cairo-clock.png
/usr/share/icons/Mint-X/apps/24/cairo-clock.png

// -----------------------------------------------------------------------------
time_t uptime()
{
	time_t ut = 0;
	File*  fp = fopen("/proc/uptime", "r");

	if( fp != NULL )
	{
		int   res;
		char  buf[BUFSIZ];
		char* b = fgets(buf, BUFSIZ, fp);

		if( b == buf )
		{
			// the following sscanf must use the C locale.

			setlocale(LC_NUMERIC, "C");
			res = sscanf(buf, "%lf", &upsecs);
			setlocale (LC_NUMERIC, "");

			if( res == 1 )
				ut = (time_t)upsecs;
		}

		fclose(fp);
	}

	return ut;
}

// -----------------------------------------------------------------------------
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int main()
{
	FILE* uptimefile;
	char  uptime_chr[28];
	long  uptime = 0;

	if( (uptimefile = fopen("/proc/uptime", "r")) == NULL )
		perror("supt"), exit(EXIT_FAILURE);

	fgets (uptime_chr, 12, uptimefile);
	fclose(uptimefile);

	uptime = strtol(uptime_chr, NULL, 10);

	printf("System up for %ld seconds, %ld hours\n", uptime, uptime / 3600);

	exit(EXIT_SUCCESS);
}
*/

