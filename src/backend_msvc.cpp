/*
===========================================================================

Builder

Copyright (c) 2025 Dan Moody

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

===========================================================================
*/

#ifdef _WIN32

#include "builder_local.h"

#include "core/include/debug.h"
#include "core/include/string_helpers.h"
#include "core/include/paths.h"
#include "core/include/array.inl"
#include "core/include/string_builder.h"
#include "core/include/core_process.h"
#include "core/include/file.h"
#include "core/include/temp_storage.h"


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

struct msvcVersion_t {
	s32	v0, v1, v2;
};

struct windowsSDKVersion_t {
	s32	v0, v1, v2, v3;
};

struct msvcState_t {
	String						compilerPath;
	String						compilerVersion;
	String						linkerPath;

	Array<const char *>			args;

	// windows sdk includes, msvc includes, that kind of thing
	std::vector<std::string>	microsoftCoreIncludes;
	std::vector<std::string>	microsoftCoreLibPaths;

	std::vector<std::string>	includeDependencies;

	windowsSDKVersion_t			windowsSDKVersion;
};

//================================================================

// TODO(DM): 20/07/2025: do we want to ignore this warning via the build script?
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wswitch"

static const char *LanguageVersionToCompilerArg( const LanguageVersion languageVersion ) {
	assert( languageVersion != LANGUAGE_VERSION_UNSET );

	switch ( languageVersion ) {
		case LANGUAGE_VERSION_C89:		return "/std:c89";
		case LANGUAGE_VERSION_C99:		return "/std:c99";
		case LANGUAGE_VERSION_C11:		return "/std:c11";
		case LANGUAGE_VERSION_C17:		return "/std:c17";
		case LANGUAGE_VERSION_C23:		return "/std:c23";
		case LANGUAGE_VERSION_CPP11:	return "/std:c++11";
		case LANGUAGE_VERSION_CPP14:	return "/std:c++14";
		case LANGUAGE_VERSION_CPP17:	return "/std:c++17";
		case LANGUAGE_VERSION_CPP20:	return "/std:c++20";
		case LANGUAGE_VERSION_CPP23:	return "/std:c++23";
	}

	return NULL;
}

#pragma clang diagnostic pop

static const char *OptimizationLevelToCompilerArg( const OptimizationLevel level ) {
	switch ( level ) {
		case OPTIMIZATION_LEVEL_O0:	return "/Od";
		case OPTIMIZATION_LEVEL_O1:	return "/O1";
		case OPTIMIZATION_LEVEL_O2:	return "/O2";
		case OPTIMIZATION_LEVEL_O3:	return "/O2";	// DM!!! 22/07/2025: whats the real answer here?
	}
}

static void OnMSVCVersionFound( const FileInfo *fileInfo, void *userData ) {
	if ( !fileInfo->is_directory ) {
		return;
	}

	Array<const char *> *msvcVersions = cast( Array<const char *> *, userData );

	( *msvcVersions ).add( tprintf( fileInfo->filename ) );
}

static void OnWindowsSDKVersionFound( const FileInfo *fileInfo, void *data ) {
	if ( !fileInfo->is_directory ) {
		return;
	}

	msvcState_t *msvcState = cast( msvcState_t *, data );

	s32 v0 = 0, v1 = 0, v2 = 0, v3 = 0;
	sscanf( fileInfo->filename, "%d.%d.%d.%d", &v0, &v1, &v2, &v3 );

	if ( v0 < msvcState->windowsSDKVersion.v0 ) return;
	if ( v1 < msvcState->windowsSDKVersion.v1 ) return;
	if ( v2 < msvcState->windowsSDKVersion.v2 ) return;
	if ( v3 < msvcState->windowsSDKVersion.v3 ) return;

	msvcState->windowsSDKVersion.v0 = v0;
	msvcState->windowsSDKVersion.v1 = v1;
	msvcState->windowsSDKVersion.v2 = v2;
	msvcState->windowsSDKVersion.v3 = v3;
}

static const char *FindRegistryValueFromKey( const HKEY key, const char *valueName ) {
	DWORD valueStrLength = 0;

	LSTATUS status = RegQueryValueExA( key, valueName, NULL, NULL, NULL, &valueStrLength );

	if ( status != ERROR_SUCCESS ) {
		return NULL;
	}

	char *valueStr = cast( char *, mem_temp_alloc( ( valueStrLength + 1 ) * sizeof( char ) ) );

	//while ( 1 )
	{
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
	}

	return NULL;
}

//================================================================

