/*******************************************************************************
**
**    I place this program under the "GNU General Public License".
**        http://www.gnu.org/licenses/licenses.html#GPL
**
*******************************************************************************/

#undef    DEBUGLOG
#define   DEBUGNSP     "ldthm"

#define  _USEARCHIVE
#undef   _USEARCHIVE_STATIC

#include <glib.h>      // for opendir, ...
#include <stdio.h>     // for ?
#include <limits.h>    // for PATH_MAX
#include <malloc.h>    // for free
#include <stdlib.h>    // for mkdtemp
#include <sys/stat.h>  // for ?

#include "loadTheme.h" //
#include "utility.h"   // for get_user_theme_path
#include "basecpp.h"   // for strvxxx functions
#include "debug.h"     // for debugging prints

//#include "nanosvg.h" // for svg xml file parsing to get object bounding boxes that rsvg & cairo don't provide

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
bool loadTheme(const char* theme_file_path, float& iwidth, float& iheight, float* bbox)
{
	DEBUGLOGB;
	DEBUGLOGP("  %s\n", theme_file_path);

	iwidth = iheight = 0;

	if( bbox )
		bbox[0] = bbox[1] = bbox[2] = bbox[3] = 0;

	DEBUGLOGE;
	return false;

/*	NSVGimage* image = nsvgParseFromFile(theme_file_path, "px", 96);

	if( image )
	{
		iwidth  = image->width;
		iheight = image->height;

		DEBUGLOGP("%fx%f (%dx%d)\n", image->width, image->height, iwidth, iheight);

		if( bbox )
		{
			bool  got1 =  false;
			float xmin =  iwidth, ymin = iheight, xmax = 0, ymax = 0;
			float xb, yb, xe, ye;

			#define min(a,b) (a) < (b) ? (a) : (b)
			#define max(a,b) (a) > (b) ? (a) : (b)

			for( NSVGshape* shape = image->shapes; shape != NULL; shape = shape->next )
			{
				xb   = min(shape->bounds[0], shape->bounds[2]);
				yb   = min(shape->bounds[1], shape->bounds[3]);
				xe   = max(shape->bounds[0], shape->bounds[2]);
				ye   = max(shape->bounds[1], shape->bounds[3]);
				xmin = min(xb, xmin);
				ymin = min(yb, ymin);
				xmax = max(xe, xmax);
				ymax = max(ye, ymax);
				got1 = true;

				DEBUGLOGP("%fx%f->%fx%f (%fx%f) (%dx%d->%dx%d) (%dx%d)\n",
					xb, yb, xe, ye, xe-xb, ye-yb,
					(int)xb, (int)yb, (int)xe, (int)ye, (int)(xe-xb), (int)(ye-yb));
			}

			DEBUGLOGP("%fx%f->%fx%f (%fx%f) (%dx%d->%dx%d) (%dx%d)\n",
				xmin, ymin, xmax, ymax, xmax-xmin, ymax-ymin,
				(int)xmin, (int)ymin, (int)xmax, (int)ymax, (int)(xmax-xmin), (int)(ymax-ymin));

			if( got1 )
			{
//				bbox[0]  =  xmin;
//				bbox[1]  =  ymin;
//				bbox[2]  =  xmax;
//				bbox[3]  =  ymax;

				float xs =  iwidth /50;
				float ys =  iheight/50;
				float xc = (xmax+xmin)/2, xd = xmax-xmin, xh = xd/2;
				float yc = (ymax+ymin)/2, yd = ymax-ymin, yh = yd/2;
				
				bbox[0]  =  xc-xh-xs;
				bbox[1]  =  yc-yh-ys;
				bbox[2]  =  xc+xh+xs;
				bbox[3]  =  yc+yh+ys;
			}
		}

		nsvgDelete(image);
	}

	DEBUGLOGE;
	return image != NULL && iwidth > 0 && iheight > 0;*/
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
static bool extract(const char* spath, const char* dpath);

//#include <unistd.h>  // for sleep

// -----------------------------------------------------------------------------
bool loadTheme(const char* theme_arch_file_path, char* theme_dir, int theme_dir_size, char* theme_fnm, int theme_fnm_size)
{
	DEBUGLOGB;

	char  temp_dir[PATH_MAX], temp_fnm[64];

	const char* utp  = get_user_theme_path();
	bool        okay = false;

	strvcpy(temp_dir,  utp ? utp : ".");
	strvcat(temp_dir, "/");
	strvcat(temp_dir, "clk040XXXXXX");
	mkdtemp(temp_dir);
	mkdir  (temp_dir,  S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH); // just in case of mkdtemp failure, create default
	strvcat(temp_dir, "/");

//	g_mkdir_with_parents(temp_dir, 0x755);

	DEBUGLOGP("arch_file_path is \n%s\n", theme_arch_file_path);
	DEBUGLOGP("temp_dir is \n%s\n",       temp_dir);

	okay = extract(theme_arch_file_path, temp_dir);

	if( okay )
	{
		char td[PATH_MAX], pb[PATH_MAX];

		// validate (this should be a call to a func since need that functionality elsewhere)

		DIR* dp  = opendir(temp_dir);

		if( okay =  dp != NULL )
		{
			dirent* ep;
			int     ne = 0;

			while( (ep = readdir(dp)) != NULL )
			{
				if( strcmp (ep->d_name, ".") != 0 && strcmp(ep->d_name, "..") != 0 )
				{
					strvcpy(td,       temp_dir);
					strvcpy(pb,       temp_dir);
					strvcat(pb,       ep->d_name);
					strvcpy(temp_fnm, ep->d_name);

					if( !(okay = ((++ne == 1) && g_isa_dir(pb))) )
						break;
				}
			}

			okay = ne == 1;

			closedir(dp);
		}

		if( okay ) // move the theme sub-dir (pb) to temp_dir's parent dir (utp or .)
		{
			char    temp_pth[PATH_MAX];
			strvcpy(temp_dir,  utp ? utp : ".");
			strvcat(temp_dir, "/");
			strvcpy(temp_pth,  temp_dir);
			strvcat(temp_pth,  temp_fnm);
			mkdir  (temp_pth,  S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);

			if( okay = g_isa_dir(temp_pth) )
			{
				DEBUGLOGP("new theme name is %s\n", temp_fnm);
				DEBUGLOGP("cur theme dir is\n%s\n", pb);       // old
				DEBUGLOGP("new theme dir is\n%s\n", temp_pth); // new

				int    rc =  rename(pb, temp_pth); // rename old (pb) to new (temp_pth)
				okay = rc == 0;

				DEBUGLOGP("rename rc is %d\n", rc);
			}
		}

		if( okay )
			g_del_dir(td);
	}
	else
	{
		DEBUGLOGS("extract failed (deleting temp dir)");

		if( g_isa_dir(temp_dir) )
			g_del_dir(temp_dir);
	}

	if( utp )
		delete [] utp;

	if( theme_dir && theme_dir_size )
	{
		if( okay )
		{
			strvcpy(theme_dir, temp_dir, theme_dir_size); // TODO: this is okay?
		}
		else
			*theme_dir = '\0';
	}

	if( theme_fnm && theme_fnm_size )
	{
		if( okay )
		{
			strvcpy(theme_fnm, temp_fnm, theme_fnm_size); // TODO: this is okay?
		}
		else
			*theme_fnm = '\0';
	}

	DEBUGLOGE;
	return okay;
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
//#define   NANOSVG_IMPLEMENTATION // put the following header's lib code inline
//#include "nanosvg.h"

#ifdef _USEARCHIVE

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
#include <archive.h>
#include <archive_entry.h>

static bool copyData(archive* sorc, archive* dest);

#ifdef _USEARCHIVE_STATIC
// -----------------------------------------------------------------------------
bool extract(const char* spath, const char* dpath)
{
	DEBUGLOGB;

	int      retc;
	bool     okay = false;
	archive* sorc = archive_read_new();
	archive* dest = archive_write_disk_new();

	if( !sorc || !dest )
	{
		DEBUGLOGE;
		DEBUGLOGS("  (1) - unable to create new read & write archive objects");
		return okay;
	}

	archive_read_support_filter_all(sorc);
	archive_read_support_format_all(sorc);

	archive_write_disk_set_options(dest, ARCHIVE_EXTRACT_TIME);

	if( (retc = archive_read_open_filename(sorc, spath, 64*1024)) != ARCHIVE_OK )
	{
		int         errn = archive_errno(sorc);
		const char* errs = archive_error_string(sorc);

		DEBUGLOGE;
		DEBUGLOGP("  (2) - unable to open archive (%d:%d)\n%s\n%s\n", retc, errn, errs, spath);
		return okay;
	}

	archive_entry* entry;

	while( true )
	{
		if( (retc = archive_read_next_header(sorc, &entry)) == ARCHIVE_EOF )
		{
			DEBUGLOGS("archive_read_next_header returned 'eof'");
			okay = true;
			break;
		}

		if( retc != ARCHIVE_OK )
		{
			DEBUGLOGS("archive_read_next_header returned 'archive not ok' error");
			break;
		}

		char    destPath[PATH_MAX];
		strvcpy(destPath, dpath);
		strvcat(destPath, archive_entry_pathname(entry));

		DEBUGLOGP("extracting %s\n", destPath);

		archive_entry_set_pathname(entry, destPath);

		if( (retc = archive_write_header(dest, entry)) != ARCHIVE_OK )
		{
			DEBUGLOGS("archive_write_header returned 'archive not ok' error");
			break;
		}

		copyData(sorc, dest);

		if( (retc = archive_write_finish_entry(dest)) != ARCHIVE_OK )
		{
			DEBUGLOGS("archive_write_finish returned 'archive not ok' error");
			break;
		}
	}

	archive_write_free(dest);
	archive_read_free(sorc);

	DEBUGLOGE;
	return okay;
}

// -----------------------------------------------------------------------------
bool copyData(archive* sorc, archive* dest)
{
	DEBUGLOGB;

	int         retc;
	const void* buff;
	size_t      size;
	int64_t     offs;
	bool        okay = false;

	while( true )
	{
		if( (retc = archive_read_data_block (sorc, &buff, &size, &offs)) == ARCHIVE_EOF )
		{
			okay = true;
			break;
		}

		if( retc != ARCHIVE_OK )
			break;

		if( (retc = archive_write_data_block(dest,  buff,  size,  offs)) != ARCHIVE_OK )
			break;
	}

	DEBUGLOGE;
	return okay;
}

#else  // _USEARCHIVE_STATIC

#include "loadlib.h" // for runtime loading

static lib::symb g_symbs[] =
{
	{ "archive_entry_pathname",          NULL },
	{ "archive_entry_set_pathname",      NULL },

	{ "archive_errno",                   NULL },
	{ "archive_error_string",            NULL },

	{ "archive_read_free",               NULL },
	{ "archive_read_new",                NULL },
	{ "archive_read_next_header",        NULL },
	{ "archive_read_open_filename",      NULL },
	{ "archive_read_support_filter_all", NULL },
	{ "archive_read_support_format_all", NULL },

	{ "archive_write_disk_new",          NULL },
	{ "archive_write_disk_set_options",  NULL },
	{ "archive_write_finish_entry",      NULL },
	{ "archive_write_free",              NULL },
	{ "archive_write_header",            NULL },

	{ "archive_read_data_block",         NULL },
	{ "archive_write_data_block",        NULL }
};

typedef const char*  (*AEP)   (archive_entry*);
typedef void         (*AESP)  (archive_entry*, const char*);

typedef int          (*AE)    (archive*);
typedef const char*  (*AES)   (archive*);

typedef int          (*ARF)   (archive*);
typedef archive*     (*ARN)   ();
typedef int          (*ARNH)  (archive*, archive_entry**);
typedef int          (*AROF)  (archive*, const char*, size_t);
typedef int          (*ARSFIA)(archive*);
typedef int          (*ARSFOA)(archive*);

typedef archive*     (*AWDN)  ();
typedef int          (*AWDSO) (archive*, int flags);
typedef int          (*AWFE)  (archive*);
typedef int          (*AWF)   (archive*);
typedef int          (*AWH)   (archive*, archive_entry*);

typedef int          (*ARDB)  (archive*, const void**, size_t*, __LA_INT64_T*);
typedef __LA_SSIZE_T (*AWDB)  (archive*, const void*,  size_t,  __LA_INT64_T);

static AEP    g_archive_entry_pathname;
static AESP   g_archive_entry_set_pathname;

static AE     g_archive_errno;
static AES    g_archive_error_string;

static ARF    g_archive_read_free;
static ARN    g_archive_read_new;
static ARNH   g_archive_read_next_header;
static AROF   g_archive_read_open_filename;
static ARSFIA g_archive_read_support_filter_all;
static ARSFOA g_archive_read_support_format_all;

static AWDN   g_archive_write_disk_new;
static AWDSO  g_archive_write_disk_set_options;
static AWFE   g_archive_write_finish_entry;
static AWF    g_archive_write_free;
static AWH    g_archive_write_header;

static ARDB   g_archive_read_data_block;
static AWDB   g_archive_write_data_block;

// -----------------------------------------------------------------------------
bool extract(const char* spath, const char* dpath)
{
	DEBUGLOGB;

	int           retc;
	lib::LoadLib* pLLib = lib::create();
	bool          okay  = pLLib && lib::load(pLLib, "archive", ".13", g_symbs, vectsz(g_symbs));

	if( okay )
	{
		DEBUGLOGS("archive lib loaded ok - before typedef'd func address setting");

		lib::func(pLLib, LIB_FUNC_ADDR(g_archive_entry_pathname));
		lib::func(pLLib, LIB_FUNC_ADDR(g_archive_entry_set_pathname));
		lib::func(pLLib, LIB_FUNC_ADDR(g_archive_errno));
		lib::func(pLLib, LIB_FUNC_ADDR(g_archive_error_string));
		lib::func(pLLib, LIB_FUNC_ADDR(g_archive_read_free));
		lib::func(pLLib, LIB_FUNC_ADDR(g_archive_read_new));
		lib::func(pLLib, LIB_FUNC_ADDR(g_archive_read_next_header));
		lib::func(pLLib, LIB_FUNC_ADDR(g_archive_read_open_filename));
		lib::func(pLLib, LIB_FUNC_ADDR(g_archive_read_support_filter_all));
		lib::func(pLLib, LIB_FUNC_ADDR(g_archive_read_support_format_all));
		lib::func(pLLib, LIB_FUNC_ADDR(g_archive_write_disk_new));
		lib::func(pLLib, LIB_FUNC_ADDR(g_archive_write_disk_set_options));
		lib::func(pLLib, LIB_FUNC_ADDR(g_archive_write_finish_entry));
		lib::func(pLLib, LIB_FUNC_ADDR(g_archive_write_free));
		lib::func(pLLib, LIB_FUNC_ADDR(g_archive_write_header));
		lib::func(pLLib, LIB_FUNC_ADDR(g_archive_read_data_block));
		lib::func(pLLib, LIB_FUNC_ADDR(g_archive_write_data_block));

		DEBUGLOGS("archive lib loaded ok - after  typedef'd func address setting");
	}

	archive *sorc, *dest;

	if( okay )
	{
		DEBUGLOGS("before archive read/write new calls");
		sorc = g_archive_read_new();
		dest = g_archive_write_disk_new();
		DEBUGLOGS("after  archive read/write new calls");

		if( !sorc || !dest )
		{
			DEBUGLOGS("  (1) - unable to create new read & write archive objects");
			okay = false;
		}
	}

	if( okay )
	{
		g_archive_read_support_filter_all(sorc);
		g_archive_read_support_format_all(sorc);

		g_archive_write_disk_set_options(dest, ARCHIVE_EXTRACT_TIME);

		if( (retc = g_archive_read_open_filename(sorc, spath, 64*1024)) != ARCHIVE_OK )
		{
			int         errn = g_archive_errno(sorc);
			const char* errs = g_archive_error_string(sorc);

			DEBUGLOGP("  (2) - unable to open archive (%d:%d)\n%s\n%s\n", retc, errn, errs, spath);
			okay = false;
		}
	}

	if( okay )
	{
		archive_entry* entry;

		while( true )
		{
			if( (retc = g_archive_read_next_header(sorc, &entry)) == ARCHIVE_EOF )
			{
				DEBUGLOGS("archive_read_next_header returned 'eof'");
				break;
			}

			if( retc != ARCHIVE_OK )
			{
				DEBUGLOGS("archive_read_next_header returned 'archive not ok' error");
				okay = false;
				break;
			}

			char    destPath[PATH_MAX];
			strvcpy(destPath, dpath);
			strvcat(destPath, g_archive_entry_pathname(entry));

			DEBUGLOGP("extracting %s\n", destPath);

			g_archive_entry_set_pathname(entry, destPath);

			if( (retc = g_archive_write_header(dest, entry)) != ARCHIVE_OK )
			{
				DEBUGLOGS("archive_write_header returned 'archive not ok' error");
				okay = false;
				break;
			}

			if( !copyData(sorc, dest) )
			{
				DEBUGLOGS("copyData returned 'archive not ok' error");
				okay = false;
				break;
			}

			if( (retc = g_archive_write_finish_entry(dest)) != ARCHIVE_OK )
			{
				DEBUGLOGS("archive_write_finish returned 'archive not ok' error");
				okay = false;
				break;
			}
		}
	}

	if( dest )
		g_archive_write_free(dest);

	if( sorc )
		g_archive_read_free(sorc);

	lib::destroy(pLLib);
	pLLib = NULL;

	DEBUGLOGS(okay ? "archive file contents extraction was successful" : "archive file contents extraction was NOT successful");

	DEBUGLOGE;
	return okay;
}

// -----------------------------------------------------------------------------
bool copyData(archive* sorc, archive* dest)
{
	DEBUGLOGB;

	int         retc;
	const void* buff;
	size_t      size;
	int64_t     offs;
	bool        okay = false;

	while( true )
	{
		if( (retc = g_archive_read_data_block (sorc, &buff, &size, &offs)) == ARCHIVE_EOF )
		{
			okay = true;
			break;
		}

		if( retc != ARCHIVE_OK )
			break;

		if( (retc = g_archive_write_data_block(dest,  buff,  size,  offs)) != ARCHIVE_OK )
			break;
	}

	DEBUGLOGE;
	return okay;
}

#endif // _USEARCHIVE_STATIC

#else  // _USEARCHIVE

bool extract(const char* spath, const char* dpath) { return false; }

#endif // _USEARCHIVE

