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

#include "string.h"
#include "debug.h"
#include "typecast.h"
#include "temp_storage.h"
#include "helpers.h"
#include "linear_allocator.h"

#include "stb_local.h"

#include <stdarg.h>
#include <string.h>

static String String_Vprintf( LinearAllocator *allocator, const char *fmt, va_list args ) {
	assert( allocator );
	assert( fmt );

	String result;

	va_list argsCopy;
	va_copy( argsCopy, args );

	int length = stbsp_vsnprintf( NULL, 0, fmt, args );
	assert( length >= 0 );

	result.count = trunc_cast( u64, length );

	result.data = cast( char *, Mem_AllocatorAlloc( allocator, result.count + 1 ) );
	stbsp_vsnprintf( result.data, length + 1, fmt, argsCopy );

	va_end( argsCopy );

	return result;
}

/*
================================================================================================

	String

================================================================================================
*/

String String_Set( const char *str ) {
	assert( str );

	return String_Set( str, strlen( str ) );
}

String String_Set( const char *str, const u64 count ) {
	assert( str );

	return String {
		.data	= cast( char *, str ),
		.count	= count,
	};
}

String String_Substring( const char *str, const u64 offset, const u64 count ) {
	assert( str );

	return String {
		.data	= cast( char *, str ) + offset,
		.count	= count,
	};
}

String String_Alloc( LinearAllocator *allocator, const char *str ) {
	assert( allocator );
	assert( str );

	return String_Alloc( allocator, str, strlen( str ) );
}

String String_Alloc( LinearAllocator *allocator, const char *str, const u64 length ) {
	assert( allocator );
	assert( str );

	String result = {
		.data	= cast( char *, Mem_AllocatorAlloc( allocator, length ) ),
		.count	= length,
	};

	memcpy( result.data, str, length );

	return result;
}

String String_Printf( LinearAllocator *allocator, const char *fmt, ... ) {
	assert( allocator );
	assert( fmt );

	va_list args;
	va_start( args, fmt );
	String result = String_Vprintf( allocator, fmt, args );
	va_end( args );

	return result;
}

String String_Copy( LinearAllocator *allocator, const String *src ) {
	assert( allocator );
	assert( src );

	return String_Alloc( allocator, src->data, src->count );
}

bool8 String_Equals( const char *lhs, const char *rhs ) {
	assert( lhs );
	assert( rhs );

	u64 lhsLen = strlen( lhs );
	u64 rhsLen = strlen( rhs );

	return ( lhsLen == rhsLen ) && memcmp( lhs, rhs, lhsLen ) == 0;
}

bool8 String_Equals( const String *lhs, const String *rhs ) {
	assert( lhs );
	assert( rhs );

	return ( lhs->count == rhs->count ) && ( memcmp( lhs->data, rhs->data, lhs->count ) == 0 );
}

bool8 String_StartsWith( const char *str, const char *prefix ) {
	assert( str );
	assert( prefix );

	return strncmp( str, prefix, strlen( prefix ) ) == 0;
}

bool8 String_StartsWith( const String *str, const String *prefix ) {
	assert( str );
	assert( prefix );

	return ( prefix->count <= str->count ) && ( memcmp( str->data, prefix->data, prefix->count ) == 0 );
}

bool8 String_EndsWith( const char *str, const char end ) {
	assert( str );

	u64 len = strlen( str );

	return ( len > 0 ) && ( str[len - 1] == end );
}

bool8 String_EndsWith( const char *str, const char *suffix ) {
	assert( str );
	assert( suffix );

	u64 len = strlen( str );
	u64 suffixLength = strlen( suffix );

	return ( suffixLength <= len ) && ( memcmp( str + len - suffixLength, suffix, suffixLength ) == 0 );
}

bool8 String_EndsWith( const String *str, const char end ) {
	assert( str );

	return ( str->count > 0 ) && ( str->data[str->count - 1] == end );
}

bool8 String_EndsWith( const String *str, const String *suffix ) {
	assert( str );
	assert( suffix );

	return ( suffix->count <= str->count ) && ( memcmp( str->data + ( str->count - suffix->count ), suffix->data, suffix->count ) == 0 );
}

bool8 String_Contains( const char *str, const char c ) {
	assert( str );

	return memchr( str, c, strlen( str ) ) != NULL;
}

bool8 String_Contains( const char *str, const char *substring ) {
	assert( str );
	assert( substring );

	return strstr( str, substring ) != NULL;
}

bool8 String_Contains( const String *str, const char c ) {
	assert( str );

	return str->data && memchr( str->data, c, str->count ) != NULL;
}

bool8 String_Contains( const String *str, const String *substring ) {
	assert( str );
	assert( substring );

	if ( substring->count == 0 ) {
		return true;
	}

	if ( substring->count > str->count ) {
		return false;
	}

	u64 searchCount = str->count - substring->count;
	for ( u64 i = 0; i <= searchCount; i++ ) {
		if ( memcmp( str->data + i, substring->data, substring->count ) == 0 ) {
			return true;
		}
	}

	return false;
}

String String_Replace( LinearAllocator *allocator, String* str, const char oldChar, const char newChar ) {
	assert( str );
	assert( str->data );

	String result = String_Copy( allocator, str );

	For ( u64, charIndex, 0, result.count ) {
		if ( result.data[charIndex] == oldChar ) {
			result.data[charIndex] = newChar;
		}
	}

	return result;
}

bool8 String_FindFromLeft( const String *str, const char c, u64 *outIndex ) {
	assert( str );
	assert( outIndex );

	char *pos = cast( char *, memchr( str->data, c, str->count ) );

	if ( !pos ) {
		return false;
	}

	*outIndex = cast( u64, pos ) - cast( u64, str->data );

	return true;
}

bool8 String_FindFromRight( const String *str, const char c, u64 *outIndex ) {
	assert( str );
	assert( outIndex );

	for ( u64 i = str->count; i > 0; i-- ) {
		if ( str->data[i - 1] == c ) {
			*outIndex = i - 1;
			return true;
		}
	}

	return false;
}

const char *String_Cstr( const String *str ) {
	assert( str );

	return TempCString( str->data, str->count );
}

const char *TempPrintf( const char *fmt, ... ) {
	assert( fmt );

	va_list args;
	va_start( args, fmt );
	String result = String_Vprintf( Mem_GetTempStorage(), fmt, args );
	va_end( args );

	return result.data;
}

char *TempCString( const char *from ) {
	assert( from );

	return TempCString( from, strlen( from ) );
}

char *TempCString( const char *from, const u64 numChars ) {
	assert( from );
	assert( numChars );

	char *result = cast( char *, Mem_TempAlloc( numChars + 1 ) );
	memcpy( result, from, numChars );
	result[numChars] = 0;

	return result;
}
