#pragma once

#ifdef DYNAMIC_LIBRARY_EXPORTS
#define DYNAMIC_LIBRARY_API __declspec( dllexport )
#else
#define DYNAMIC_LIBRARY_API __declspec( dllimport )
#endif

extern DYNAMIC_LIBRARY_API void	SayHello();