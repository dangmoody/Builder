#ifdef _WIN32

#include "win_sdk.h"

#include "core/include/typecast.inl"
#include "core/include/file.h"
#include "core/include/temp_storage.h"
#include "core/include/string_helpers.h"

#include <Windows.h>

#include <stdio.h>
#include <assert.h>

static void OnWindowsSDKVersionFound( const FileInfo *fileInfo, void *data ) {
	if ( !fileInfo->is_directory ) {
		return;
	}

	windowsSDK_t *windowsSDK = cast( windowsSDK_t *, data );

	s32 v0 = 0, v1 = 0, v2 = 0, v3 = 0;
	sscanf( fileInfo->filename, "%d.%d.%d.%d", &v0, &v1, &v2, &v3 );

	if ( v0 < windowsSDK->version.v0 ) return;
	if ( v1 < windowsSDK->version.v1 ) return;
	if ( v2 < windowsSDK->version.v2 ) return;
	if ( v3 < windowsSDK->version.v3 ) return;

	windowsSDK->version.v0 = v0;
	windowsSDK->version.v1 = v1;
	windowsSDK->version.v2 = v2;
	windowsSDK->version.v3 = v3;
}

static const char *FindRegistryValueFromKey( const HKEY key, const char *valueName ) {
	DWORD valueStrLength = 0;

	LSTATUS status = RegQueryValueExA( key, valueName, NULL, NULL, NULL, &valueStrLength );

	if ( status != ERROR_SUCCESS ) {
		return NULL;
	}

	char *valueStr = cast( char *, mem_temp_alloc( ( valueStrLength + 1 ) * sizeof( char ) ) );

	DWORD type;
	status = RegQueryValueExA( key, valueName, NULL, &type, cast( LPBYTE, valueStr ), &valueStrLength );

	if ( type != REG_SZ ) {
		return NULL;
	}

	if ( status == ERROR_SUCCESS ) {
		valueStr[valueStrLength] = 0;
		return valueStr;
	} else if ( status == ERROR_MORE_DATA ) {
		assert( false );	// should never get here
	}

	return NULL;
}

bool8 Win_GetSDK( windowsSDK_t *outSDK ) {
	assert( outSDK );

	// memset( outSDK, 0, sizeof( windowsSDK_t ) );

	HKEY key;

	LSTATUS status = RegOpenKeyExA( HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows Kits\\Installed Roots", 0, KEY_QUERY_VALUE | KEY_WOW64_32KEY | KEY_ENUMERATE_SUB_KEYS, &key );

	if ( status != ERROR_SUCCESS ) {
		error(
			"Failed to get Windows SDK installation directory from your Windows registry.  This likely means you don't have the Windows SDK installed on your machine.\n"
			"In order to build using MSVC (which you asked me to do) then you will need to install a version of the Windows SDK on your PC.\n"
		);

		return false;
	}

	defer( RegCloseKey( key ) );

	const char *windowsSDKRoot = FindRegistryValueFromKey( key, "KitsRoot10" );

	assert( windowsSDKRoot );

	// get the latest version of the windows sdk
	const char *windowsSDKFolder = tprintf( "%sLib", windowsSDKRoot );

	if ( !file_get_all_files_in_folder( windowsSDKFolder, false, true, OnWindowsSDKVersionFound, outSDK ) ) {
		error( "Failed to query your Windows SDK root folder for the version of the Windows SDK that you asked for.  Do you definitely have it installed?\n" );
		return false;
	}

	const char *versionStr = tprintf( "%d.%d.%d.%d", outSDK->version.v0, outSDK->version.v1, outSDK->version.v2, outSDK->version.v3 );

	string_printf( &outSDK->ucrtInclude,   "%sinclude\\%s\\ucrt",   windowsSDKRoot, versionStr );
	string_printf( &outSDK->umInclude,     "%sinclude\\%s\\um",     windowsSDKRoot, versionStr );
	string_printf( &outSDK->sharedInclude, "%sinclude\\%s\\shared", windowsSDKRoot, versionStr );

	string_printf( &outSDK->ucrtLibPath, "%sLib\\%s\\ucrt\\x64", windowsSDKRoot, versionStr );
	string_printf( &outSDK->umLibPath,   "%sLib\\%s\\um\\x64",   windowsSDKRoot, versionStr );

	return true;
}

#endif