// Querying for Windows SDK and MSVC is based off code from https://gist.github.com/andrewrk/ffb272748448174e6cdb4958dae9f3d8
// DM: I'm just straight lifting stuff from the code listing linked above - idc anymore
// its ridiculous that Microsoft genuinely think this isnt a frankly retarded way of grabbing some simple information off your PC

static bool8 MSVC_Init( compilerBackend_t *backend, const std::string &compilerPath, const std::string &compilerVersion ) {
	msvcState_t *msvcState = cast( msvcState_t *, mem_alloc( sizeof( msvcState_t ) ) );
	new( msvcState ) msvcState_t;

	msvcState->windowsSDKVersion = {};
	msvcState->compilerPath = compilerPath.c_str();
	msvcState->compilerVersion = compilerVersion.c_str();

	backend->data = msvcState;

	// get the latest version of the windows sdk
	// DM: you think this is bad? you aint seen nothing yet! see the next code block down!
	{
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

		if ( !file_get_all_files_in_folder( windowsSDKFolder, false, true, OnWindowsSDKVersionFound, msvcState ) ) {
			error( "Failed to query your Windows SDK root folder for the version of the Windows SDK that you asked for.  Do you definitely have it installed?\n" );
			return false;
		}

		msvcState->microsoftCoreIncludes.push_back( tprintf( "%sinclude\\%d.%d.%d.%d\\ucrt", windowsSDKRoot, msvcState->windowsSDKVersion.v0, msvcState->windowsSDKVersion.v1, msvcState->windowsSDKVersion.v2, msvcState->windowsSDKVersion.v3 ) );

		msvcState->microsoftCoreLibPaths.push_back( tprintf( "%sLib\\%d.%d.%d.%d\\ucrt\\x64", windowsSDKRoot, msvcState->windowsSDKVersion.v0, msvcState->windowsSDKVersion.v1, msvcState->windowsSDKVersion.v2, msvcState->windowsSDKVersion.v3 ) );
		msvcState->microsoftCoreLibPaths.push_back( tprintf( "%sLib\\%d.%d.%d.%d\\um\\x64", windowsSDKRoot, msvcState->windowsSDKVersion.v0, msvcState->windowsSDKVersion.v1, msvcState->windowsSDKVersion.v2, msvcState->windowsSDKVersion.v3 ) );
	}

	// get all versions of MSVC
	// thanks to Microsoft we will be doing that in the most retarded way possible
	// DM: I can only assume at this point that Microsoft are running some sick social experiment to see just how much unnecessary work they can put a programmer through before they snap
	if ( string_equals( compilerPath.c_str(), "cl" ) || string_equals( compilerPath.c_str(), "cl.exe" ) ) {
		HRESULT hr = S_OK;

		hr = CoInitializeEx( NULL, COINIT_MULTITHREADED );

		if ( FAILED( hr ) ) {
			return false;
		}

		defer( CoUninitialize() );

		GUID myUID						= { 0x42843719, 0xDB4C, 0x46C2, { 0x8E, 0x7C, 0x64, 0xF1, 0x81, 0x6E, 0xFD, 0x5B } };
		GUID CLSID_SetupConfiguration	= { 0x177F0C4A, 0x1CD3, 0x4DE7, { 0xA3, 0x2C, 0x71, 0xDB, 0xBB, 0x9F, 0xA3, 0x6D } };

		ISetupConfiguration *setupConfig = NULL;
		hr = CoCreateInstance( CLSID_SetupConfiguration, NULL, CLSCTX_INPROC_SERVER, myUID, cast( void **, &setupConfig ) );

		if ( FAILED( hr ) ) {
			return false;
		}

		defer( setupConfig->Release() );

		IEnumSetupInstances *instances = NULL;
		hr = setupConfig->EnumInstances( &instances );

		if ( FAILED( hr ) ) {
			return false;
		}

		if ( !instances ) {
			return false;
		}

		defer( instances->Release() );

		//Array<const char *> msvcRootFolders;
		const char *msvcRootFolder;
		Array<const char *>	foundMSVCVersions;

		ULONG found = 0;
		ISetupInstance *instance = NULL;

		hr = instances->Next( 1, &instance, &found );

		while ( found ) {
			char *visualStudioInstallationPath = NULL;
			BSTR visualStudioInstallationPathWide = NULL;
			hr = instance->GetInstallationPath( &visualStudioInstallationPathWide );

			if ( FAILED( hr ) ) {
				return false;
			}

			defer( SysFreeString( visualStudioInstallationPathWide ) );

			// TODO(DM): GetInstallationPath() returns a wide string but Core doesnt support those yet
			// make Core support wide strings and then remove this conversion
			{
				UINT wideLength = SysStringLen( visualStudioInstallationPathWide );

				int utf8Length = WideCharToMultiByte( CP_UTF8, 0, visualStudioInstallationPathWide, trunc_cast( int, wideLength ), NULL, 0, NULL, NULL );

				if ( utf8Length < 0 ) {
					return false;
				}

				visualStudioInstallationPath = cast( char *, mem_temp_alloc( trunc_cast( u64, utf8Length + 1 ) * sizeof( char ) ) );

				int converted = WideCharToMultiByte( CP_UTF8, 0, visualStudioInstallationPathWide, trunc_cast( int, wideLength ), visualStudioInstallationPath, utf8Length, NULL, NULL );

				if ( !converted ) {
					return false;
				}

				visualStudioInstallationPath[utf8Length] = 0;
			}

			msvcRootFolder = tprintf( "%s\\VC\\Tools\\MSVC", visualStudioInstallationPath );

			if ( !file_get_all_files_in_folder( msvcRootFolder, false, true, OnMSVCVersionFound, &foundMSVCVersions ) ) {
				return false;
			}

			instance->Release();

			hr = instances->Next( 1, &instance, &found );
		}

		// if no matching version is found then just use the newest version
		u32 useVersionIndex = 0;

		For ( u32, versionIndex, 0, foundMSVCVersions.count ) {
			if ( string_equals( foundMSVCVersions[versionIndex], compilerVersion.c_str() ) ) {
				useVersionIndex = versionIndex;

				break;
			}
		}

		msvcState->compilerVersion = foundMSVCVersions[useVersionIndex];

		string_printf( &msvcState->compilerPath, "%s\\%s\\bin\\Hostx64\\x64\\cl", msvcRootFolder, msvcState->compilerVersion.data );
		string_printf( &msvcState->linkerPath, "%s\\%s\\bin\\Hostx64\\x64\\link", msvcRootFolder, msvcState->compilerVersion.data );

		msvcState->microsoftCoreIncludes.push_back( tprintf( "%s\\%s\\include", msvcRootFolder, msvcState->compilerVersion.data ) );
		msvcState->microsoftCoreLibPaths.push_back( tprintf( "%s\\%s\\lib\\x64", msvcRootFolder, msvcState->compilerVersion.data ) );
	}

	return true;
}

