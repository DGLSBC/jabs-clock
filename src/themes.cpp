/*******************************************************************************
**
**    I place this program under the "GNU General Public License".
**        http://www.gnu.org/licenses/licenses.html#GPL
**
*******************************************************************************/

/*******************************************************************************
**
** Theme related functions
**
*******************************************************************************/

#undef    DEBUGLOG
#define   DEBUGNSP   "thms"

#include "cfgdef.h"
#include "global.h"
#include "themes.h"
#include "utility.h"
#include "strnatcmp.h"
#include "debug.h"     // for debugging prints


// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
struct ThemeList
{
	GList* pList;
	GList* pCurr;
	int    count;
};

// TODO: must get rid of these
static int  get_theme_list();
static void del_theme_list(GList* pThemeList);

static ThemeEntry g_tleEmpty = { NULL, NULL };

// TODO: must get rid of these
static GList* g_pThemeList = NULL;
static guint  g_themeCount = 0;

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
static int get_theme_list(GList*& pThemes);

// -----------------------------------------------------------------------------
int theme_list_get(ThemeList*& tl)
{
	tl = new ThemeList;
	return tl ? tl->count = get_theme_list(tl->pList) : 0;
}

// -----------------------------------------------------------------------------
ThemeEntry& theme_list_beg(ThemeList*& tl)
{
	if( tl )
		tl->pCurr = g_list_first(tl->pList);
	return tl && tl->pCurr != NULL ? *((ThemeEntry*)tl->pCurr->data) : g_tleEmpty;
}

// -----------------------------------------------------------------------------
ThemeEntry& theme_list_nxt(ThemeList*& tl)
{
	if( tl )
		tl->pCurr = g_list_next(tl->pCurr);
	return tl && tl->pCurr != NULL ? *((ThemeEntry*)tl->pCurr->data) : g_tleEmpty;
}

// -----------------------------------------------------------------------------
bool theme_list_end(ThemeList*& tl)
{
	return tl ? tl->pCurr == NULL : true;
}

// -----------------------------------------------------------------------------
ThemeEntry& theme_list_fnd(ThemeList*& tl, const char* path, const char* file, const char* name, int* index)
{
	DEBUGLOGB;

	if( tl && tl->count && ((path && *path) || (file && *file) || (name && *name)) )
	{
		ThemeEntry* pTE;
		int         idx = 0;
		bool        fnd = false;

		theme_list_beg(tl);

		DEBUGLOGP("comparing %d list entries to matchee of %s, %s, %s\n",
			theme_list_cnt(tl), path ? path : "null", file ? file : "null", name ? name : "null");

		while( !theme_list_end(tl) )
		{
			pTE  = (ThemeEntry*)tl->pCurr->data;

			DEBUGLOGP("comparing list entry %s, %s, %s to matchee\n",
				pTE->pPath ? pTE->pPath->str : "null",
				pTE->pFile ? pTE->pFile->str : "null",
				pTE->pName ? pTE->pName->str : "null");

			fnd  =  true;
			fnd &=  path ? strcmp(path, pTE->pPath->str) == 0 : true;
			fnd &=  file ? strcmp(file, pTE->pFile->str) == 0 : true;
			fnd &=  name ? strcmp(name, pTE->pName->str) == 0 : true;

			if( fnd )
			{
				DEBUGLOGP("found match at index %d\n", idx);

				if(  index )
					*index = idx;

				break;
			}

			theme_list_nxt(tl);
			idx++;
		}
	}

	DEBUGLOGE;
	return tl && tl->pCurr != NULL ? *((ThemeEntry*)tl->pCurr->data) : g_tleEmpty;
}

// -----------------------------------------------------------------------------
ThemeEntry& theme_list_nth(ThemeList*& tl, int nth)
{
	if( tl && nth >= 0 && nth < tl->count )
	{
		theme_list_beg(tl);

		while( --nth >= 0 )
			theme_list_nxt(tl);
	}

	return tl && tl->pCurr != NULL ? *((ThemeEntry*)tl->pCurr->data) : g_tleEmpty;
}

// -----------------------------------------------------------------------------
int theme_list_cnt(ThemeList*& tl)
{
	return tl ? tl->count : 0;
}

