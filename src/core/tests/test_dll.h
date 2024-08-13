#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/*
================================================================================================

	Test DLL

	This is used by the Core tests to check that the Library API can load/query/unload DLLs
	correctly.

	We test that we can load a DLL, grab a function from it and make sure that the loaded
	function can be called/used correctly.

	If you edit something here you might also need to edit/update changes inside the Core
	Library tests.

================================================================================================
*/

#define TEST_DLL_NAME "TestDLL"

extern __declspec( dllexport ) const char* get_dll_name( void );

#ifdef __cplusplus
}
#endif