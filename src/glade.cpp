/*******************************************************************************
**
**    I place this program under the "GNU General Public License".
**        http://www.gnu.org/licenses/licenses.html#GPL
**
*******************************************************************************/

#undef    DEBUGLOG
#define   DEBUGNSP   "glade"

#include <limits.h>      // for PATH_MAX

#include "glade.h"       //
#include "debug.h"       // for debugging prints
#include "utility.h"     // for get_user_appnm_path, strvcpy, strvcat, ...

#if      _USEGTK
#if GTK_CHECK_VERSION(3,0,0)
#undef   _USEGLADE
#else
#define  _USEGLADE
#undef   _USEGLADE_STATIC
#include <glade/glade.h> //
#ifndef  _USEGLADE_STATIC
#include "loadlib.h"     // for runtime loading
#endif
#endif
#else
#undef   _USEGLADE
#endif

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
namespace glade
{

#ifdef _USEGLADE

static void pGladeXml(GladeXML* pGladeXml);
static GladeXML* g_pGladeXml =  NULL;

#ifdef _USEGLADE_STATIC

bool        okay()                   { return true; }
GtkWidget* pWidget(const char* name) { return okayGUI() && okay() ? glade_xml_get_widget(g_pGladeXml, name) : NULL; }

#else // _USEGLADE_STATIC

static lib::symb g_symbs[] =
{
	{ "glade_xml_new",        NULL },
	{ "glade_xml_get_widget", NULL }
};

typedef GladeXML*  (*GXN) (const char* fname, const char* root, const char* domain);
typedef GtkWidget* (*GXGW)(GladeXML*   self,  const char* name);

static lib::LoadLib* g_pLLib;
static bool          g_pLLibok = false;
static  GXN          g_glade_xml_new = NULL;
static  GXGW         g_glade_xml_get_widget = NULL;

bool        okay()                   { return g_pLLibok; }
GtkWidget* pWidget(const char* name) { return okayGUI() && okay() ? (g_glade_xml_get_widget ? g_glade_xml_get_widget(g_pGladeXml, name) : NULL) : NULL; }

#endif // _USEGLADE_STATIC

#else  // _USEGLADE

bool        okay()                   { return false; }

#if _USEGTK
GtkWidget* pWidget(const char* name) { return NULL; }
#endif
#endif // _USEGLADE

} // namespace glade

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
bool glade::init()
{
	DEBUGLOGB;

#ifdef  _USEGLADE
#ifndef _USEGLADE_STATIC

	g_pLLib   = lib::create();
	g_pLLibok = g_pLLib && lib::load(g_pLLib, "glade-2.0", ".0", g_symbs, vectsz(g_symbs));

	DEBUGLOGP("g_pLLib is %s\n", g_pLLibok ? "okay" : "NOT okay");

	if( g_pLLibok )
	{
		lib::func(g_pLLib, LIB_FUNC_ADDR(g_glade_xml_new));
		lib::func(g_pLLib, LIB_FUNC_ADDR(g_glade_xml_get_widget));
	}
	else
	if( g_pLLib )
	{
		lib::destroy(g_pLLib);
		g_pLLib = NULL;
	}

#endif
#endif

	DEBUGLOGE;
	return okay();
}

// -----------------------------------------------------------------------------
void glade::dnit(bool unload)
{
	DEBUGLOGB;

#ifdef _USEGLADE

	if( okayGUI() )
	{
		DEBUGLOGS("clearing glade object");
//		g_clear_object(pGladeXml());
//		g_object_unref(pGladeXml());
		g_object_unref(g_pGladeXml);
		pGladeXml(NULL);

#ifndef _USEGLADE_STATIC

		if( unload && g_pLLibok )
		{
			// the libglade module apparently doesn't close down correctly
			// when done so via g_module_close, since unloading, reloading,
			// and then reusing glade_xml_new causes it to barf up all sorts
			// of errors (see below) and renders the entire library unusable

			// GLib-GObject-WARNING **:  cannot register existing type 'GladeXML'
			// GLib-CRITICAL **:         g_once_init_leave: assertion 'result != 0' failed
			// GLib-GObject-CRITICAL **: g_object_new: assertion 'G_TYPE_IS_OBJECT (object_type)' failed
			// libglade-CRITICAL **:     glade_xml_construct: assertion 'self != NULL' failed
			// GLib-GObject-CRITICAL **: g_object_unref: assertion 'G_IS_OBJECT (object)' failed

			// TODO: is there a workaround to the above that would allow the
			//       following code to be re-enabled?

//			DEBUGLOGS("unloading glade library");
//			lib::destroy(g_pLLib);
//			g_pLLibok = false;
//			g_pLLib   = NULL;
		}

#endif
	}

#endif

	DEBUGLOGE;
}

#ifdef _USEGLADE
// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
void glade::pGladeXml(GladeXML* pGladeXml) {        g_pGladeXml  = pGladeXml; }
bool glade::okayGUI()                      { return g_pGladeXml != NULL; }

