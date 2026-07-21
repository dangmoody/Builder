#include "win_support.h"
#ifdef _WIN32

#include "win_support.h"
#include "builder_local.h"

#include "typecast.h"
#include "file.h"
#include "temp_storage.h"
#include "string.h"
#include "array.inl"
#include "string_builder.h"
#include "defer.h"
#include "helpers.h"
#include "paths.h"

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

static void OnWindowsSDKVersionFound( const fileInfo_t *fileInfo, void *data ) {
	Array<windowsSDKVersion_t> *versions = cast( Array<windowsSDKVersion_t> *, data );

	s32 v0 = 0, v1 = 0, v2 = 0, v3 = 0;
	sscanf( fileInfo->filename, "%d.%d.%d.%d", &v0, &v1, &v2, &v3 );

	versions->Add( { v0, v1, v2, v3 } );
}

static const char *FindRegistryValueFromKey( const HKEY key, const char *valueName ) {
	DWORD valueStrLength = 0;

	LSTATUS status = RegQueryValueExA( key, valueName, NULL, NULL, NULL, &valueStrLength );

	if ( status != ERROR_SUCCESS ) {
		return NULL;
	}

	// valueStrLength from RegQueryValueExA includes the null terminator for REG_SZ strings
	char *valueStr = cast( char *, Mem_TempAlloc( valueStrLength * sizeof( char ) ) );

	DWORD type;
	status = RegQueryValueExA( key, valueName, NULL, &type, cast( LPBYTE, valueStr ), &valueStrLength );

	if ( type != REG_SZ ) {
		return NULL;
	}

	if ( status == ERROR_SUCCESS ) {
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

bool8 Win_GetWindowsSDK( LinearAllocator *allocator, windowsSDK_t *outSDK ) {
	assert( allocator );
	assert( outSDK );

	HKEY key;

	const char *winSDKRegPath = "SOFTWARE\\Microsoft\\Windows Kits\\Installed Roots";
	LSTATUS status = RegOpenKeyExA( HKEY_LOCAL_MACHINE, winSDKRegPath, 0, KEY_QUERY_VALUE | KEY_WOW64_32KEY | KEY_ENUMERATE_SUB_KEYS, &key );

	if ( status != ERROR_SUCCESS ) {
		error(
			"Failed to get Windows SDK installation directory from your Windows registry.  The registry path \"%s\" doesn't seem to exist on your machine.\n"
			"This likely means you don't have the Windows SDK installed on your machine.\n"
			"In order to build using MSVC (which you asked me to do) then you will need to install a version of the Windows SDK on your PC.\n"
			, winSDKRegPath
		);

		return false;
	}

	defer { RegCloseKey( key ); };

	const char *winSDKRegKey = "KitsRoot10";
	const char *windowsSDKRoot = FindRegistryValueFromKey( key, winSDKRegKey );

	if ( !windowsSDKRoot ) {
		error(
			"Failed to get Windows SDK installation directory from your Windows registry.  The registry key \"%s\" couldn't be queried from the registry path: \"%s\"\n"
			"This likely means you don't have the Windows SDK installed on your machine.\n"
			"In order to build using MSVC (which you asked me to do) then you will need to install a version of the Windows SDK on your PC.\n"
			, winSDKRegPath
			, winSDKRegKey
		);

		return false;
	}

	outSDK->rootFolder = String_Alloc( allocator, windowsSDKRoot, strlen( windowsSDKRoot ) + 1 );

	// get the latest version of the windows sdk
	const char *windowsSDKFolder = TempPrintf( "%sLib", windowsSDKRoot );

	Array<windowsSDKVersion_t> versions;
	versions.Init( Mem_GetTempStorage() );

	if ( !FS_GetAllFilesInFolder( windowsSDKFolder, FILE_VISIT_FOLDERS, OnWindowsSDKVersionFound, &versions ) ) {
		error( "Failed to query your Windows SDK root folder for the version of the Windows SDK that you asked for.  Do you definitely have at least one version of the Windows SDK installed?\n" );
		return false;
	}

	// newest version first
	qsort( versions.data, versions.count, sizeof( windowsSDKVersion_t ), CompareWindowsSDKVersions );

	bool8 foundVersion = false;

	// find the first windows SDK folder that isnt malformed
	For ( u32, versionIndex, 0, versions.count ) {
		windowsSDKVersion_t *version = &versions[versionIndex];

		Array<const char *> missingFolders;
		missingFolders.Init( Mem_GetTempStorage() );
		missingFolders.Reserve( 5 );

		// TODO(DM): 21/04/2026: rewind temp storage after we are done with this?
		const char *ucrtIncludeFolder = TempPrintf( "%sinclude\\%d.%d.%d.%d\\ucrt", outSDK->rootFolder.data, version->v0, version->v1, version->v2, version->v3 );
		const char *umIncludeFolder = TempPrintf( "%sinclude\\%d.%d.%d.%d\\um", outSDK->rootFolder.data, version->v0, version->v1, version->v2, version->v3 );
		const char *sharedIncludeFolder = TempPrintf( "%sinclude\\%d.%d.%d.%d\\shared", outSDK->rootFolder.data, version->v0, version->v1, version->v2, version->v3 );
		const char *ucrtLibFolder = TempPrintf( "%sLib\\%d.%d.%d.%d\\ucrt\\x64", outSDK->rootFolder.data, version->v0, version->v1, version->v2, version->v3 );
		const char *umLibFolder = TempPrintf( "%sLib\\%d.%d.%d.%d\\um\\x64", outSDK->rootFolder.data, version->v0, version->v1, version->v2, version->v3 );

		if ( !FS_FolderExists( ucrtIncludeFolder ) ) {
			missingFolders.Add( ucrtIncludeFolder );
		}

		if ( !FS_FolderExists( umIncludeFolder ) ) {
			missingFolders.Add( umIncludeFolder );
		}

		if ( !FS_FolderExists( sharedIncludeFolder ) ) {
			missingFolders.Add( sharedIncludeFolder );
		}

		if ( !FS_FolderExists( ucrtLibFolder ) ) {
			missingFolders.Add( ucrtLibFolder );
		}

		if ( !FS_FolderExists( umLibFolder ) ) {
			missingFolders.Add( umLibFolder );
		}

		if ( missingFolders.count > 0 ) {
			StringBuilder sb = SB_Create( Mem_GetTempStorage() );
			//defer { string_builder_destroy( &sb ); };
			SB_Appendf( &sb, "Version %d.%d.%d.%d of your Windows SDK installation is malformed because the following folders could not be found:\n", version->v0, version->v1, version->v2, version->v3 );
			For ( u32, missingFolderIndex, 0, missingFolders.count ) {
				SB_Appendf( &sb, " - %s\n", missingFolders[missingFolderIndex] );
			}
			SB_Appendf( &sb, "If you want to use this version of the Windows SDK specifically, you will need to fix this yourself.\n" );

			warning( "%s\n", SB_ToString( &sb ) );

			continue;
		}

		outSDK->version = *version;

		foundVersion = true;

		break;
	}

	if ( !foundVersion ) {
		error(
			"Failed to find a valid installation of the Windows SDK on your machine.\n"
			"You have %llu versions of the Windows SDK installed on your machine, and somehow all of them appear to be malformed.\n"
			"You need to install a version through the Visual Studio Installer, or via the separate Build Tools installer from Microsoft.\n"
			, versions.count
		);

		return false;
	}

	const char *versionStr = TempPrintf( "%d.%d.%d.%d", outSDK->version.v0, outSDK->version.v1, outSDK->version.v2, outSDK->version.v3 );

	outSDK->ucrtInclude = path_join( allocator, windowsSDKRoot, "include", versionStr, "ucrt" );
	outSDK->umInclude = path_join( allocator, windowsSDKRoot, "include", versionStr, "um" );
	outSDK->sharedInclude = path_join( allocator, windowsSDKRoot, "include", versionStr, "shared" );
	outSDK->ucrtLibPath = path_join( allocator, windowsSDKRoot, "Lib", versionStr, "ucrt", "x64" );
	outSDK->umLibPath = path_join( allocator, windowsSDKRoot, "Lib", versionStr, "um", "x64" );

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
	LinearAllocator				*allocator;
	const char					*rootFolder;
	std::vector<msvcInstall_t>	*installs;
};

static void OnMSVCInstallFound( const fileInfo_t *fileInfo, void *userData ) {
	foundMSVCInstallData_t *data = cast( foundMSVCInstallData_t *, userData );

	msvcInstall_t install = {};

	sscanf( fileInfo->filename, "%d.%d.%d", &install.version.v0, &install.version.v1, &install.version.v2 );

	install.rootFolder = path_join( data->allocator, data->rootFolder, fileInfo->filename );
	install.includePath = path_join( data->allocator, install.rootFolder.data, "include" );
	install.libPath = path_join( data->allocator, install.rootFolder.data, "lib", "x64" );

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
bool8 Win_GetMSVCInstall( LinearAllocator *allocator, msvcInstall_t *outInstall ) {
	assert( allocator );
	assert( outInstall );

	LogVerbose( "Querying for MSVC installations...\n" );

	HRESULT hr = S_OK;

	hr = CoInitializeEx( NULL, COINIT_MULTITHREADED );

	if ( FAILED( hr ) ) {
		error( "CoInitializeEx() call failed: 0x%X\n", hr );
		return false;
	}

	defer { CoUninitialize(); };

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

	defer { setupConfig->Release(); };

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

	defer { instances->Release(); };

	const char *msvcRootFolder;
	std::vector<msvcInstall_t> foundMSVCInstalls;	// DM!!! this doesnt need to be a vector

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

		defer { SysFreeString( visualStudioInstallationPathWide ); };

		// TODO(DM): GetInstallationPath() returns a wide string but Core doesnt support those yet
		// make Core support wide strings and then remove this conversion
		{
			UINT wideLength = SysStringLen( visualStudioInstallationPathWide );

			int utf8Length = WideCharToMultiByte( CP_UTF8, 0, visualStudioInstallationPathWide, trunc_cast( int, wideLength ), NULL, 0, NULL, NULL );

			if ( utf8Length <= 0 ) {
				error( "First WideCharToMultiByte() call failed: WinAPI error code 0x%X\n", GetLastError() );
				return false;
			}

			visualStudioInstallationPath = cast( char*, Mem_TempAlloc( trunc_cast( u64, utf8Length + 1 ) * sizeof( char ) ) );

			int converted = WideCharToMultiByte( CP_UTF8, 0, visualStudioInstallationPathWide, trunc_cast( int, wideLength ), visualStudioInstallationPath, utf8Length, NULL, NULL );

			if ( !converted ) {
				error( "Second WideCharToMultiByte() call failed: WinAPI error code 0x%X\n", GetLastError() );
				return false;
			}

			visualStudioInstallationPath[utf8Length] = 0;
		}

		msvcRootFolder = TempPrintf( "%s\\VC\\Tools\\MSVC", visualStudioInstallationPath );

		foundMSVCInstallData_t data = {
			.allocator	= allocator,
			.rootFolder	= msvcRootFolder,
			.installs	= &foundMSVCInstalls
		};

		if ( !FS_GetAllFilesInFolder( msvcRootFolder, FILE_VISIT_FOLDERS, OnMSVCInstallFound, &data ) ) {
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

		Array<const char *> missingFolders;
		missingFolders.Init( Mem_GetTempStorage() );
		missingFolders.Reserve( 2 );

		if ( !FS_FolderExists( install->includePath.data ) ) {
			missingFolders.Add( install->includePath.data );
		}

		if ( !FS_FolderExists( install->libPath.data ) ) {
			missingFolders.Add( install->libPath.data );
		}

		if ( missingFolders.count > 0 ) {
			StringBuilder sb = SB_Create( Mem_GetTempStorage() );
			//defer { string_builder_destroy( &sb ); };
			SB_Appendf( &sb, "Version %d.%d.%d of your MSVC installation is malformed because the following folders could not be found:\n", install->version.v0, install->version.v1, install->version.v2 );
			For ( u32, missingFolderIndex, 0, missingFolders.count ) {
				SB_Appendf( &sb, " - %s\n", missingFolders[missingFolderIndex] );
			}
			SB_Appendf( &sb, "If you want to use this version of MSVC specifically, you will need to fix this yourself.\n" );

			warning( "%s\n", SB_ToString( &sb ) );

			continue;
		}

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