static void MSVC_Shutdown( compilerBackend_t *backend ) {
	mem_free( backend->data );
	backend->data = NULL;
}

static bool8 MSVC_CompileSourceFile(
	compilerBackend_t *backend,
	buildContext_t *buildContext,
	BuildConfig *config,
	compilationCommandArchetype_t &cmdArchetype,
	const char *sourceFile,
	bool recordCompilation )
{
	assert( backend );
	assert( sourceFile );
	assert( config );

	const char *sourceFileNoPath = path_remove_path_from_file( sourceFile );
	const char *sourceFileNoExtension = path_remove_file_extension( sourceFileNoPath );
	const char *intermediatePath = tprintf( "%s%c%s", config->binaryFolder.c_str(), PATH_SEPARATOR, INTERMEDIATE_PATH );

	config->additionalIncludes.emplace_back( "." );

	msvcState_t *msvcState = cast( msvcState_t *, backend->data );

	msvcState->includeDependencies.clear();

	Array<const char *> finalArgs = cmdArchetype.baseArgs;

	const char *intermediateFile = tprintf( "%s%c%s.o", intermediatePath, PATH_SEPARATOR, sourceFileNoExtension );

	// Fill up remaining arguments

	// Output Flag/File
	finalArgs.add( tprintf( "%s%s", cmdArchetype.outputFlag, intermediateFile ) );

	// Source File
	finalArgs.add( sourceFile );

	// MSVC doesnt output include dependencies to .d files
	// it only supports printing them to stdout
	// so we have to parse the stdout of the process ourselves
	procFlags_t procFlags = 0;
	if ( buildContext->consolidateCompilerArgs ) {
		printf( "%s -> %s\n", sourceFile, intermediateFile );
	} else {
		procFlags = PROC_FLAG_SHOW_ARGS;
	}

	String processStdout;
	s32 exitCode = RunProc( &finalArgs, NULL, procFlags, &processStdout );

	// now parse the stdout
	// all include dependencies are on their own line
	// the line always starts with a specific prefix
	{
		const char *buffer = processStdout.data;

		const char *includeDependencyPrefix = "Note: including file: ";
		const u64 includeDependencyPrefixLength = strlen( includeDependencyPrefix );

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wcast-qual"
		char *lineStart = cast( char *, buffer );
#pragma clang diagnostic pop

		while ( *lineStart ) {
			char *lineEnd = strchr( lineStart, '\n' );

			if ( !lineEnd ) {
				continue;
			}

			u64 lineLength = cast( u64, lineEnd ) - cast( u64, lineStart );
			std::string bufferLine( lineStart, lineLength );

			if ( string_ends_with( bufferLine.c_str(), "\r" ) ) {
				bufferLine.pop_back();
			}

			if ( string_starts_with( bufferLine.c_str(), includeDependencyPrefix ) ) {
				bufferLine.erase( 0, includeDependencyPrefixLength );

				while ( bufferLine[0] == ' ' ) {
					bufferLine.erase( 0, 1 );
				}

				msvcState->includeDependencies.push_back( bufferLine );
			} else {
				printf( "%s\n", bufferLine.c_str() );
			}

			lineStart = lineEnd + 1;
		}
	}

	if ( recordCompilation ) {
		RecordCompilationDatabaseEntry( buildContext, sourceFile, finalArgs );
	}

	return exitCode == 0;
}

