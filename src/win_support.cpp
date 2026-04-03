#include "win_support.h"
#ifdef _WIN32

#include "win_support.h"
#include "builder_local.h"

#include "core/include/typecast.inl"
#include "core/include/file.h"
#include "core/include/temp_storage.h"
#include "core/include/string_helpers.h"
#include "core/include/array.inl"

#include <Windows.h>

#include <stdio.h>
#include <assert.h>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wnon-virtual-dtor"

struct DECLSPEC_UUID( "B41463C3-8866-43B5-BC33-2B0676F7F42E" ) DECLSPEC_NOVTABLE ISetupInstance : public IUnknown {
	STDMETHOD( GetInstanceId )( _Out_ BSTR *pbstrInstanceId ) = 0;
	STDMETHOD( GetInstallDate )( _Out_ LPFILETIME pInstallDate ) = 0;
	STDMETHOD( GetInstallationName )( _Out_ BSTR *pbstrInstallationName ) = 0;
	STDMETHOD( GetInstallationPath )( _Out_ BSTR *pbstrInstallationPath ) = 0;
	STDMETHOD( GetInstallationVersion )( _Out_ BSTR *pbstrInstallationVersion ) = 0;
	STDMETHOD( GetDisplayName )( _In_ LCID lcid, _Out_ BSTR *pbstrDisplayName ) = 0;
	STDMETHOD( GetDescription )( _In_ LCID lcid, _Out_ BSTR *pbstrDescription ) = 0;
	STDMETHOD( ResolvePath )( _In_opt_z_ LPCOLESTR pwszRelativePath, _Out_ BSTR *pbstrAbsolutePath ) = 0;
};

struct DECLSPEC_UUID( "6380BCFF-41D3-4B2E-8B2E-BF8A6810C848" ) DECLSPEC_NOVTABLE IEnumSetupInstances : public IUnknown {
	STDMETHOD( Next )( _In_ ULONG celt, _Out_writes_to_( celt, *pceltFetched ) ISetupInstance **rgelt, _Out_opt_ _Deref_out_range_( 0, celt ) ULONG *pceltFetched ) = 0;
	STDMETHOD( Skip )( _In_ ULONG celt ) = 0;
	STDMETHOD( Reset )( void ) = 0;
	STDMETHOD( Clone )( _Deref_out_opt_ IEnumSetupInstances **ppenum ) = 0;
};

struct DECLSPEC_UUID( "42843719-DB4C-46C2-8E7C-64F1816EFD5B" ) DECLSPEC_NOVTABLE ISetupConfiguration : public IUnknown {
	STDMETHOD( EnumInstances )( _Out_ IEnumSetupInstances **ppEnumInstances ) = 0;
	STDMETHOD( GetInstanceForCurrentProcess )( _Out_ ISetupInstance **ppInstance ) = 0;
	STDMETHOD( GetInstanceForPath )( _In_z_ LPCWSTR wzPath, _Out_ ISetupInstance **ppInstance ) = 0;
};

#pragma clang diagnostic pop

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

static int CompareWindowsSDKVersions( const void *a, const void *b ) {
	const windowsSDKVersion_t *versionA = cast( const windowsSDKVersion_t *, a );
	const windowsSDKVersion_t *versionB = cast( const windowsSDKVersion_t *, b );

	if ( versionA->v0 != versionB->v0 ) return versionB->v0 - versionA->v0;
	if ( versionA->v1 != versionB->v1 ) return versionB->v1 - versionA->v1;
	if ( versionA->v2 != versionB->v2 ) return versionB->v2 - versionA->v2;

	return versionB->v3 - versionA->v3;
}

