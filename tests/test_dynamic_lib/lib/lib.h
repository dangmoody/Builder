#pragma once

#if defined( _WIN32 )
	#ifdef DYNAMIC_LIBRARY_EXPORTS
		#define DYNAMIC_LIBRARY_API	__declspec( dllexport )
	#else
		#define DYNAMIC_LIBRARY_API	__declspec( dllimport )
	#endif
#elif defined( __linux__ )
	#ifdef DYNAMIC_LIBRARY_EXPORTS
		#define DYNAMIC_LIBRARY_API	__attribute__( ( visibility( "default" ) ) )
	#else
		#define DYNAMIC_LIBRARY_API
	#endif
#endif

extern DYNAMIC_LIBRARY_API void	SayHello();