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
#define   DEBUGNSP     "thms"

#include <glib.h>      // for ?

#include "themes.h"    //
#include "cfgdef.h"    // for ?
#include "global.h"    // for ?
#include "utility.h"   // for ?
#include "strnatcmp.h" // for ?
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
void theme_ntry_cpy(ThemeEntry& td, const ThemeEntry& ts)
{
	const
	GString*  strs[] = {  ts.pPath,       ts.pFile,       ts.pName,   ts.pAuthor,
						  ts.pVersion,    ts.pInfo,       ts.pModes,
						  ts.pTextColor,  ts.pShdoColor,  ts.pFillColor };
	GString** strd[] = { &td.pPath,      &td.pFile,      &td.pName,  &td.pAuthor,
						 &td.pVersion,   &td.pInfo,      &td.pModes,
						 &td.pTextColor, &td.pShdoColor, &td.pFillColor };

	for( size_t s = 0; s < vectsz(strs); s++ )
		*strd[s]  = g_string_new(strs[s] && strs[s]->str ? strs[s]->str : "");
}

// -----------------------------------------------------------------------------
void theme_ntry_del(ThemeEntry& te)
{
	GString*  strs[] = {  te.pPath,       te.pFile,       te.pName,   te.pAuthor,
						  te.pVersion,    te.pInfo,       te.pModes,
						  te.pTextColor,  te.pShdoColor,  te.pFillColor };

	for( size_t s = 0; s < vectsz(strs); s++ )
	{
		if( strs[s] )
			g_string_free(strs[s], TRUE);
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

static GList* get_theme_list(GString* pSystemPath, GString* pUserPath1, GString* pUserPath2);
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

	const char* upath  = get_user_theme_path();
	GString*    gsupth = upath ? g_string_new(upath) : NULL;

	const char* spath  = get_system_theme_path();
	GString*    gsspth = spath ? g_string_new(spath) : NULL;

	const char* opath  = get_user_old_theme_path();
	GString*    gsopth = opath ? g_string_new(opath) : NULL;

//	printf("%s(1): after theme path retrievals\n", __func__);
//	printf("%s(1): upath=%s\n", __func__, upath);
//	printf("%s(1): spath=%s\n", __func__, spath);
//	printf("%s(1): opath=%s\n", __func__, opath);

//	printf("%s(1): before theme list retrieval\n", __func__);

//	g_pThemeList = upath ? get_theme_list(gsspth, gsupth, gsopth) : NULL;
	g_pThemeList = get_theme_list(gsspth, gsupth, gsopth);

//	printf("%s(1): after theme list retrieval\n", __func__);

	if( gsopth) g_string_free(gsopth, TRUE);
	if( opath ) delete [] opath;

	if( gsspth) g_string_free(gsspth, TRUE);
//	if( spath ) delete [] spath;

	if( gsupth) g_string_free(gsupth, TRUE);
	if( upath ) delete [] upath;

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
		pEntry->pPath      = g_string_new (INTERNAL_THEME);
		pEntry->pFile      = g_string_new (INTERNAL_THEME);
		pEntry->pName      = g_string_new ("<default>");
		pEntry->pAuthor    = g_string_new ("pillbug");
		pEntry->pVersion   = g_string_new ("1.0");
		pEntry->pInfo      = g_string_new ("Internal fallback theme");
		pEntry->pModes     = g_string_new ("12/24");
		pEntry->pTextColor = g_string_new ("1.0;1.0;1.0");
		pEntry->pShdoColor = g_string_new ("0.0;0.0;0.0");
		pEntry->pFillColor = g_string_new ("0.5;0.5;0.5");
		g_pThemeList       = g_list_append(g_pThemeList, (gpointer)pEntry);
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

		GString* strs[] = { pTE->pPath,      pTE->pFile,      pTE->pName,  pTE->pAuthor,
							pTE->pVersion,   pTE->pInfo,      pTE->pModes,
							pTE->pTextColor, pTE->pShdoColor, pTE->pFillColor };

		for( size_t s = 0; s < vectsz(strs); s++ )
		{
//			if( pTE->pFile )
//				g_string_free(pTE->pFile, TRUE);
			if( strs[s] )
				g_string_free(strs[s], TRUE);
		}

		delete pTE; themeEntry = pTE = NULL;
	}
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
GList* get_theme_list(GString* pSystemPath, GString* pUserPath1, GString* pUserPath2)
{
//	printf("%s(2): entry\n\tspath sent as\n*%s*\n\tupath sent as\n*%s*\n\topath sent as\n*%s*\n",
//		__func__, pSystemPath->str, pUserPath1->str, pUserPath2->str);

	GList* pThemeList = NULL;

	if( pSystemPath )
		get_theme_list(pSystemPath, &pThemeList);

	if( pUserPath1 )
		get_theme_list(pUserPath1,  &pThemeList);

	if( pUserPath2 )
		get_theme_list(pUserPath2,  &pThemeList);

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

		pEntry->pPath      = g_string_new(pPath->str);
		pEntry->pFile      = g_string_new(pName);
		pEntry->pName      = pEntry->pAuthor    = NULL;
		pEntry->pVersion   = pEntry->pInfo      = pEntry->pModes     = NULL;
		pEntry->pTextColor = pEntry->pShdoColor = pEntry->pFillColor = NULL;

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
				const char* group1 =   "Theme";
				const char* group2 =   "jabs-clock";
				const char* grps[] = {  group1,         group1,           group1,            group1,         group2,          group2,              group2,              group2 };
				const char* keys[] = { "name",         "author",         "version",         "info",         "modes",         "text-color",        "shadow-color",      "fill-color" };
				GString**   vals[] = { &pEntry->pName, &pEntry->pAuthor, &pEntry->pVersion, &pEntry->pInfo, &pEntry->pModes, &pEntry->pTextColor, &pEntry->pShdoColor, &pEntry->pFillColor };

				for( size_t k = 0; k < vectsz(keys); k++ )
				{
					if( (str = g_key_file_get_string(pKF, grps[k], keys[k], NULL)) )
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

	const GString* n1  = ((const ThemeEntry*)themeEntry1)->pName;
	const GString* n2  = ((const ThemeEntry*)themeEntry2)->pName;
	const GString* p1  = ((const ThemeEntry*)themeEntry1)->pPath;
	const GString* p2  = ((const ThemeEntry*)themeEntry2)->pPath;
	const GString* f1  = ((const ThemeEntry*)themeEntry1)->pFile;
	const GString* f2  = ((const ThemeEntry*)themeEntry2)->pFile;
	const bool     ok1 =   n1 && n2 && n1->str && n2->str;
	const bool     ok2 =   p1 && p2 && p1->str && p2->str;
	const bool     ok3 =   f1 && f2 && f1->str && f2->str;

	int ret = 0; // default to same if

	if( ok1 && (ret = strnatcasecmp(n1->str, n2->str)) == 0 )
	{
		if( ok2 && (ret = strnatcasecmp(p1->str, p2->str)) == 0 )
		{
			if( ok3 )
				ret = strnatcasecmp(f1->str, f2->str);
		}
	}

	return ret;
}

