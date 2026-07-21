/*
===========================================================================

Core

Copyright (c) 2025 - present Dan Moody

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

===========================================================================
*/

#include "paths.h"

#include "temp_storage.h"
#include "typecast.h"
#include "string.h"
#include "string_builder.h"
#include "math.h"
#include "defer.h"

#include <stdarg.h>
#include <string.h>

static bool8 Path_GetLastSlash( const string_t *path, u64 *outLastSlashPos ) {
	Assert( path );

	u64 lastForwardSlash = 0;
	u64 lastBackSlash = 0;

	if ( !String_FindFromRight( path, '/', &lastForwardSlash ) && !String_FindFromRight( path, '\\', &lastBackSlash ) ) {
		return false;
	}

	if ( outLastSlashPos ) {
		*outLastSlashPos = Max( lastForwardSlash, lastBackSlash );
	}

	return true;
}

/*
================================================================================================

	Paths

================================================================================================
*/

string_t Path_RemoveFileFromPath( const string_t *path ) {
	Assert( path );

	u64 lastSlashPos = 0;
	if ( !Path_GetLastSlash( path, &lastSlashPos ) ) {
		return *path;
	}

	return string_t {
		.data	= path->data,
		.count	= lastSlashPos,
	};
}

string_t Path_RemovePathFromFile( const string_t *path ) {
	Assert( path );

	u64 lastSlashPos = 0;
	if ( !Path_GetLastSlash( path, &lastSlashPos ) ) {
		return *path;
	}

	lastSlashPos += 1;

	return {
		.data	= path->data + lastSlashPos,
		.count	= path->count - lastSlashPos,
	};
}

string_t Path_RemoveFileExtension( const string_t *filename ) {
	Assert( filename );

	u64 dotPos = 0;
	if ( !String_FindFromRight( filename, '.', &dotPos ) ) {
		return *filename;
	}

	return {
		.data	= filename->data,
		.count	= dotPos
	};
}

static string_t Path_JoinInternalV( linearAllocator_t *allocator, const int count, va_list args ) {
	Assert( allocator );

	stringBuilder_t builder = SB_Create( allocator );

	For ( int, argIndex, 0, count ) {
		if ( argIndex > 0 ) {
			SB_Appendf( &builder, "%c", PATH_SEPARATOR );
		}

		const char *part = va_arg( args, const char * );

		SB_Appendf( &builder, "%s", part );
	}

	const char *finalPath = SB_ToString( &builder );

	string_t str = {
		.data	= Cast( char *, finalPath ),
		.count	= strlen( finalPath ),
	};

	return str;
}

string_t Path_JoinInternal( linearAllocator_t *allocator, const int count, ... ) {
	Assert( allocator );

	va_list args;
	va_start( args, count );
	string_t result = Path_JoinInternalV( allocator, count, args );
	va_end( args );

	return result;
}

string_t Path_RelativePathTo( linearAllocator_t *allocator, const char *from, const char *to ) {
	Assert( from );
	Assert( to );

	u64 pos = Mem_TempTell();
	defer { Mem_TempRewindTo( pos ); };

	string_t fromStr = String_Set( from );
	string_t toStr = String_Set( to );
	fromStr = Path_FixSlashes( Mem_GetTempStorage(), &fromStr );
	toStr = Path_FixSlashes( Mem_GetTempStorage(), &toStr );

	// determine the directory part of 'from'
	// if the last path segment contains a dot then treat it as a filename and strip it
	// otherwise treat the whole path as a directory and ensure it ends with a slash
	string_t fromDir;
	{
		u64 lastSlashPos = 0;
		bool8 hasSlash = String_FindFromRight( &fromStr, PATH_SEPARATOR, &lastSlashPos );

		bool8 lastSegmentHasDot = false;

		u64 lastSegmentStart = hasSlash ? lastSlashPos + 1 : 0;

		for ( u64 indexInLastSegment = lastSegmentStart; indexInLastSegment < fromStr.count; indexInLastSegment++ ) {
			if ( fromStr.data[indexInLastSegment] == '.' ) {
				lastSegmentHasDot = true;
				break;
			}
		}

		if ( lastSegmentHasDot ) {
			// fromDir = {
			// 	.data  = fromStr.data,
			// 	.count = lastSlashPos + 1,
			// };
			fromDir = String_Set( fromStr.data, lastSlashPos + 1 );
		} else if ( fromStr.data[fromStr.count - 1] == PATH_SEPARATOR ) {
			fromDir = fromStr;
		} else {
			fromDir = String_Printf( Mem_GetTempStorage(), "%.*s%c", (int) fromStr.count, fromStr.data, PATH_SEPARATOR );
		}
	}

	// walk both paths simultaneously, recording the end of the last complete
	// segment that matched (i.e. right after a slash)
	u64 common = 0;
	u64 charIndex = 0;
	while ( charIndex < fromDir.count && charIndex < toStr.count && fromDir.data[charIndex] == toStr.data[charIndex] ) {
		charIndex++;
		if ( fromDir.data[charIndex - 1] == PATH_SEPARATOR ) {
			common = charIndex;
		}
	}

	// if one path ends exactly at a segment boundary in the other then include that boundary
	if ( charIndex < fromDir.count && fromDir.data[charIndex] == PATH_SEPARATOR && charIndex == toStr.count ) {
		common = charIndex;
	} else if ( charIndex == fromDir.count && charIndex < toStr.count && toStr.data[charIndex] == PATH_SEPARATOR ) {
		common = charIndex;
	} else if ( charIndex == fromDir.count && charIndex == toStr.count ) {
		common = charIndex;
	}

	// count directory segments remaining in fromDir after the common prefix
	// the character at 'common' is the boundary separator itself
	// skip it so we don't count it as an extra level
	u64 countStart = common;
	if ( countStart < fromDir.count && fromDir.data[countStart] == PATH_SEPARATOR ) {
		countStart++;
	}

	u64 numBacks = 0;

	for ( u64 charPos = countStart; charPos < fromDir.count; charPos++ ) {
		if ( fromDir.data[charPos] == PATH_SEPARATOR ) {
			numBacks++;
		}
	}

	stringBuilder_t sb = SB_Create( allocator );

	u64 resultLength = 0;

	For ( u64, backIndex, 0, numBacks ) {
		bool8 isLast = ( backIndex == numBacks - 1 );

		if ( !isLast || common < toStr.count ) {
			SB_Appendf( &sb, "..%c", PATH_SEPARATOR );
			resultLength += 3;
		} else {
			SB_Appendf( &sb, ".." );
			resultLength += 2;
		}
	}

	if ( common < toStr.count ) {
		SB_Appendf( &sb, "%.*s", (int) ( toStr.count - common ), toStr.data + common );
		resultLength += toStr.count - common;
	}

	return {
		.data  = Cast( char *, SB_ToString( &sb ) ),
		.count = resultLength,
	};
}