// -----------------------------------------------------------------------------
bool glade::getXml(const char* name, const char* root, GCallback callBack)
{
	DEBUGLOGB;

	char        uipath[PATH_MAX];
	const char* anpath = get_user_appnm_path();
	bool  okay         = false;

	strvcpy(uipath,  anpath ? anpath : ".");
	strvcat(uipath, "/glade/");
	strvcat(uipath,  name);
	strvcat(uipath, ".glade");

	DEBUGLOGP("uipath=\n%s\n", uipath);

	if( anpath )
		delete [] anpath;

/*	if( callBack )
	{
		static GladeLoad gladeLoad;

		gladeLoad.uipath   = g_string_new(uipath);
		gladeLoad.root     = g_string_new(root ? root : "");
		gladeLoad.callBack = callBack;

		DEBUGLOGP("before async loading via worker thread of %s.glade\n", name);

		GThread* pThread = g_thread_try_new(__func__, load_glade_func, &gladeLoad, NULL);

		DEBUGLOGP("after worker thread creation which %s\n", pThread ? "succeeded" : "failed");

		if( pThread )
		{
			g_thread_unref(pThread);
			okay = true;
		}
	}*/

	if( !okay )
	{
/*		GTimeVal            ct;
		g_get_current_time(&ct);
		DEBUGLOGP("before sync loading of %s.glade (%d.%d)\n", name, (int)ct.tv_sec, (int)ct.tv_usec);*/

#ifdef _USEGLADE_STATIC

		if( g_isa_file(uipath) )
			pGladeXml(glade_xml_new(uipath, root, NULL));

#else // _USEGLADE_STATIC

		if( !glade::okay() )
			init();

		if( g_glade_xml_new )
		{
//			pGladeXml(g_glade_xml_new(uipath, root, NULL));

			DEBUGLOGS("before g_glade_xml_new call");
			GladeXML* pGX = g_isa_file(uipath) ? g_glade_xml_new(uipath, root, NULL) : NULL;
			DEBUGLOGS("after  g_glade_xml_new call");

			if( pGX )
			{
				pGladeXml(pGX);
				g_object_ref_sink(G_OBJECT(g_pGladeXml));
			}
		}

#endif // _USEGLADE_STATIC

//		okay = okayGUI() != NULL;
		okay = okayGUI();

/*		g_get_current_time(&ct);
		DEBUGLOGP("after loading, which %s (%d.%d)\n", ok ? "succeeded" : "failed", (int)ct.tv_sec, (int)ct.tv_usec);*/

		if( callBack )
			callBack();
	}

	DEBUGLOGE;
	return okay;
}

#else // _USEGLADE

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
bool glade::okayGUI() { return false; }

#if _USEGTK
// -----------------------------------------------------------------------------
bool glade::getXml(const char* name, const char* root, GCallback callBack)
{
	return false;
}
#endif
#endif // _USEGLADE
#if 0
// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
const gchar* glade::getGladePath()
{
#ifdef _RELEASE
	return PKGDATA_DIR "/glade/" APP_NAME_OLD ".glade";
#else
//	return "../glade/" APP_NAME_OLD "-new.glade";

	static char uipath[PATH_MAX];
	static bool first = true;

	if( first )
	{
		first              = false;
		const char* anpath = get_user_appnm_path();

		if( anpath )
		{
			strvcpy(uipath,  anpath ? anpath : ".");
			strvcat(uipath, "/glade/" APP_NAME_OLD ".glade");

			if( anpath )
				delete [] anpath;
		}
		else
		{
			strvcpy(uipath, "../glade/" APP_NAME_OLD "-new.glade");
		}
	}

	return uipath;
#endif
}
#endif
#if 0
// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
struct GladeLoad
{
	GString*  uipath;   // full path to the glade xml file to be loaded
	GString*  root;     // name of the root glade element from which to start loading
	GCallback callBack; // where to go once glade loading has finished
	bool      okay;     // whether the loading was a success
};

// -----------------------------------------------------------------------------
static gpointer load_glade_func(gpointer data)
{
	DEBUGLOGB;

	if( data )
	{
		GladeLoad* pGD = (GladeLoad*)data;

		DEBUGLOGP("before worker threaded loading of:\n\t%s\n", pGD->uipath->str);

		g_sync_threads_gui_beg();
//		glade::pGladeXml(glade_xml_new(pGD->uipath->str, pGD->root->len ? pGD->root->str : NULL, NULL));
		pGD->okay      = glade::okayGUI();
		g_sync_threads_gui_end();

		DEBUGLOGP("after loading, which %s\n", pGD->okay ? "succeeded" : "failed");

		g_string_free(pGD->uipath, TRUE);
		g_string_free(pGD->root,   TRUE);

		DEBUGLOGS("before making the callBack call");

		pGD->callBack();

		DEBUGLOGS("after making the callBack call");
	}

	DEBUGLOGE;
	return 0;
}
#endif

