#pragma once

#if defined( _WIN32 )
#if defined( MATHLIB_BUILDING )
#define MATHLIB_API __declspec( dllexport )
#else
#define MATHLIB_API __declspec( dllimport )
#endif
#else
#define MATHLIB_API
#endif

MATHLIB_API int MathLib_Add( const int a, const int b );
MATHLIB_API int MathLib_Subtract( const int a, const int b );
