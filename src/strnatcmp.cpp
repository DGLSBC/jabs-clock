/* -*- mode: c; c-file-style: "k&r" -*-

  strnatcmp.c -- Perform 'natural order' comparisons of strings in C.
  Copyright (C) 2000, 2004 by Martin Pool <mbp sourcefrog net>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/

/* partial change history:
 *
 * 2004-10-10 mbp: Lift out character type dependencies into macros.
 *
 * Eric Sosman pointed out that ctype functions take a parameter whose
 * value must be that of an unsigned int, even on platforms that have
 * negative chars in their default char type.
 */

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
#undef    DEBUGLOG
#define   DEBUGNSP     "snat"

#include <ctype.h>     // for ?

#include "strnatcmp.h" //
#include "debug.h"     // for debugging prints

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
static int strnatcmp0(const nat_char* a, const nat_char* b, bool fold_case);

// -----------------------------------------------------------------------------
int strnatcmp(const nat_char* a, const nat_char* b)
{
	return strnatcmp0(a, b, false); // case sensitive
}

// -----------------------------------------------------------------------------
int strnatcasecmp(const nat_char* a, const nat_char* b)
{
	return strnatcmp0(a, b, true);  // case insensitive
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
static int nat_cmpleft(const nat_char* a, const nat_char* b);
static int nat_cmprght(const nat_char* a, const nat_char* b);

// below funcs are defined inline to make it easier to adapt this code to
// different characters types or comparison functions

static inline bool     nat_isdigit(nat_char a) { return isdigit((unsigned char)a) != 0; }
static inline bool     nat_isspace(nat_char a) { return isspace((unsigned char)a) != 0; }
static inline nat_char nat_toupper(nat_char a) { return toupper((unsigned char)a); }

// -----------------------------------------------------------------------------
int strnatcmp0(const nat_char* a, const nat_char* b, bool fold_case)
{
	nat_char ca, cb;
	int      rc = 0;
	bool     ok = a  && b;
	int      ai = 0, bi = 0;

	while( ok && rc == 0 )
	{
		while( nat_isspace(ca = a[ai]) ) ai++; // skip over leading spaces
		while( nat_isspace(cb = b[bi]) ) bi++; // skip over leading spaces

		if( nat_isdigit(ca) && nat_isdigit(cb) ) // process run of digits, if any
		{
			if( ca == '0' || cb == '0' ) // fractional?
			{
				if( (rc = nat_cmpleft(a+ai, b+bi)) != 0 )
					break;
			}
			else
			{
				if( (rc = nat_cmprght(a+ai, b+bi)) != 0 )
					break;
			}
		}

		if( !ca && !cb ) // do the strings compare the same?
			break;       // perhaps the caller will want to call strcmp to break the tie

		if( fold_case )
		{
			ca = nat_toupper(ca);
			cb = nat_toupper(cb);
		}

		if(      ca < cb ) rc = -1;
		else if( ca > cb ) rc = +1;
		else { ai++;  bi++; }
	}

	return rc;
}

// -----------------------------------------------------------------------------
int nat_cmpleft(const nat_char* a, const nat_char* b)
{
	// compare two left-aligned numbers: the first to have a different value wins

	bool ad, bd;
	int  rc = 9;

	while( rc == 9 )
	{
		ad = nat_isdigit(*a);
		bd = nat_isdigit(*b);

		if(      !ad )     rc = !bd ? 0 : -1;
		else if( !bd )     rc =           +1;
		else if( *a < *b ) rc =           -1;
		else if( *a > *b ) rc =           +1;
		else { a++; b++; }
	}

	return rc;
}

// -----------------------------------------------------------------------------
int nat_cmprght(const nat_char* a, const nat_char* b)
{
	// compare two right-aligned numbers: the longest run of digits wins

	bool ad, bd;
	int  rc   = 9;
	int  bias = 0;

	// that aside, the greatest value wins, but it's not known which is greater
	// until both numbers are scanned to know if both magnitudes are the same,
	// so it's remembered in bias

	while( rc == 9 )
	{
		ad = nat_isdigit(*a);
		bd = nat_isdigit(*b);

		if(       !ad )       rc = !bd  ? bias : -1;
		else if(  !bd )       rc =               +1;
		else if(  *a <   *b ) if( !bias ) bias = -1;
		else if(  *a >   *b ) if( !bias ) bias = +1;
		else if( !*a && !*b ) rc = bias;

		if( rc == 9 ) { a++; b++; }
	}

     return rc;
}

