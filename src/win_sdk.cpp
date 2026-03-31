#ifdef _WIN32

#include "win_sdk.h"
#include "builder_local.h"

#include "core/include/typecast.inl"
#include "core/include/file.h"
#include "core/include/temp_storage.h"
#include "core/include/string_helpers.h"
#include "core/include/array.inl"

#include <Windows.h>

#include <stdio.h>
#include <assert.h>

static void OnWindowsSDKVersionFound( const FileInfo *fileInfo, void *data ) {
	if ( !fileInfo->is_directory ) {
		return;
	}

	Array<windowsSDKVersion_t> *versions = cast( Array<windowsSDKVersion_t> *, data );

	s32 v0 = 0, v1 = 0, v2 = 0, v3 = 0;
	sscanf( fileInfo->filename, "%d.%d.%d.%d", &v0, &v1, &v2, &v3 );

	versions->add( { v0, v1, v2, v3 } );
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

static int CompareVersions( const void *a, const void *b ) {
	const windowsSDKVersion_t *av = cast( const windowsSDKVersion_t *, a );
	const windowsSDKVersion_t *bv = cast( const windowsSDKVersion_t *, b );

	if ( av->v0 != bv->v0 ) return bv->v0 - av->v0;
	if ( av->v1 != bv->v1 ) return bv->v1 - av->v1;
	if ( av->v2 != bv->v2 ) return bv->v2 - av->v2;

	return bv->v3 - av->v3;
}

bool8 Win_GetSDK( windowsSDK_t *outSDK ) {
	assert( outSDK );

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

	LogVerbose( "Windows SDK root folder (as taken from the registry): \"%s\"\n", windowsSDKRoot );

	outSDK->rootFolder = windowsSDKRoot;

	// get the latest version of the windows sdk
	const char *windowsSDKFolder = tprintf( "%sLib", windowsSDKRoot );

	Array<windowsSDKVersion_t> versions;

	if ( !file_get_all_files_in_folder( windowsSDKFolder, false, true, OnWindowsSDKVersionFound, &versions ) ) {
		error( "Failed to query your Windows SDK root folder for the version of the Windows SDK that you asked for.  Do you definitely have at least one version of the Windows SDK installed?\n" );
		return false;
	}

	// newest version first
	qsort( versions.data, versions.count, sizeof( windowsSDKVersion_t ), CompareVersions );

	// find the first windows SDK folder that isnt malformed
	For ( u32, versionIndex, 0, versions.count ) {
		windowsSDKVersion_t *version = &versions[versionIndex];

		if ( !folder_exists( tprintf( "%sinclude\\%d.%d.%d.%d\\ucrt",   outSDK->rootFolder.data, version->v0, version->v1, version->v2, version->v3 ) ) ) continue;
		if ( !folder_exists( tprintf( "%sinclude\\%d.%d.%d.%d\\um",     outSDK->rootFolder.data, version->v0, version->v1, version->v2, version->v3 ) ) ) continue;
		if ( !folder_exists( tprintf( "%sinclude\\%d.%d.%d.%d\\shared", outSDK->rootFolder.data, version->v0, version->v1, version->v2, version->v3 ) ) ) continue;
		if ( !folder_exists( tprintf( "%sinclude\\%d.%d.%d.%d\\ucrt",   outSDK->rootFolder.data, version->v0, version->v1, version->v2, version->v3 ) ) ) continue;

		if ( !folder_exists( tprintf( "%sLib\\%d.%d.%d.%d\\ucrt\\x64", outSDK->rootFolder.data, version->v0, version->v1, version->v2, version->v3 ) ) ) continue;
		if ( !folder_exists( tprintf( "%sLib\\%d.%d.%d.%d\\um\\x64",   outSDK->rootFolder.data, version->v0, version->v1, version->v2, version->v3 ) ) ) continue;

		outSDK->version = *version;

		break;
	}

	if ( outSDK->version.v0 == -1 && outSDK->version.v1 == -1 && outSDK->version.v2 == -1 && outSDK->version.v3 == -1 ) {
		error(
			"Failed to find a valid installation of the Windows SDK on your machine.\n"
			"You have %llu versions of the Windows SDK installed on your machine, and somehow all of them appear to be malformed.\n"
			"You need to install a version through the Visual Studio Installer, or via the separate Build Tools installer from Microsoft.\n"
			, versions.count
		);

		return false;
	}

	const char *versionStr = tprintf( "%d.%d.%d.%d", outSDK->version.v0, outSDK->version.v1, outSDK->version.v2, outSDK->version.v3 );

	string_printf( &outSDK->ucrtInclude,   "%sinclude\\%s\\ucrt",   windowsSDKRoot, versionStr );
	string_printf( &outSDK->umInclude,     "%sinclude\\%s\\um",     windowsSDKRoot, versionStr );
	string_printf( &outSDK->sharedInclude, "%sinclude\\%s\\shared", windowsSDKRoot, versionStr );

	string_printf( &outSDK->ucrtLibPath, "%sLib\\%s\\ucrt\\x64", windowsSDKRoot, versionStr );
	string_printf( &outSDK->umLibPath,   "%sLib\\%s\\um\\x64",   windowsSDKRoot, versionStr );

	printf( "Using latest valid Windows SDK version that was found, which was: %s.\n\n", versionStr );

	return true;
}

#endif
