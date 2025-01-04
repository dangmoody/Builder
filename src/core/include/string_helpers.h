/*
===========================================================================

Core

Copyright (c) Dan Moody 2019 - Present

Unauthorized copying of this file, via any medium is strictly prohibited.
Proprietary and confidential.

===========================================================================
*/

#pragma once

#include "core_types.h"


// Returns true if the contents of string 'lhs' are EXACTLY the same as the contents of string 'rhs'.  Case sensitive.
bool8	string_equals( const char* lhs, const char* rhs );

// Returns true if the first characters of string 'str' are EXACTLY the same as string 'prefix'.  Case sensitive.
bool8	string_starts_with( const char* str, const char* prefix );

// Returns true if the last characters of string 'str' are EXACTLY the same as string 'suffix'.  Case sensitive.
bool8	string_ends_with( const char* str, const char* suffix );

// Returns true if string 'str' has EXACTLY the contents of 'substring' somewhere in it.  Case sensitive.
bool8	string_contains( const char* str, const char* substring );

// Takes a slice of characters from string 'str' starting at 'offset' + 'count' characters and puts it into out_string.
void	string_substring( const char* str, const u64 offset, const u64 count, char* out_string );

// TODO(DM): 13/2/2023: these wrappers shouldnt really exist
// what we should do instead is just #define sprintf to be either the C implementation or the stb one (or whatever)
// and then people can just call sprintf instead of having to use these
// I think that would be better
int		string_sprintf( char* buffer, const char* fmt, ... );
int		string_vsprintf( char* buffer, const char* fmt, const va_list args );
int		string_snprintf( char* buffer, const s64 buffer_length, const char* fmt, ... );
int		string_vsnprintf( char* buffer, const s64 buffer_length, const char* fmt, const va_list args );

// Returns a char* that is the result of calling sprintf with the given format string and var args, uses temp storage to allocate the result string.
char*	tprintf( const char* fmt, ... );

// Returns a char* that is the result of calling sprintf with the given format string and args list, uses temp storage to allocate the result string.
char*	vtprintf( const char* fmt, const va_list args );