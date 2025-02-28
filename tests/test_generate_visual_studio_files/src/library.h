#pragma once

#ifdef LIBRARY_EXPORTS
#define LIBRARY_API __declspec( dllexport )
#else
#define LIBRARY_API __declspec( dllimport )
#endif

extern LIBRARY_API void Library1_SayHello();
extern LIBRARY_API void Library2_SayHello();