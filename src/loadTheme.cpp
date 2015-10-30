/*******************************************************************************
**
**    I place this program under the "GNU General Public License".
**        http://www.gnu.org/licenses/licenses.html#GPL
**
*******************************************************************************/

#undef    DEBUGLOG
#define   DEBUGNSP   "ldthm"

#include <glib.h>      // for opendir, ...
#include <sys/stat.h>  // for ?
#include <limits.h>    // for PATH_MAX
#include <malloc.h>    // for free
#include <stdlib.h>    // for mkdtemp
#include <stdio.h>     // for ?

#include "loadTheme.h" //
#include "utility.h"   // for get_user_theme_path
#include "basecpp.h"   // for strvxxx functions
#include "debug.h"     // for debugging prints

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
#define  _USEARCHIVE

//#include "nanosvg.h" // for svg xml file parsing to get object bounding boxes that rsvg & cairo don't provide

// -----------------------------------------------------------------------------
bool loadTheme(const char* theme_file_path, float& iwidth, float& iheight, float* bbox)
{
	DEBUGLOGB;
	DEBUGLOGP("  *%s*\n", theme_file_path);

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
bool loadTheme(const char* theme_archive_file_path, char* theme_dir, int theme_dir_size, char* theme_fnm, int theme_fnm_size)
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

	DEBUGLOGP("archive_file_path is \n*%s*\n", theme_archive_file_path);
	DEBUGLOGP("temp_dir          is \n*%s*\n", temp_dir);

	okay = extract(theme_archive_file_path, temp_dir);

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
				DEBUGLOGP("new theme name is  *%s*\n", temp_fnm);
				DEBUGLOGP("cur theme dir  is\n*%s*\n", pb);       // old
				DEBUGLOGP("new theme dir  is\n*%s*\n", temp_pth); // new

				int    rc =  rename(pb, temp_pth); // rename old (pb) to new (temp_pth)
				okay = rc == 0;

				DEBUGLOGP("rename rc is %d\n", rc);
			}
		}

		if( okay )
			g_del_dir(td);
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
		DEBUGLOGP("  (2) - unable to open archive (%d:%d)\n%s\n*%s*\n", retc, errn, errs, spath);
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

		char    destPath[1024];
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

#else

bool extract(const char* spath, const char* dpath) { return false; }

#endif // _USEARCHIVE

