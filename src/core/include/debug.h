/*
===========================================================================

Core

Copyright (c) Dan Moody 2019 - Present

Unauthorized copying of this file, via any medium is strictly prohibited.
Proprietary and confidential.

===========================================================================
*/

#pragma once

/*
================================================================================================

	Debug functions

================================================================================================
*/

// logging
void							warning( const char* fmt, ... );
void							error( const char* fmt, ... );

void							dump_callstack( void );

// DO NOT CALL THESE FUNCTIONS DIRECTLY
void							fatal_error_internal( const char* file, const int line, const char* prefix, const char* fmt, ... );


#ifdef _WIN32
	#ifndef debug_break
		#define debug_break()	__debugbreak()
	#endif
#else
	#error Unrecognised platform.  Please fill this #define in for this platform.
#endif

#ifndef fatal_error
	#define fatal_error( fmt, ... )	fatal_error_internal( __FILE__, __LINE__, "FATAL ERROR", fmt, __VA_ARGS__ ); debug_break()
#endif

#ifdef _DEBUG
	// helper macro for debugging
	// will trigger a breakpoint if the condition is met
	#define break_here_if( condition ) \
		do { \
			if ( (condition) ) { \
				debug_break(); \
			} \
		} while ( 0 )

	#define assert( x ) \
		do { \
			if ( !( x ) ) { \
				fatal_error_internal( __FUNCTION__, __LINE__, "ASSERT FAILED", #x ); \
				debug_break(); \
			} \
		} while ( 0 )

	#define assertf( x, fmt, ... ) \
		do { \
			if ( !( x ) ) { \
				fatal_error_internal( __FUNCTION__, __LINE__, "ASSERT FAILED", fmt, __VA_ARGS__ ); \
				debug_break(); \
			} \
		} while ( 0 )
#else
	#define assert( x )
	#define assertf( x, fmt, ... )
#endif