static bool8 MSVC_LinkIntermediateFiles( compilerBackend_t *backend, const Array<const char *> &intermediateFiles, BuildConfig *config ) {
	assert( backend );
	assert( config );

	const char *fullBinaryName = BuildConfig_GetFullBinaryName( config );

	msvcState_t *msvcState = cast( msvcState_t *, backend->data );
	Array<const char *> &args = msvcState->args;
	args.reserve(
		1 +	// link
		1 +	// /lib or /shared
		1 +	// /DEBUG
		1 +	// /OUT:<name>
		intermediateFiles.count +
		msvcState->microsoftCoreLibPaths.size() +
		config->additionalLibPaths.size() +
		config->additionalLibs.size()
	);

	args.reset();

	args.add( msvcState->linkerPath.data );

	if ( config->binaryType == BINARY_TYPE_STATIC_LIBRARY ) {
		args.add( "/lib" );
	} else if ( config->binaryType == BINARY_TYPE_DYNAMIC_LIBRARY ) {
		args.add( "/DLL" );
	}

	if ( !config->removeSymbols ) {
		args.add( "/DEBUG" );
	}

	args.add( tprintf( "/OUT:%s", fullBinaryName ) );

	args.add_range( &intermediateFiles );

	For ( u32, libPathIndex, 0, msvcState->microsoftCoreLibPaths.size() ) {
		args.add( tprintf( "/LIBPATH:%s", msvcState->microsoftCoreLibPaths[libPathIndex].c_str() ) );
	}

	For ( u32, libPathIndex, 0, config->additionalLibPaths.size() ) {
		args.add( tprintf( "/LIBPATH:%s", config->additionalLibPaths[libPathIndex].c_str() ) );
	}

	For ( u32, libIndex, 0, config->additionalLibs.size() ) {
		args.add( config->additionalLibs[libIndex].c_str() );
	}

	s32 exitCode = RunProc( &args, NULL, PROC_FLAG_SHOW_ARGS | PROC_FLAG_SHOW_STDOUT );

	return exitCode == 0;
}