bool8 Win_GetWindowsSDK( windowsSDK_t *outSDK ) {
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

	outSDK->rootFolder = windowsSDKRoot;

	// get the latest version of the windows sdk
	const char *windowsSDKFolder = tprintf( "%sLib", windowsSDKRoot );

	Array<windowsSDKVersion_t> versions;

	if ( !file_get_all_files_in_folder( windowsSDKFolder, false, true, OnWindowsSDKVersionFound, &versions ) ) {
		error( "Failed to query your Windows SDK root folder for the version of the Windows SDK that you asked for.  Do you definitely have at least one version of the Windows SDK installed?\n" );
		return false;
	}

	// newest version first
	qsort( versions.data, versions.count, sizeof( windowsSDKVersion_t ), CompareWindowsSDKVersions );

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

	printf( "Using latest valid Windows SDK version that was found, which was: %s.\n", versionStr );

	LogVerbose(
		"Using Windows SDK with the following paths:\n"
		" - Root folder (as taken from your registry): \"%s\"\n"
		" - UCRT include folder:   \"%s\"\n"
		" - UM include folder:     \"%s\"\n"
		" - Shared include folder: \"%s\"\n"
		" - UCRT lib path:         \"%s\"\n"
		" - UM lib path:           \"%s\"\n"
		"\n"
		, outSDK->rootFolder.data
		, outSDK->ucrtInclude.data
		, outSDK->umInclude.data
		, outSDK->sharedInclude.data
		, outSDK->ucrtLibPath.data
		, outSDK->umLibPath.data
	);

	return true;
}


//================================================================

struct foundMSVCInstallData_t {
	const char					*rootFolder;
	std::vector<msvcInstall_t>	*installs;
};

static void OnMSVCInstallFound( const FileInfo *fileInfo, void *userData ) {
	if ( !fileInfo->is_directory ) {
		return;
	}

	foundMSVCInstallData_t *data = cast( foundMSVCInstallData_t *, userData );

	msvcInstall_t install = {};
	sscanf( fileInfo->filename, "%d.%d.%d", &install.version.v0, &install.version.v1, &install.version.v2 );

	string_printf( &install.rootFolder,  "%s\\%s", data->rootFolder, fileInfo->filename );
	string_printf( &install.includePath, "%s\\include", install.rootFolder.data );
	string_printf( &install.libPath,     "%s\\lib\\x64", install.rootFolder.data );

	LogVerbose( "Found MSVC installation located at: \"%s\"\n", install.rootFolder.data );

	data->installs->push_back( install );
}

static int CompareMSVCInstallVersions( const void *a, const void *b ) {
	const msvcInstall_t *installA = cast( const msvcInstall_t*, a );
	const msvcInstall_t *installB = cast( const msvcInstall_t*, b );

	const msvcVersion_t *versionA = &installA->version;
	const msvcVersion_t *versionB = &installB->version;

	if ( versionA->v0 != versionB->v0 ) return versionB->v0 - versionA->v0;
	if ( versionA->v1 != versionB->v1 ) return versionB->v1 - versionA->v1;

	return versionB->v2 - versionA->v2;
}

static bool8 MSVCNotInstalled() {
	error( "No valid MSVC installation found on your PC.  You need to install one through either the Visual Studio Installer or through the MS Build Tools.\n" );
	return false;
}