// -----------------------------------------------------------------------------
void theme_list_del(ThemeList*& tl)
{
	if( tl )
	{
		del_theme_list(tl->pList);
		delete tl; tl = NULL;
	}
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
int get_theme_list(GList*& pThemes)
{
	GList* pThemeList = g_pThemeList;
	int    themeCount = g_themeCount;

	g_pThemeList = NULL;
	g_themeCount = 0;

	int ret = get_theme_list();

	pThemes = g_pThemeList;

	g_pThemeList = pThemeList;
	g_themeCount = themeCount;

	return ret;
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
//static void delete_entry(gpointer themeEntry, gpointer data);
static void delete_entry(gpointer themeEntry);

static const  char* get_system_theme_path();

static GList* get_theme_list(GString* pSystemPath, GString* pUserPath);
static void   get_theme_list(GString* pPath, GList** ppThemes);
static int    get_theme_list_sort(gconstpointer themeEntry1, gconstpointer themeEntry2);

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
int get_theme_list()
{
//	printf("%s(1): entry\n", __func__);

	if( g_pThemeList )
		del_theme_list(g_pThemeList);

	g_pThemeList = NULL;
	g_themeCount = 0;

	// read in the names of all of the system installed themes as well as
	// any from the user's home directory

//	printf("%s(1): before theme path retrievals\n", __func__);

	const char* upath  = get_user_theme_path  ();
	const char* spath  = get_system_theme_path();
	GString*    gsupth = upath ? g_string_new(upath) : NULL;
	GString*    gsspth = spath ? g_string_new(spath) : NULL;

//	printf("%s(1): after theme path retrievals\n", __func__);
//	printf("%s(1): upath=%s\n", __func__, upath);
//	printf("%s(1): spath=%s\n", __func__, spath);

//	printf("%s(1): before theme list retrieval\n", __func__);

	g_pThemeList = upath ? get_theme_list(gsspth, gsupth) : NULL;

//	printf("%s(1): after theme list retrieval\n", __func__);

	if( upath ) delete []     upath;
	if( gsspth) g_string_free(gsspth, TRUE);
	if( gsupth) g_string_free(gsupth, TRUE);

	GList* pThemeActiv = NULL;
	GList* pThemeEntry = g_list_first(g_pThemeList);

	while( pThemeEntry )
	{
		pThemeEntry = g_list_next(pThemeEntry);
		g_themeCount++;
	}

	ThemeEntry* pEntry = NULL;

	if( pEntry = new ThemeEntry )
	{
		pEntry->pPath    = g_string_new (INTERNAL_THEME);
		pEntry->pFile    = g_string_new (INTERNAL_THEME);
		pEntry->pName    = g_string_new ("<default>");
		pEntry->pAuthor  = g_string_new ("Pillbug");
		pEntry->pVersion = g_string_new ("1.0");
		pEntry->pInfo    = g_string_new ("Internal fallback theme");
		g_pThemeList     = g_list_append(g_pThemeList, (gpointer)pEntry);
		g_themeCount++;
	}

//	printf("%s(1): theme count is %d\n", __func__, g_themeCount);

//	printf("%s(1): exit\n", __func__);

	return g_themeCount;
}

// -----------------------------------------------------------------------------
void del_theme_list(GList* pThemeList)
{
	DEBUGLOGB;

//	if( pThemeList )
//		g_list_foreach(pThemeList, delete_entry, NULL);
	if( pThemeList )
		g_list_free_full(pThemeList, delete_entry);

	DEBUGLOGE;
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
//void delete_entry(gpointer themeEntry, gpointer data)
void delete_entry(gpointer themeEntry)
{
	if( themeEntry )
	{
		ThemeEntry* pTE = (ThemeEntry*)themeEntry;

		DEBUGLOGP("entry deleted for %s\n", pTE->pFile->str);

		if( pTE->pFile )
			g_string_free(pTE->pFile,    TRUE);
		if( pTE->pPath )
			g_string_free(pTE->pPath,    TRUE);
		if( pTE->pName )
			g_string_free(pTE->pName,    TRUE);
		if( pTE->pAuthor )
			g_string_free(pTE->pAuthor,  TRUE);
		if( pTE->pVersion )
			g_string_free(pTE->pVersion, TRUE);
		if( pTE->pInfo )
			g_string_free(pTE->pInfo,    TRUE);

		delete pTE; themeEntry = pTE = NULL;
	}
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
const char* get_system_theme_path()
{
//	return PKGDATA_DIR "/themes";
	return PKGDATA_DIR_OLD "/themes";
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
GList* get_theme_list(GString* pSystemPath, GString* pUserPath)
{
//	printf("%s(2): entry\nspath sent as\n*%s*\nupath sent as\n*%s*\n", __func__, pSystemPath->str, pUserPath->str);

	GList* pThemeList = NULL;

	get_theme_list(pSystemPath, &pThemeList);
	get_theme_list(pUserPath,   &pThemeList);

	if( pThemeList )
	{
		pThemeList = g_list_sort (pThemeList, get_theme_list_sort);
		pThemeList = g_list_first(pThemeList);
	}

//	printf("%s(2): exit\n", __func__);

	return pThemeList;
}

// -----------------------------------------------------------------------------
void get_theme_list(GString* pPath, GList** ppThemes)
{
	DEBUGLOGS("(3) entry");

	if( !pPath )
	{
		DEBUGLOGS("(3) exit(1)");
		return;
	}

//	DEBUGLOGP("(3) pPath sent as \n*%s*\n", pPath->str);

	ThemeEntry* pEntry = NULL;
	GDir*       pDir   = g_dir_open(pPath->str, 0, NULL);

	if( !pDir )
	{
		DEBUGLOGS("(3) exit(2)");
		return;
	}

//	DEBUGLOGS("(3) dir path exists");

	GString* apath = g_string_new(NULL);
	GString* epath = g_string_new(NULL);
	GString* tpath = g_string_new(pPath->str);

	g_string_append_c(tpath, '/');

	while( true )
	{
		const gchar* pName = g_dir_read_name(pDir);

		if( !pName || !(*pName) )
			break;

		if( strcmp(pName, ".") == 0 || strcmp(pName, "..") == 0 )
			continue;

		g_string_assign(epath, tpath->str);
		g_string_append(epath, pName);

		if( !g_file_test(epath->str, G_FILE_TEST_IS_DIR) )
			continue;

//		DEBUGLOGP("(3) got a possible theme sub-dir entry (%s)\n", pName);

		static const char* svgFiles[] = { "face", "hour-hand" };

		bool valid  = false;
		for( size_t f = 0; f < vectsz(svgFiles); f++ )
		{
			g_string_printf(apath, "%s/clock-%s.svg", epath->str, svgFiles[f]);

			if( valid = g_isa_file(apath->str) )
				break;

			g_string_append(apath, "z"); // check for a compressed svg file

			if( valid = g_isa_file(apath->str) )
				break;
		}

		if( !valid || !(pEntry = new ThemeEntry) )
			continue;

//		DEBUGLOGS("(3) making a new theme list entry");

		pEntry->pPath = g_string_new(pPath->str);
		pEntry->pFile = g_string_new(pName);
		pEntry->pName = pEntry->pAuthor = pEntry->pVersion = pEntry->pInfo = NULL;

		g_string_assign(apath, epath->str);
		g_string_append(apath, "/theme.conf");

//		DEBUGLOGP("(3) checking if theme dir has a theme.conf\n(%s)\n", apath->str);

		if( g_isa_file(apath->str) )
		{
//			DEBUGLOGS("(3) theme dir has a theme.conf - is it valid?");

			GKeyFile* pKF = g_key_file_new();

			if( pKF && g_key_file_load_from_file(pKF, apath->str, G_KEY_FILE_NONE, NULL) )
			{
//				DEBUGLOGS("(3) theme dir has a valid theme.conf - processing it");

				const char* str;
				const char* group  =   "Theme";
				const char* keys[] = { "name",         "author",         "version",         "info" };
				GString**   vals[] = { &pEntry->pName, &pEntry->pAuthor, &pEntry->pVersion, &pEntry->pInfo };

				for( size_t k = 0; k < vectsz(keys); k++ )
				{
					if( (str = g_key_file_get_string(pKF, group, keys[k], NULL)) )
					{
//						DEBUGLOGP("(3) key of %s has val of %s\n", keys[k], str);
						*(vals[k]) = g_string_new(str);
						g_free((char*)str);
					}
				}
			}

			if( pKF )
				g_key_file_free(pKF);
		}

		if( pEntry->pName == NULL )
			pEntry->pName =  g_string_new(pName);

		*ppThemes = g_list_append(*ppThemes, (gpointer)pEntry);

		DEBUGLOGP("(3) entry made for %s\n", pName);
	}

	DEBUGLOGS("(3) done making list");

	g_string_free(tpath, TRUE);
	g_string_free(epath, TRUE);
	g_string_free(apath, TRUE);

	g_dir_close(pDir);

	DEBUGLOGS("(3) exit(3)");
}

// -----------------------------------------------------------------------------
int get_theme_list_sort(gconstpointer themeEntry1, gconstpointer themeEntry2)
{
	// TODO: make this more robust by using theme entry file & path strings as well

	const GString* e1  = ((const ThemeEntry*)themeEntry1)->pName;
	const GString* e2  = ((const ThemeEntry*)themeEntry2)->pName;
	const bool     ok  =   e1 && e2 && e1->str && e2->str;
/*	const int      l1  =   ok ?  e1->len  : 0;
	const int      l2  =   ok ?  e2->len  : 0;
	const int      lm  =   l1 <  l2 ? l1  : l2;
	const int      r1  =   ok ?  strnicmp(e1->str, e2->str, lm) : 0;
//	const int      rs  =   r1 != 0  ?  r1 : l1 < l2 ? -1 : (l1 > l2 ? +1 : 0);
	const int      rs  =   r1 != 0  ?  r1 : l1 - l2;
	printf("%s: e1=%s, e2=%s, l1=%d, l2=%d, lm=%d, r1=%d, rs=%d\n",
		   __func__, e1->str, e2->str, l1, l2, lm, r1, rs);
	return rs;
*/
	return ok ? strnatcasecmp(e1->str, e2->str) : 0;
}

