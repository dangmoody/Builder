#pragma once

#if defined( _WIN32 )
	#ifdef LIBRARY_EXPORTS
		#define LIBRARY_API __declspec( dllexport )
	#else
		#define LIBRARY_API __declspec( dllimport )
	#endif
#elif defined( __linux__ )
	#ifdef LIBRARY_EXPORTS
		#define LIBRARY_API __attribute__( ( visibility( "default" ) ) )
	#else
		#define LIBRARY_API
	#endif
#endif

extern LIBRARY_API void Library1_SayHello();
extern LIBRARY_API void Library2_SayHello();