// get all versions of MSVC
// thanks to Microsoft we will be doing that in the most retarded way possible
bool8 Win_GetMSVCInstall( msvcInstall_t *outInstall ) {
	assert( outInstall );

	LogVerbose( "Querying for MSVC installations...\n" );

	HRESULT hr = S_OK;

	hr = CoInitializeEx( NULL, COINIT_MULTITHREADED );

	if ( FAILED( hr ) ) {
		error( "CoInitializeEx() call failed: 0x%X\n", hr );
		return false;
	}

	defer( CoUninitialize() );

	// wtf?
	GUID myUID						= { 0x42843719, 0xDB4C, 0x46C2, { 0x8E, 0x7C, 0x64, 0xF1, 0x81, 0x6E, 0xFD, 0x5B } };
	GUID CLSID_SetupConfiguration	= { 0x177F0C4A, 0x1CD3, 0x4DE7, { 0xA3, 0x2C, 0x71, 0xDB, 0xBB, 0x9F, 0xA3, 0x6D } };

	ISetupConfiguration *setupConfig = NULL;
	hr = CoCreateInstance( CLSID_SetupConfiguration, NULL, CLSCTX_INPROC_SERVER, myUID, cast( void **, &setupConfig ) );

	if ( hr == REGDB_E_CLASSNOTREG ) {
		return MSVCNotInstalled();
	}

	if ( FAILED( hr ) ) {
		error( "CoCreateInstance() call failed: 0x%X\n", hr );
		return false;
	}

	defer( setupConfig->Release() );

	IEnumSetupInstances *instances = NULL;
	hr = setupConfig->EnumInstances( &instances );

	if ( FAILED( hr ) ) {
		error( "setupConfig->EnumInstances() called failed: 0x%X\n", hr );
		return false;
	}

	if ( !instances ) {
		error( "setupConfig->EnumInstances() returned no instances.  Bailing...\n" );
		return false;
	}

	defer( instances->Release() );

	const char *msvcRootFolder;
	std::vector<msvcInstall_t> foundMSVCInstalls;

	ULONG foundInstance = 0;
	ISetupInstance *instance = NULL;

	hr = instances->Next( 1, &instance, &foundInstance );

	while ( foundInstance ) {
		char *visualStudioInstallationPath = NULL;
		BSTR visualStudioInstallationPathWide = NULL;
		hr = instance->GetInstallationPath( &visualStudioInstallationPathWide );

		if ( FAILED( hr ) ) {
			error( "instance->GetInstallationPath() call failed: 0x%X\n", hr );
			return false;
		}

		defer( SysFreeString( visualStudioInstallationPathWide ) );

		// TODO(DM): GetInstallationPath() returns a wide string but Core doesnt support those yet
		// make Core support wide strings and then remove this conversion
		{
			UINT wideLength = SysStringLen( visualStudioInstallationPathWide );

			int utf8Length = WideCharToMultiByte( CP_UTF8, 0, visualStudioInstallationPathWide, trunc_cast( int, wideLength ), NULL, 0, NULL, NULL );

			if ( utf8Length <= 0 ) {
				error( "First WideCharToMultiByte() call failed: WinAPI error code 0x%X\n", GetLastError() );
				return false;
			}

			visualStudioInstallationPath = cast( char*, mem_temp_alloc( trunc_cast( u64, utf8Length + 1 ) * sizeof( char ) ) );

			int converted = WideCharToMultiByte( CP_UTF8, 0, visualStudioInstallationPathWide, trunc_cast( int, wideLength ), visualStudioInstallationPath, utf8Length, NULL, NULL );

			if ( !converted ) {
				error( "Second WideCharToMultiByte() call failed: WinAPI error code 0x%X\n", GetLastError() );
				return false;
			}

			visualStudioInstallationPath[utf8Length] = 0;
		}

		msvcRootFolder = tprintf( "%s\\VC\\Tools\\MSVC", visualStudioInstallationPath );

		foundMSVCInstallData_t data = {
			.rootFolder	= msvcRootFolder,
			.installs	= &foundMSVCInstalls
		};

		if ( !file_get_all_files_in_folder( msvcRootFolder, false, true, OnMSVCInstallFound, &data ) ) {
			error( "Failed to query for all MSVC installation folders.  Error code: " ERROR_CODE_FORMAT "\n", get_last_error_code() );
			return false;
		}

		instance->Release();

		hr = instances->Next( 1, &instance, &foundInstance );
	}

	// newest version first
	qsort( foundMSVCInstalls.data(), foundMSVCInstalls.size(), sizeof( msvcInstall_t ), CompareMSVCInstallVersions );

	u32 useVersionIndex = 0;
	bool8 found = false;

	For ( u32, versionIndex, 0, foundMSVCInstalls.size() ) {
		msvcInstall_t *install = &foundMSVCInstalls[versionIndex];

		if ( !folder_exists( install->includePath.data ) ) continue;
		if ( !folder_exists( install->libPath.data ) ) continue;

		useVersionIndex = versionIndex;
		found = true;

		break;
	}

	if ( !found ) {
		return MSVCNotInstalled();
	}

	*outInstall = foundMSVCInstalls[useVersionIndex];

	printf( "Using latest valid MSVC version that was found, which was: %d.%d.%d\n", outInstall->version.v0, outInstall->version.v1, outInstall->version.v2 );

	LogVerbose(
		"Using MSVC with the following paths:\n"
		" - Root folder : \"%s\"\n"
		" - Include path: \"%s\"\n"
		" - Lib path    : \"%s\"\n"
		, outInstall->rootFolder.data
		, outInstall->includePath.data
		, outInstall->libPath.data
	);

	return true;
}

#endif