static bool8 MSVC_GetCompilationCommandArchetype( const compilerBackend_t *backend, const BuildConfig *config, compilationCommandArchetype_t &outCmdArchetype ) {
	msvcState_t *msvcState = cast( msvcState_t *, backend->data );

	const u64 definesCount = config->defines.size();
	const u64 microsoftCoreIncludesCount = msvcState->microsoftCoreIncludes.size();
	const u64 additionalIncludesCount = config->additionalIncludes.size();
	const u64 ignoredWarningsCount = config->ignoreWarnings.size();
	const u64 additionalArgsCount = config->additionalCompilerArguments.size();

	Array<const char *> &baseArgs = outCmdArchetype.baseArgs;
	baseArgs.reserve(
		1 +	// compilerPath
		1 +	// compile flag
		1 +	// lang version flag
		1 +	// symbols flag
		1 +	// opt level flag
		definesCount +
		microsoftCoreIncludesCount +
		additionalIncludesCount +
		1 +	// warning as error flag
		1 +	// warning level flag
		ignoredWarningsCount +
		additionalArgsCount
	);

	// Compiler Path
	baseArgs.add( msvcState->compilerPath.data );

	// Compile Flag
	baseArgs.add( "/c" );

	// Language Version
	if ( config->languageVersion != LANGUAGE_VERSION_UNSET ) {
		baseArgs.add( LanguageVersionToCompilerArg( config->languageVersion ) );
	}

	// Symbols Flag
	if ( !config->removeSymbols ) {
		baseArgs.add( "/DEBUG" );
	}

	// Optimization Level
	baseArgs.add( OptimizationLevelToCompilerArg( config->optimizationLevel ) );

	// Diagnostics Flag
	baseArgs.add( "/showIncludes" );

	// Defines
	For ( u32, defineIndex, 0, definesCount ) {
		baseArgs.add( tprintf( "/D%s", config->defines[defineIndex].c_str() ) );
	}

	// Microsoft Core Includes
	For ( u32, includeIndex, 0, microsoftCoreIncludesCount ) {
		baseArgs.add( tprintf( "/I%s", msvcState->microsoftCoreIncludes[includeIndex].c_str() ) );
	}

	// Additional Includes
	For ( u32, includeIndex, 0, additionalIncludesCount ) {
		baseArgs.add( tprintf( "/I%s", config->additionalIncludes[includeIndex].c_str() ) );
	}

	// Warning As Error
	if ( config->warningsAsErrors ) {
		baseArgs.add( "/WX" );
	}

	// Warning Level
	{
		std::vector<std::string> allowedWarningLevels = {
			"/W",
			"/W0",
			"/W1",
			"/W2",
			"/W3",
			"/W4",
			"/Wall",
		};

		// MSVC only allows one warning level to be set
		if ( config->warningLevels.size() > 1 ) {
			StringBuilder builder;
			string_builder_reset( &builder );

			string_builder_appendf( &builder, "MSVC only allows ONE of the following warning levels to be set:\n" );

			For ( u64, allowedWarningLevelIndex, 0, allowedWarningLevels.size() ) {
				string_builder_appendf( &builder, "%s, ", allowedWarningLevels[allowedWarningLevelIndex].c_str() );
			}

			error( "%s\n", string_builder_to_string( &builder ) );

			return false;
		}

		For ( u64, warningLevelIndex, 0, config->warningLevels.size() ) {
			const std::string &warningLevel = config->warningLevels[warningLevelIndex];

			bool8 found = false;

			For ( u64, allowedWarningLevelIndex, 0, allowedWarningLevels.size() ) {
				if ( allowedWarningLevels[allowedWarningLevelIndex] == warningLevel ) {	// TODO(DM): 14/06/2025: better to compare hashes here instead?
					found = true;
					break;
				}
			}

			if ( !found ) {
				error( "\"%s\" is not allowed as a warning level.\n", warningLevel.c_str() );
				return false;
			}

			baseArgs.add( warningLevel.c_str() );
		}
	}

	// Ignored Warnings
	For ( u64, ignoreWarningIndex, 0, ignoredWarningsCount ) {
		baseArgs.add( config->ignoreWarnings[ignoreWarningIndex].c_str() );
	}

	// Additional Arguments
	For ( u64, additionalArgumentIndex, 0, additionalArgsCount ) {
		baseArgs.add( config->additionalCompilerArguments[additionalArgumentIndex].c_str() );
	}

	// Dependency Flags
	// MSVC doesn't have any

	// Output Flag
	outCmdArchetype.outputFlag = "/Fo";

	return true;
}

static void MSVC_GetIncludeDependenciesFromSourceFileBuild( compilerBackend_t *backend, std::vector<std::string> &outIncludeDependencies ) {
	msvcState_t *msvcState = cast( msvcState_t *, backend->data );

	outIncludeDependencies = msvcState->includeDependencies;
}

static String MSVC_GetCompilerPath( compilerBackend_t *backend ) {
	msvcState_t *msvcState = cast( msvcState_t *, backend->data );

	return msvcState->compilerPath;
}

static String MSVC_GetCompilerVersion( compilerBackend_t *backend ) {
	msvcState_t *msvcState = cast( msvcState_t *, backend->data );

	return msvcState->compilerVersion;
}

void CreateCompilerBackend_MSVC( compilerBackend_t *outBackend ) {
	*outBackend = compilerBackend_t {
		.data										= NULL,
		.Init										= MSVC_Init,
		.Shutdown									= MSVC_Shutdown,
		.CompileSourceFile							= MSVC_CompileSourceFile,
		.LinkIntermediateFiles						= MSVC_LinkIntermediateFiles,
		.GetCompilationCommandArchetype				= MSVC_GetCompilationCommandArchetype,
		.GetIncludeDependenciesFromSourceFileBuild	= MSVC_GetIncludeDependenciesFromSourceFileBuild,
		.GetCompilerPath							= MSVC_GetCompilerPath,
		.GetCompilerVersion							= MSVC_GetCompilerVersion,
	};
}

#endif // _WIN32