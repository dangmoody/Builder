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

#include "builder_local.h"

#include "core/include/allocation_context.h"
#include "core/include/array.inl"
#include "core/include/string_helpers.h"
#include "core/include/string_builder.h"
#include "core/include/paths.h"
#include "core/include/core_process.h"
#include "core/include/file.h"
#include "core/include/typecast.inl"
#include "core/include/temp_storage.h"
#include "core/include/hash.h"
#include "core/include/timer.h"
#include "core/include/library.h"
#include "core/include/core_string.h"
#include "core/include/hashmap.h"

#ifdef _WIN64
#include <Shlwapi.h>
#endif

#include <stdio.h>

/*
=============================================================================

	Builder

	by Dan Moody

=============================================================================
*/

enum {
	BUILDER_VERSION_MAJOR	= 0,
	BUILDER_VERSION_MINOR	= 8,
	BUILDER_VERSION_PATCH	= 0,
};

enum buildResult_t {
	BUILD_RESULT_SUCCESS	= 0,
	BUILD_RESULT_FAILED,
	BUILD_RESULT_SKIPPED
};

#define SET_BUILDER_OPTIONS_FUNC_NAME	"set_builder_options"
#define PRE_BUILD_FUNC_NAME				"on_pre_build"
#define POST_BUILD_FUNC_NAME			"on_post_build"

#define QUIT_ERROR() \
	debug_break(); \
	return 1

#if USE_BUILD_INFO_FILES
struct builderVersion_t {
	s32	major;
	s32	minor;
	s32	patch;
};

struct buildInfoData_t {
	builderVersion_t									builderVersion;
	std::vector<BuildConfig>							configs;
	std::string											userConfigSourceFilename;
	std::string											userConfigDLLFilename;
	std::vector<std::vector<std::vector<std::string>>>	includeDependencies;	// [configIndex][sourceFileIndex][dependencyIndex]
};
#endif // USE_BUILD_INFO_FILES

errorCode_t GetLastErrorCode() {
#ifdef _WIN64
	return GetLastError();
#else
#error Unrecognised platform!
#endif
}

static u64 GetLastFileWriteTime( const char* filename ) {
	FileInfo fileInfo;
	File file = file_find_first( filename, &fileInfo );

	assert( file.ptr != INVALID_HANDLE_VALUE );

	return fileInfo.last_write_time;
}

static const char* GetFileExtensionFromBinaryType( BinaryType type ) {
	switch ( type ) {
		case BINARY_TYPE_EXE:				return "exe";
		case BINARY_TYPE_DYNAMIC_LIBRARY:	return "dll";
		case BINARY_TYPE_STATIC_LIBRARY:	return "lib";
	}

	assertf( false, "Something went really wrong here.\n" );

	return "ERROR";
}

bool8 FileIsSourceFile( const char* filename ) {
	static const char* fileExtensions[] = {
		".cpp",
		".cxx",
		".cc",
		".c",
	};

	For ( u64, extensionIndex, 0, count_of( fileExtensions ) ) {
		if ( string_ends_with( filename, fileExtensions[extensionIndex] ) ) {
			return true;
		}
	}

	return false;
}

bool8 FileIsHeaderFile( const char* filename ) {
	static const char* fileExtensions[] = {
		".h",
		".hpp",
	};

	For ( u64, extensionIndex, 0, count_of( fileExtensions ) ) {
		if ( string_ends_with( filename, fileExtensions[extensionIndex] ) ) {
			return true;
		}
	}

	return false;
}

static const char* BuildConfig_ToString( const BuildConfig* config ) {
	auto BinaryTypeToString = []( const BinaryType type ) -> const char* {
		switch ( type ) {
			case BINARY_TYPE_EXE:				return "BINARY_TYPE_EXE";
			case BINARY_TYPE_DYNAMIC_LIBRARY:	return "BINARY_TYPE_DYNAMIC_LIBRARY";
			case BINARY_TYPE_STATIC_LIBRARY:	return "BINARY_TYPE_STATIC_LIBRARY";
		}
	};

	auto OptimizationLevelToString = []( OptimizationLevel level ) -> const char* {
		switch ( level ) {
			case OPTIMIZATION_LEVEL_O0: return "OPTIMIZATION_LEVEL_00";
			case OPTIMIZATION_LEVEL_O1: return "OPTIMIZATION_LEVEL_01";
			case OPTIMIZATION_LEVEL_O2: return "OPTIMIZATION_LEVEL_02";
			case OPTIMIZATION_LEVEL_O3: return "OPTIMIZATION_LEVEL_03";
		}
	};

	StringBuilder builder = {};
	string_builder_reset( &builder );

	auto PrintCStringArray = [&builder]( const char* name, const std::vector<const char*>& array ) {
		string_builder_appendf( &builder, "\t%s: { ", name );
		For( u64, i, 0, array.size() ) {
			string_builder_appendf( &builder, "%s", array[i] );

			if ( i < array.size() - 1 ) {
				string_builder_appendf( &builder, ", " );
			}
		}
		string_builder_appendf( &builder, " }\n" );
	};

	auto PrintSTDStringArray = [&builder]( const char* name, const std::vector<std::string>& array ) {
		string_builder_appendf( &builder, "\t%s: { ", name );
		For( u64, i, 0, array.size() ) {
			string_builder_appendf( &builder, "%s", array[i].c_str() );

			if ( i < array.size() - 1 ) {
				string_builder_appendf( &builder, ", " );
			}
		}
		string_builder_appendf( &builder, " }\n" );
	};

	auto PrintField = [&builder]( const char* key, const char* value ) {
		string_builder_appendf( &builder, "\t%s: %s\n", key, value );
	};

	string_builder_appendf( &builder, "%s: {\n", config->name.c_str() );

	if ( config->depends_on.size() > 0 ) {
		string_builder_appendf( &builder, "\tdepends_on: { " );
		For ( u64, dependencyIndex, 0, config->depends_on.size() ) {
			string_builder_appendf( &builder, "%s", config->depends_on[dependencyIndex].name.c_str() );

			if ( dependencyIndex < config->depends_on.size() - 1 ) {
				string_builder_appendf( &builder, ", " );
			}
		}
		string_builder_appendf( &builder, " }\n" );
	}

	PrintSTDStringArray( "source_files", config->source_files );
	PrintSTDStringArray( "defines", config->defines );
	PrintSTDStringArray( "additional_includes", config->additional_includes );
	PrintSTDStringArray( "additional_lib_paths", config->additional_lib_paths );
	PrintSTDStringArray( "additional_libs", config->additional_libs );
	PrintSTDStringArray( "ignore_warnings", config->ignore_warnings );

	PrintField( "binary_name", config->binary_name.c_str() );
	PrintField( "binary_folder", config->binary_folder.c_str() );
	PrintField( "binary_type", BinaryTypeToString( config->binary_type ) );
	PrintField( "optimization_level", OptimizationLevelToString( config->optimization_level ) );
	PrintField( "remove_symbols", config->remove_symbols ? "true" : "false" );
	PrintField( "remove_file_extension", config->remove_file_extension ? "true" : "false" );
	PrintField( "warnings_as_errors", config->warnings_as_errors ? "true" : "false" );

	string_builder_appendf( &builder, "}\n" );

	return string_builder_to_string( &builder );
}

const char* BuildConfig_GetFullBinaryName( const BuildConfig* config ) {
	assert( !config->binary_name.empty() );

	StringBuilder builder = {};
	string_builder_reset( &builder );

	if ( !config->binary_folder.empty() ) {
		string_builder_appendf( &builder, "%s%c", config->binary_folder.c_str(), PATH_SEPARATOR );
	}

	string_builder_appendf( &builder, "%s", config->binary_name.c_str() );

	if ( !config->remove_file_extension ) {
		string_builder_appendf( &builder, ".%s", GetFileExtensionFromBinaryType( config->binary_type ) );
	}

	return string_builder_to_string( &builder );
}

s32 RunProc( Array<const char*>* args, Array<const char*>* environmentVariables, const procFlags_t procFlags ) {
	assert( args );
	assert( args->data );
	assert( args->count >= 1 );

	if ( procFlags & PROC_FLAG_SHOW_ARGS ) {
		For ( u64, argIndex, 0, args->count ) {
			printf( "%s ", ( *args )[argIndex] );
		}
		printf( "\n" );
	}

	Process* process = process_create( args, environmentVariables, PROCESS_FLAG_ASYNC | PROCESS_FLAG_COMBINE_STDOUT_AND_STDERR );

	if ( !process ) {
		error(
			"Failed to run process \"%s\".\n"
			"Is it definitely installed? Is it meant to be added to your PATH? Did you type the path correctly?\n"
			, ( *args )[0]
		);

		// DM: 20/07/2025: I'm not 100% sure that its totally ok to have -1 as our own special exit code to mean that the process couldnt be found
		// its totally possible for other processes to return -1 and have it mean something else
		// the interpretation of the exit code of the processes we run is the responsibility of the calling code and were probably making a lot of assumptions there
		return -1;
	}

	// show stdout
	if ( procFlags & PROC_FLAG_SHOW_STDOUT ) {
		char buffer[1024] = { 0 };
		u64 bytesRead = U64_MAX;

		while ( ( bytesRead = process_read_stdout( process, buffer, 1024 ) ) ) {
			buffer[bytesRead] = 0;

			printf( "%s", buffer );
		}
	}

	s32 exitCode = process_join( process );

	process_destroy( process );
	process = NULL;

	return exitCode;
}

static s32 ShowUsage( const s32 exitCode ) {
	printf(
		"Builder.exe\n"
		"\n"
		"USAGE:\n"
		"    Builder.exe <file> [arguments]\n"
		"\n"
		"Arguments:\n"
		"    " ARG_HELP_SHORT "|" ARG_HELP_LONG " (optional):\n"
		"        Shows this help and then exits.\n"
		"\n"
		"    " ARG_VERBOSE_SHORT "|" ARG_VERBOSE_LONG " (optional):\n"
		"        Enables verbose logging, so more detailed information gets output when doing a build.\n"
		"\n"
		"    <file> (required):\n"
		"        The file you want to build with.  There can only be one.\n"
#if USE_BUILD_INFO_FILES
		"        This file can either be a C++ code file (which is recommended) or a \"" BUILD_INFO_FILE_EXTENSION "\" file.\n"
#else
		"        This file must be a C++ code file.\n"
#endif
		"\n"
		"    " ARG_CONFIG "<config> (optional):\n"
		"        Sets the config to whatever you specify.\n"
		"        This must match the name of a config that you set inside \"" SET_BUILDER_OPTIONS_FUNC_NAME "\".\n"
		"\n"
		"    " ARG_NUKE " <folder> (optional):\n"
		"        Deletes every file in <folder> and all subfolders, but does not delete <folder>.\n"
		"\n"
		"    " ARG_VISUAL_STUDIO_BUILD " (optional):\n"
		"        Specifies that the build is being done from Visual Studio.\n"
		"        So even if BuilderOptions::generate_solution is set to true in the build settings source file we shouldn't generate Visual Studio project files and instead should just do a build using the specified config.\n"
		"\n"
	);

	return exitCode;
}

static buildResult_t BuildBinary( BuildConfig* config, compilerBackend_t* compilerBackend, std::vector<std::vector<std::string>>& outIncludeDependencies ) {
	// create binary folder
	if ( !folder_create_if_it_doesnt_exist( config->binary_folder.c_str() ) ) {
		errorCode_t errorCode = GetLastErrorCode();
		fatal_error( "Failed to create the binary folder you specified inside %s: \"%s\".  Error code: " ERROR_CODE_FORMAT "\n", SET_BUILDER_OPTIONS_FUNC_NAME, config->binary_folder.c_str(), errorCode );
		return BUILD_RESULT_FAILED;
	}

	// create intermediate folder
	const char* intermediatePath = tprintf( "%s%c%s", config->binary_folder.c_str(), PATH_SEPARATOR, INTERMEDIATE_PATH );
	if ( !folder_create_if_it_doesnt_exist( intermediatePath ) ) {
		errorCode_t errorCode = GetLastErrorCode();
		fatal_error( "Failed to create intermediate binary folder.  Error code: " ERROR_CODE_FORMAT "\n", errorCode );
		return BUILD_RESULT_FAILED;
	}

	s32 exitCode = 0;

	Array<const char*> intermediateFiles;
	intermediateFiles.reserve( config->source_files.size() );

	u32 numCompiledFiles = 0;

	// compile
	// make .o files for all compilation units
	// TODO(DM): 14/06/2025: embarrassingly parallel
	For ( u64, sourceFileIndex, 0, config->source_files.size() ) {
		const char* sourceFile = config->source_files[sourceFileIndex].c_str();
		const char* sourceFileNoPath = path_remove_path_from_file( sourceFile );

		const char* intermediateFilename = tprintf( "%s%c%s.o", intermediatePath, PATH_SEPARATOR, sourceFileNoPath );
		intermediateFiles.add( intermediateFilename );

		const char* depFilename = tprintf( "%s%c%s.d", intermediatePath, PATH_SEPARATOR, sourceFileNoPath );

		// only rebuild the .o file if the source file (or any of the files that source file depends on) was written to more recently or it doesnt exist
		{
			bool8 anyFileIsNewer = false;

			FileInfo intermediateFileInfo;
			File intermediateFile = file_find_first( intermediateFilename, &intermediateFileInfo );

			// if the .o file doesnt exist then assume we havent built this file yet
			// if the .o file does exist but the source file was written to it more recently then we know we want to rebuild
			if ( ( intermediateFile.ptr == INVALID_HANDLE_VALUE ) || ( GetLastFileWriteTime( sourceFile ) > intermediateFileInfo.last_write_time ) ) {
				anyFileIsNewer = true;
			}

			// if the source file wasnt newer than the .o file then do the same timestamp check for all the files that this source file depends on
			// just because the source file didnt change doesnt mean we dont want to recompile it
			// what if one of the header files it relies on changed? we still want to recompile that file!
			{
				std::vector<std::string>& includeDependencies = outIncludeDependencies[sourceFileIndex];

				For ( u64, dependencyIndex, 0, includeDependencies.size() ) {
					u64 dependencyLastWriteTime = GetLastFileWriteTime( includeDependencies[dependencyIndex].c_str() );

					if ( dependencyLastWriteTime > intermediateFileInfo.last_write_time ) {
						anyFileIsNewer = true;
						break;
					}
				}
			}

			// the .o file exists and none of the files it relies on have been changed more recently
			// no need to rebuild
			if ( !anyFileIsNewer ) {
				continue;
			}
		}

		if ( !compilerBackend->CompileSourceFile( compilerBackend, sourceFile, config ) ) {
			error( "Compile failed.\n" );
			return BUILD_RESULT_FAILED;
		}

		numCompiledFiles += 1;

		compilerBackend->GetIncludeDependenciesFromSourceFileBuild( compilerBackend, outIncludeDependencies[sourceFileIndex] );
	}

	// if the binary doesnt exist or if any of the intermediate files are newer than the binary then we want to link
	// otherwise we can skip it
	{
		bool8 doLinking = false;

		const char* fullBinaryName = BuildConfig_GetFullBinaryName( config );

		FileInfo binaryFileInfo = {};
		File binaryFile = file_find_first( fullBinaryName, &binaryFileInfo );

		if ( binaryFile.ptr == INVALID_HANDLE_VALUE ) {
			doLinking = true;
		} else {
			For ( u64, intermediateFileIndex, 0, intermediateFiles.count ) {
				u64 intermediateFileLastWriteTime = GetLastFileWriteTime( intermediateFiles[intermediateFileIndex] );

				if ( intermediateFileLastWriteTime > binaryFileInfo.last_write_time ) {
					doLinking = true;
					break;
				}
			}
		}

		if ( !doLinking ) {
			return BUILD_RESULT_SKIPPED;
		}
	}

	if ( !compilerBackend->LinkIntermediateFiles( compilerBackend, intermediateFiles, config ) ) {
		error( "Linking failed.\n" );
		return BUILD_RESULT_FAILED;
	}

	return BUILD_RESULT_SUCCESS;
}

static bool8 FileExists( const char* filename ) {
	FileInfo fileInfo = {};
	File file = file_find_first( filename, &fileInfo );

	return file.ptr != INVALID_HANDLE_VALUE;
}

static void NukeFolderInternal_r( const char* folder, const bool8 verbose ) {
	const char* searchPattern = tprintf( "%s%c*", folder, PATH_SEPARATOR );

	FileInfo fileInfo = {};
	File file = file_find_first( searchPattern, &fileInfo );

	do {
		if ( string_equals( fileInfo.filename, "." ) || string_equals( fileInfo.filename, ".." ) || fileInfo.filename[0] == 0 ) {
			continue;
		}

		const char* fileFullPath = tprintf( "%s%c%s", folder, PATH_SEPARATOR, fileInfo.filename );

		if ( fileInfo.is_directory ) {
			if ( verbose ) {
				printf( "Found folder %s\n", fileFullPath );
			}

			NukeFolderInternal_r( fileFullPath, verbose );

			if ( verbose ) {
				printf( "Deleting folder %s\n", fileFullPath );
			}

			if ( !folder_delete( fileFullPath ) ) {
				error( "Nuke failed to delete folder \"%s\".\n", fileFullPath );
			}
		} else {
			if ( verbose ) {
				printf( "Deleting file %s\n", fileFullPath );
			}

			if ( !file_delete( fileFullPath ) ) {
				error( "Nuke failed to delete folder \"%s\".\n", fileFullPath );
			}
		}
	} while ( file_find_next( &file, &fileInfo ) );
}

void NukeFolder_r( const char* folder, const bool8 deleteRoot, const bool8 verbose ) {
	NukeFolderInternal_r( folder, verbose );

	if ( deleteRoot ) {
		if ( !folder_delete( folder ) ) {
			errorCode_t errorCode = GetLastErrorCode();
			error( "Nuke failed to delete root folder \"%s\".  You'll have to do this manually.  Error code " ERROR_CODE_FORMAT "\n", folder, errorCode );
		}
	}
}

// TODO(DM): redo this so that we get the next forward and backslash, then check which one comes first, and return that
// if neither then return NULL
const char* GetSlashInPath( const char* path ) {
	const char* slash = NULL;

	if ( !slash ) slash = strchr( path, '/' );
	if ( !slash ) slash = strchr( path, '\\' );

	return slash;
}

static void GetAllSourceFiles_r( const char* path, const char* subfolder, const String& searchFilter, std::vector<std::string>& outSourceFiles ) {
	assert( path );
	assert( searchFilter.data );

	// TODO(DM): need a better way of finding the appropriate slash type here between platforms
	const char* start = searchFilter.data;
	const char* end = NULL;
	if ( !end ) end = strchr( start, '/' );
	if ( !end ) end = strchr( start, '\\' );
	if ( end ) {
		u64 subPathLength = cast( u64, end ) - cast( u64, start );
		String subPath;
		string_copy_from_c_string( &subPath, start, subPathLength );

		if ( string_equals( subPath.data, "**" ) ) {
			//printf( "Doing recursive file search\n" );

			// + 1 to skip the slash as well
			String searchFilterCopy = searchFilter;
			searchFilterCopy.data += subPathLength + 1;
			searchFilterCopy.count -= subPathLength + 1;

			//printf( "'path' is now: %s\n", newPath.data );
			//printf( "'searchFilter' is now: %s\n", searchFilter.data );
			//printf( "\n" );

			const char* fullSearchPath = NULL;
			if ( subfolder ) {
				fullSearchPath = tprintf( "%s%c%s%c*", path, PATH_SEPARATOR, subfolder, PATH_SEPARATOR );
			} else {
				fullSearchPath = tprintf( "%s%c*", path, PATH_SEPARATOR );
			}

			FileInfo fileInfo = {};
			File file = file_find_first( fullSearchPath, &fileInfo );

			do {
				if ( string_equals( fileInfo.filename, "." ) || string_equals( fileInfo.filename, ".." ) ) {
					continue;
				}

				if ( !fileInfo.is_directory ) {
					continue;
				}

				const char* newSubfolder = NULL;
				if ( subfolder ) {
					newSubfolder = tprintf( "%s%c%s", subfolder, PATH_SEPARATOR, fileInfo.filename );
				} else {
					newSubfolder = tprintf( "%s", fileInfo.filename );
				}

				GetAllSourceFiles_r( path, newSubfolder, searchFilterCopy, outSourceFiles );
			} while ( file_find_next( &file, &fileInfo ) );
		} else if ( string_equals( subPath.data, "*" ) ) {
			//printf( "Doing non-recursive file search\n" );

			const char* fullSearchPath = NULL;
			if ( subfolder ) {
				fullSearchPath = tprintf( "%s%c%s%c%s", path, PATH_SEPARATOR, subfolder, PATH_SEPARATOR, subPath.data );
			} else {
				fullSearchPath = subPath.data;
			}

			FileInfo fileInfo = {};
			File file = file_find_first( fullSearchPath, &fileInfo );

			do {
				if ( string_equals( fileInfo.filename, "." ) || string_equals( fileInfo.filename, ".." ) ) {
					continue;
				}

				if ( fileInfo.is_directory ) {
					continue;
				}

				const char* foundFilename = NULL;
				if ( subfolder ) {
					foundFilename = tprintf( "%s%c%s", subfolder, PATH_SEPARATOR, fileInfo.filename );
				} else {
					foundFilename = tprintf( "%s", fileInfo.filename );
				}

				//printf( "FOUND FILE \"%s\"\n", foundFilename );

				outSourceFiles.push_back( foundFilename );
			} while ( file_find_next( &file, &fileInfo ) );
		} else {
			// + 1 to skip the slash as well
			String searchFilterCopy = searchFilter;
			searchFilterCopy.data += subPathLength + 1;
			searchFilterCopy.count -= subPathLength + 1;

			//printf( "'path' is now: %s\n", newPath.data );
			//printf( "'searchFilter' is now: %s\n", searchFilter.data );
			//printf( "\n" );

			const char* newSubfolder = NULL;
			if ( subfolder ) {
				newSubfolder = tprintf( "%s%c%s", subfolder, PATH_SEPARATOR, subPath.data );
			} else {
				newSubfolder = tprintf( "%s", subPath.data );
			}

			GetAllSourceFiles_r( path, newSubfolder, searchFilterCopy, outSourceFiles );
		}
	} else {
		const char* fullSearchPath = NULL;
		if ( subfolder ) {
			fullSearchPath = tprintf( "%s%c%s%c%s", path, PATH_SEPARATOR, subfolder, PATH_SEPARATOR, searchFilter.data );
		} else {
			fullSearchPath = tprintf( "%s%c%s", path, PATH_SEPARATOR, searchFilter.data );
		}

		FileInfo fileInfo = {};
		File file = file_find_first( fullSearchPath, &fileInfo );

		if ( file.ptr != INVALID_HANDLE_VALUE ) {
			do {
				if ( string_equals( fileInfo.filename, "." ) || string_equals( fileInfo.filename, ".." ) ) {
					continue;
				}

				if ( fileInfo.is_directory ) {
					continue;
				}

				const char* foundFilename = NULL;
				if ( subfolder ) {
					foundFilename = tprintf( "%s%c%s", subfolder, PATH_SEPARATOR, fileInfo.filename );
				} else {
					foundFilename = tprintf( "%s", fileInfo.filename );
				}

				//printf( "FOUND FILE \"%s\"\n", foundFilename );

				outSourceFiles.push_back( foundFilename );
			} while ( file_find_next( &file, &fileInfo ) );
		}
	}
}

static std::vector<std::string> BuildConfig_GetAllSourceFiles( const buildContext_t* context, const BuildConfig* config ) {
	std::vector<std::string> sourceFiles;

	For ( u64, sourceFileIndex, 0, config->source_files.size() ) {
		const char* sourceFile = config->source_files[sourceFileIndex].c_str();

		bool8 inputFileIsSameAsSourceFile = string_equals( sourceFile, context->inputFile );

		const char* sourceFileNoPath = path_remove_path_from_file( sourceFile );
		const char* sourceFilePath = NULL;

		if ( inputFileIsSameAsSourceFile ) {
			GetAllSourceFiles_r( context->inputFilePath.data, NULL, sourceFileNoPath, sourceFiles );
		} else {
			sourceFilePath = path_remove_file_from_path( sourceFile );

			if ( !sourceFilePath ) {
				sourceFilePath = ".";
			}

			GetAllSourceFiles_r( context->inputFilePath.data, NULL, sourceFile, sourceFiles );
		}
	}

	return sourceFiles;
}

#if USE_BUILD_INFO_FILES
struct byteBuffer_t {
	Array<u8>	data;
	u64			readOffset;
};

static void ByteBuffer_SkipReadTo( byteBuffer_t* buffer, const char c ) {
	while ( buffer->data[buffer->readOffset] != c ) {
		buffer->readOffset++;
	}
}

static void ByteBuffer_SkipReadPast( byteBuffer_t* buffer, const char c ) {
	ByteBuffer_SkipReadTo( buffer, c );
	buffer->readOffset++;
}

static bool8 ByteBuffer_Read_Bool8( byteBuffer_t* buffer ) {
	bool8* result = cast( bool8*, &buffer->data[buffer->readOffset] );

	buffer->readOffset += sizeof( bool8 );

	return *result;
}

static void ByteBuffer_Write_Bool8( byteBuffer_t* buffer, const bool8 x ) {
	buffer->data.add( x );
}

static s32 ByteBuffer_Read_S32( byteBuffer_t* buffer ) {
	s32* result = cast( s32*, &buffer->data[buffer->readOffset] );

	buffer->readOffset += sizeof( s32 );

	return *result;
}

static void ByteBuffer_Write_S32( byteBuffer_t* buffer, const s32 x ) {
	buffer->data.reserve( buffer->data.alloced + sizeof( s32 ) );

	buffer->data.add( ( x ) & 0xFF );
	buffer->data.add( ( x >> 8 ) & 0xFF );
	buffer->data.add( ( x >> 16 ) & 0xFF );
	buffer->data.add( ( x >> 24 ) & 0xFF );
}

static u64 ByteBuffer_Read_U64( byteBuffer_t* buffer ) {
	u64* result = cast( u64*, &buffer->data[buffer->readOffset] );

	buffer->readOffset += sizeof( u64 );

	return *result;
}

static void ByteBuffer_Write_U64( byteBuffer_t* buffer, const u64 x ) {
	buffer->data.reserve( buffer->data.alloced + sizeof( u64 ) );

	buffer->data.add( ( x ) & 0xFF );
	buffer->data.add( ( x >> 8 ) & 0xFF );
	buffer->data.add( ( x >> 16 ) & 0xFF );
	buffer->data.add( ( x >> 24 ) & 0xFF );
	buffer->data.add( ( x >> 32 ) & 0xFF );
	buffer->data.add( ( x >> 40 ) & 0xFF );
	buffer->data.add( ( x >> 48 ) & 0xFF );
	buffer->data.add( ( x >> 56 ) & 0xFF );
}

static char* ByteBuffer_Read_CString( byteBuffer_t* buffer ) {
	u8* bufferPos = &buffer->data[buffer->readOffset];

	const char* lineEnd = cast( const char*, memchr( bufferPos, '\n', buffer->data.count - buffer->readOffset ) );
	u64 stringLength = cast( u64, lineEnd ) - cast( u64, bufferPos );

	char* string = cast( char*, mem_alloc( ( stringLength + 1 ) * sizeof( char ) ) );	// + 1 for null terminator
	memcpy( string, bufferPos, stringLength * sizeof( char ) );
	string[stringLength] = 0;

	ByteBuffer_SkipReadPast( buffer, '\n' );

	return string;
}

static void ByteBuffer_Write_CString( byteBuffer_t* buffer, const char* str ) {
	assert( str );

	buffer->data.add_range( cast( const u8*, str ), strlen( str ) );
	buffer->data.add_range( cast( const u8*, "\n" ), strlen( "\n" ) );
}

static std::vector<const char*> ByteBuffer_Read_CStringArray( byteBuffer_t* buffer ) {
	u64 arrayCount = ByteBuffer_Read_U64( buffer );
	std::vector<const char*> result( arrayCount );

	For ( u64, i, 0, result.size() ) {
		result[i] = ByteBuffer_Read_CString( buffer );
	}

	return result;
}

static std::vector<std::string> ByteBuffer_Read_STDStringArray( byteBuffer_t* buffer ) {
	u64 arrayCount = ByteBuffer_Read_U64( buffer );
	std::vector<std::string> result( arrayCount );

	For ( u64, i, 0, result.size() ) {
		result[i] = ByteBuffer_Read_CString( buffer );
	}

	return result;
}

static void ByteBuffer_Write_STDStringArray( byteBuffer_t* buffer, const std::vector<std::string>& array ) {
	ByteBuffer_Write_U64( buffer, array.size() );

	For ( u64, i, 0, array.size() ) {
		ByteBuffer_Write_CString( buffer, array[i].c_str() );
	}
}

static bool8 BuildInfo_Read( const char* buildInfoFilename, buildInfoData_t* outBuildInfoData ) {
	byteBuffer_t buffer = {};

	// do this to avoid crash in ~Array() because it expects the allocator to not be NULL
	// we are not using the members of buffer::data (Array<T>) in a "standard" way here
	buffer.data.allocator = mem_get_current_allocator();

	bool8 read = file_read_entire( buildInfoFilename, cast( char**, &buffer.data.data ), &buffer.data.count );

	// the first time you build a source file with builder there wont be a .build_info file
	if ( !read ) {
		return false;
	}

	outBuildInfoData->builderVersion.major = ByteBuffer_Read_S32( &buffer );
	outBuildInfoData->builderVersion.minor = ByteBuffer_Read_S32( &buffer );
	outBuildInfoData->builderVersion.patch = ByteBuffer_Read_S32( &buffer );

	outBuildInfoData->userConfigSourceFilename = ByteBuffer_Read_CString( &buffer );
	outBuildInfoData->userConfigDLLFilename = ByteBuffer_Read_CString( &buffer );

	// parse all BuilderOptions
	{
		u64 totalNumConfigs = ByteBuffer_Read_U64( &buffer );

		outBuildInfoData->configs.resize( totalNumConfigs );
		outBuildInfoData->includeDependencies.resize( totalNumConfigs );

		For ( u64, configIndex, 0, outBuildInfoData->configs.size() ) {
			// parse the config itself
			{
				BuildConfig* config = &outBuildInfoData->configs[configIndex];

				config->name = ByteBuffer_Read_CString( &buffer );
				ByteBuffer_Read_U64( &buffer );	// name hash, skip

				{
					u64 numDependsOn = ByteBuffer_Read_U64( &buffer );

					config->depends_on.resize( numDependsOn );

					For ( u64, dependencyIndex, 0, numDependsOn ) {
						config->depends_on[dependencyIndex].name = ByteBuffer_Read_CString( &buffer );
					}
				}

				config->source_files = ByteBuffer_Read_STDStringArray( &buffer );
				config->defines = ByteBuffer_Read_STDStringArray( &buffer );
				config->additional_includes = ByteBuffer_Read_STDStringArray( &buffer );
				config->additional_lib_paths = ByteBuffer_Read_STDStringArray( &buffer );
				config->additional_libs = ByteBuffer_Read_STDStringArray( &buffer );
				config->ignore_warnings = ByteBuffer_Read_STDStringArray( &buffer );

				config->binary_name = ByteBuffer_Read_CString( &buffer );
				config->binary_folder = ByteBuffer_Read_CString( &buffer );

				config->binary_type = cast( BinaryType, ByteBuffer_Read_S32( &buffer ) );
				config->optimization_level = cast( OptimizationLevel, ByteBuffer_Read_S32( &buffer ) );
				config->remove_symbols = ByteBuffer_Read_Bool8( &buffer );
				config->remove_file_extension = ByteBuffer_Read_Bool8( &buffer );
				config->warnings_as_errors = ByteBuffer_Read_Bool8( &buffer );
			}

			// parse all compilation units
			{
				u64 numConfigIncludeDependencies = ByteBuffer_Read_U64( &buffer );

				outBuildInfoData->includeDependencies[configIndex].resize( numConfigIncludeDependencies );

				For ( u64, sourceFileIndex, 0, numConfigIncludeDependencies ) {
					u64 numIncludeDependencies = ByteBuffer_Read_U64( &buffer );

					outBuildInfoData->includeDependencies[configIndex][sourceFileIndex].resize( numIncludeDependencies );

					For ( u64, includeDependencyIndex, 0, numIncludeDependencies ) {
						outBuildInfoData->includeDependencies[configIndex][sourceFileIndex][includeDependencyIndex] = ByteBuffer_Read_CString( &buffer );
					}
				}
			}
		}
	}

	// reconstruct all build dependencies after we have deserialized them all from the file
	For ( u64, configIndex, 0, outBuildInfoData->configs.size() ) {
		BuildConfig* config = &outBuildInfoData->configs[configIndex];

		For ( u64, dependencyIndex, 0, config->depends_on.size() ) {
			BuildConfig* dependency = &config->depends_on[dependencyIndex];

			bool8 found = false;

			For ( u64, otherConfigIndex, 0, outBuildInfoData->configs.size() ) {
				BuildConfig* otherConfig = &outBuildInfoData->configs[otherConfigIndex];

				if ( otherConfig->name == dependency->name ) {
					config->depends_on[dependencyIndex] = *otherConfig;

					found = true;
					break;
				}
			}

			if ( !found ) {
				error( "Failed to find the build dependency \"%s\".\n", dependency->name.c_str() );	// TODO(DM): 26/11/2024: better error msg here!
				return false;
			}
		}
	}

	return true;
}

static bool8 BuildInfo_Write( const buildContext_t* context, const buildInfoData_t* buildInfoData ) {
	assert( context );
	assert( buildInfoData );
	assert( !buildInfoData->configs.empty() );
	assert( !buildInfoData->userConfigSourceFilename.empty() );
	assert( !buildInfoData->userConfigDLLFilename.empty() );

	printf( "Writing \"%s\"...\n", context->buildInfoFilename.data );

	byteBuffer_t buffer = {};

	ByteBuffer_Write_S32( &buffer, BUILDER_VERSION_MAJOR );
	ByteBuffer_Write_S32( &buffer, BUILDER_VERSION_MINOR );
	ByteBuffer_Write_S32( &buffer, BUILDER_VERSION_PATCH );

	ByteBuffer_Write_CString( &buffer, buildInfoData->userConfigSourceFilename.c_str() );
	ByteBuffer_Write_CString( &buffer, buildInfoData->userConfigDLLFilename.c_str() );

	ByteBuffer_Write_U64( &buffer, buildInfoData->configs.size() );

	For ( u64, configIndex, 0, buildInfoData->configs.size() ) {
		const BuildConfig* config = &buildInfoData->configs[configIndex];

		ByteBuffer_Write_CString( &buffer, config->name.c_str() );

		u64 configNameHash = hash_string( config->name.c_str(), 0 );
		ByteBuffer_Write_U64( &buffer, configNameHash );

		// serialize names of all build dependencies
		{
			ByteBuffer_Write_U64( &buffer, config->depends_on.size() );

			For ( u64, dependencyIndex, 0, config->depends_on.size() ) {
				ByteBuffer_Write_CString( &buffer, config->depends_on[dependencyIndex].name.c_str() );
			}
		}

		ByteBuffer_Write_STDStringArray( &buffer, config->source_files );
		ByteBuffer_Write_STDStringArray( &buffer, config->defines );
		ByteBuffer_Write_STDStringArray( &buffer, config->additional_includes );
		ByteBuffer_Write_STDStringArray( &buffer, config->additional_lib_paths );
		ByteBuffer_Write_STDStringArray( &buffer, config->additional_libs );
		ByteBuffer_Write_STDStringArray( &buffer, config->ignore_warnings );

		ByteBuffer_Write_CString( &buffer, config->binary_name.c_str() );
		ByteBuffer_Write_CString( &buffer, config->binary_folder.c_str() );

		ByteBuffer_Write_S32( &buffer, config->binary_type );
		ByteBuffer_Write_S32( &buffer, config->optimization_level );
		ByteBuffer_Write_Bool8( &buffer, config->remove_symbols );
		ByteBuffer_Write_Bool8( &buffer, config->remove_file_extension );
		ByteBuffer_Write_Bool8( &buffer, config->warnings_as_errors );

		// serialize all compilation units
		// for each compilation unit, serialize all the included files it depends on and when their last write time
		{
			std::vector<std::vector<std::string>> configIncludeDependencies = buildInfoData->includeDependencies[configIndex];

			ByteBuffer_Write_U64( &buffer, configIncludeDependencies.size() );

			For ( u64, sourceFileIndex, 0, configIncludeDependencies.size() ) {
				std::vector<std::string> includeDependencies = configIncludeDependencies[sourceFileIndex];

				ByteBuffer_Write_U64( &buffer, includeDependencies.size() );

				For ( u64, includeDepdendencyIndex, 0, includeDependencies.size() ) {
					ByteBuffer_Write_CString( &buffer, includeDependencies[includeDepdendencyIndex].c_str() );
				}
			}
		}

		mem_reset_temp_storage();
	}

	// write to the file
	if ( !file_write_entire( context->buildInfoFilename.data, buffer.data.data, buffer.data.count ) ) {
		error( "Failed to write %s!\n", context->buildInfoFilename.data );
		return false;
	}

	printf( "\n" );

	return true;
}
#endif // USE_BUILD_INFO_FILES

static void AddBuildConfigAndDependenciesUnique( buildContext_t* context, const BuildConfig* config, std::vector<BuildConfig>& outConfigs ) {
	u64 configNameHash = hash_string( config->name.c_str(), 0 );

	if ( hashmap_get_value( context->configIndices, configNameHash ) == HASHMAP_INVALID_VALUE ) {
		// add other configs that this config depends on first
		For ( size_t, dependencyIndex, 0, config->depends_on.size() ) {
			AddBuildConfigAndDependenciesUnique( context, &config->depends_on[dependencyIndex], outConfigs );
		}

		outConfigs.push_back( *config );

		hashmap_set_value( context->configIndices, configNameHash, trunc_cast( u32, outConfigs.size() - 1 ) );
	}
}

int main( int argc, char** argv ) {
	float64 totalTimeStart = time_ms();

	float64 userConfigBuildTimeMS = -1.0;
	float64 setBuilderOptionsTimeMS = -1.0;
	float64 visualStudioGenerationTimeMS = -1.0;
	float64 buildInfoReadTimeMS = -1.0;
	float64 buildInfoWriteTimeMS = -1.0;

	core_init( MEM_MEGABYTES( 128 ) );	// TODO(DM): 26/03/2025: can we just use defaults for this now?
	defer( core_shutdown() );

	printf( "Builder v%d.%d.%d RC0\n\n", BUILDER_VERSION_MAJOR, BUILDER_VERSION_MINOR, BUILDER_VERSION_PATCH );

	buildContext_t context = {
		.configIndices	= hashmap_create( 1 ),	// TODO(DM): 30/03/2025: whats a reasonable default here?
#ifdef _DEBUG
		.verbose		= true,
#else
		.verbose		= false,
#endif
	};

	context.includeDependencies.resize( 1 );

	// parse command line args
	const char* inputConfigName = NULL;
	u64 inputConfigNameHash = 0;

	bool8 isVisualStudioBuild = false;

	For ( s32, argIndex, 1, argc ) {
		const char* arg = argv[argIndex];
		const u64 argLen = strlen( arg );

		if ( string_equals( arg, ARG_HELP_SHORT ) || string_equals( arg, ARG_HELP_LONG ) ) {
			return ShowUsage( 0 );
		}

		if ( string_equals( arg, ARG_VERBOSE_SHORT ) || string_equals( arg, ARG_HELP_LONG ) ) {
			context.verbose = true;
			continue;
		}

		if ( FileIsSourceFile( arg ) ) {
			if ( context.inputFile != NULL ) {
				error( "You've already specified a file for me to build.  If you want me to build more than one source file, specify it via %s().\n", SET_BUILDER_OPTIONS_FUNC_NAME );
				QUIT_ERROR();
			}

			context.inputFile = arg;

			continue;
		}

		if ( string_starts_with( arg, ARG_CONFIG ) ) {
			const char* equals = strchr( arg, '=' );

			if ( !equals ) {
				error( "I detected that you want to set a config, but you never gave me the equals (=) immediately after it.  You need to do that.\n" );

				return ShowUsage( 1 );
			}

			const char* configName = equals + 1;

			if ( strlen( configName ) < 1 ) {
				error( "You specified the start of the config arg, but you never actually gave me a name for the config.  I need that.\n" );

				return ShowUsage( 1 );
			}

			inputConfigName = configName;
			inputConfigNameHash = hash_string( inputConfigName, 0 );

			continue;
		}

		if ( string_equals( arg, ARG_NUKE ) ) {
			if ( argIndex == argc - 1 ) {
				error( "You passed in " ARG_NUKE " but you never told me what folder you want me to nuke.  I need to know!" );
				QUIT_ERROR();
			}

			const char* folderToNuke = argv[argIndex + 1];

			printf( "Nuking \"%s\"\n", folderToNuke );

			float64 startTime = time_ms();

			NukeFolder_r( folderToNuke, false, true );

			float64 endTime = time_ms();

			printf( "Done.  %f ms\n", endTime - startTime );

			return 0;
		}

		if ( string_equals( arg, ARG_VISUAL_STUDIO_BUILD ) ) {
			isVisualStudioBuild = true;

			continue;
		}

		// unrecognised arg, show error
		error( "Unrecognised argument \"%s\".\n", arg );
		QUIT_ERROR();
	}

	// we need a source file specified at the command line
	// otherwise we dont know what to build!
	if ( context.inputFile == NULL ) {
		error(
			"You haven't told me what source files I need to build.  I need one.\n"
			"Run builder " ARG_HELP_LONG " if you need help.\n"
		);

		QUIT_ERROR();
	}

	// the default binary folder is the same folder as the source file
	// if the file doesnt have a path then assume its in the same path as the current working directory (where we are calling builder from)
	{
		const char* inputFilePath = path_remove_file_from_path( context.inputFile );

		if ( !inputFilePath ) {
			inputFilePath = path_current_working_directory();
		}

		const char* inputFileNoPath = path_remove_path_from_file( context.inputFile );
		const char* inputFileNoPathOrExtension = path_remove_file_extension( inputFileNoPath );

		context.inputFilePath = inputFilePath;

		string_printf( &context.dotBuilderFolder, "%s%c.builder", context.inputFilePath.data, PATH_SEPARATOR );
#if USE_BUILD_INFO_FILES
		string_printf( &context.buildInfoFilename, "%s%c%s%s", context.dotBuilderFolder.data, PATH_SEPARATOR, inputFileNoPathOrExtension, BUILD_INFO_FILE_EXTENSION );
#endif
	}

	const char* defaultBinaryName = path_remove_file_extension( path_remove_path_from_file( context.inputFile ) );

	// init default compiler backend (the version of clang that builder came with)
	compilerBackend_t compilerBackend;
	CreateCompilerBackend_Clang( &compilerBackend );
	compilerBackend.compilerPath = DEFAULT_COMPILER_PATH;

	compilerBackend.Init( &compilerBackend );
	defer( compilerBackend.Shutdown( &compilerBackend ) );

#if USE_BUILD_INFO_FILES
	// read .build_info file
	buildInfoData_t buildInfoData = {};
	bool8 readBuildInfo = false;
	{
		float64 start = time_ms();

		readBuildInfo = BuildInfo_Read( context.buildInfoFilename.data, &buildInfoData );

		float64 end = time_ms();

		buildInfoReadTimeMS = end - start;
	}

	buildInfoData.userConfigSourceFilename = context.inputFile;
#endif // USE_BUILD_INFO_FILES

	// user config build step
	// see if they have set_builder_options() overridden
	// if they do, then build a DLL first and call that function to set some more build options
	buildResult_t userConfigBuildResult = BUILD_RESULT_SKIPPED;
	const char* userConfigFullBinaryName = NULL;
	{
		float64 userConfigBuildTimeStart = time_ms();

		BuildConfig userConfigBuildConfig = {
			.source_files = {
#if USE_BUILD_INFO_FILES
				buildInfoData.userConfigSourceFilename,
#else
				context.inputFile,
#endif
			},
			.defines = {
				"_CRT_SECURE_NO_WARNINGS",
				"BUILDER_DOING_USER_CONFIG_BUILD",
#if defined( _DEBUG )
				"_DEBUG",
#else
				"NDEBUG",
#endif
			},
			.additional_includes = {
				path_app_path()	// add the folder that builder lives in as an additional include path otherwise people have no real way of being able to include it
			},
			.additional_libs = {
#if defined( _WIN64 )
				"user32.lib",
				// MSVCRT is needed for ABI compatibility between builder and the user config DLL on windows
#if defined( _DEBUG )
				"msvcrtd.lib",
#else
				"msvcrt.lib",
#endif
#endif
			},
			.ignore_warnings = {
				"-Wno-missing-prototypes",	// otherwise the user has to forward declare functions like set_builder_options and thats annoying
				"-Wno-reorder-init-list",	// allow users to initialize struct members in whatever order they want
			},
			.binary_name = defaultBinaryName,//path_remove_path_from_file( path_remove_file_extension( buildInfoData.userConfigSourceFilename.c_str() ) ),
			.binary_folder = context.dotBuilderFolder.data,
			.binary_type = BINARY_TYPE_DYNAMIC_LIBRARY,
			// this is needed because this tells the compiler what to set _ITERATOR_DEBUG_LEVEL to
			// ABI compatibility will be broken if this is not the same between all binaries
#if defined( _DEBUG )
			.optimization_level = OPTIMIZATION_LEVEL_O0,
#else
			.optimization_level = OPTIMIZATION_LEVEL_O3,
#endif
		};

		userConfigFullBinaryName = BuildConfig_GetFullBinaryName( &userConfigBuildConfig );
#if USE_BUILD_INFO_FILES
		buildInfoData.userConfigDLLFilename = userConfigFullBinaryName;
#endif

		userConfigBuildResult = BuildBinary( &userConfigBuildConfig, &compilerBackend, context.includeDependencies );

		switch ( userConfigBuildResult ) {
			case BUILD_RESULT_SUCCESS:
				printf( "\n" );
				break;

			case BUILD_RESULT_FAILED:
				error( "Pre-build failed!\n" );
				QUIT_ERROR();

			case BUILD_RESULT_SKIPPED:
				// nothing
				break;
		}

		float64 userConfigBuildTimeEnd = time_ms();

		userConfigBuildTimeMS = userConfigBuildTimeEnd - userConfigBuildTimeStart;
	}

	BuilderOptions options = {};

#if USE_BUILD_INFO_FILES
	Library library = library_load( buildInfoData.userConfigDLLFilename.c_str() );
#else
	Library library = library_load( userConfigFullBinaryName );
#endif
	defer( library_unload( &library ) );

	typedef void ( *setBuilderOptionsFunc_t )( BuilderOptions* options );
	typedef void ( *preBuildFunc_t )();
	typedef void ( *postBuildFunc_t )();

	preBuildFunc_t preBuildFunc = cast( preBuildFunc_t, library_get_proc_address( library, PRE_BUILD_FUNC_NAME ) );
	postBuildFunc_t postBuildFunc = cast( postBuildFunc_t, library_get_proc_address( library, POST_BUILD_FUNC_NAME ) );

	{
		// now get the user-specified options
		setBuilderOptionsFunc_t setBuilderOptionsFunc = cast( setBuilderOptionsFunc_t, library_get_proc_address( library, SET_BUILDER_OPTIONS_FUNC_NAME ) );
		if ( setBuilderOptionsFunc ) {
			float64 setBuilderOptionsTimeStart = time_ms();

			setBuilderOptionsFunc( &options );

			float64 setBuilderOptionsTimeEnd = time_ms();

			setBuilderOptionsTimeMS = setBuilderOptionsTimeEnd - setBuilderOptionsTimeStart;

#if USE_BUILD_INFO_FILES
			buildInfoData.configs = options.configs;

			buildInfoData.includeDependencies.resize( buildInfoData.configs.size() );
#endif // USE_BUILD_INFO_FILES

			// if the user wants to generate a visual studio solution then do that now
			if ( options.generate_solution && !isVisualStudioBuild ) {
				// you either want to generate a visual studio solution or build this config, but not both
				if ( inputConfigName ) {
					error(
						"I see you want to generate a Visual Studio Solution, but you've also specified a config that you want to build.\n"
						"You must do one or the other, you can't do both.\n\n"
					);

					QUIT_ERROR();
				}

				// make sure BuilderOptions::configs and configs from visual studio match
				// we will need this list later for validation
				options.configs.clear();
				For ( u64, projectIndex, 0, options.solution.projects.size() ) {
					VisualStudioProject* project = &options.solution.projects[projectIndex];

					For ( u64, configIndex, 0, project->configs.size() ) {
						VisualStudioConfig* config = &project->configs[configIndex];

						AddBuildConfigAndDependenciesUnique( &context, &config->options, options.configs );
					}
				}

				printf( "Generating Visual Studio files\n" );

				float64 start = time_ms();

				bool8 generated = GenerateVisualStudioSolution( &context, &options );

				float64 end = time_ms();

				visualStudioGenerationTimeMS = end - start;

				if ( !generated ) {
					error( "Failed to generate Visual Studio solution.\n" );	// TODO(DM): better error message
					QUIT_ERROR();
				}

				printf( "Done.\n" );

				return 0;
			}
		}
	}

	// if no configs were manually added then assume we are just doing a default build with no user-specified options
#if USE_BUILD_INFO_FILES
	if ( buildInfoData.configs.size() == 0 ) {
#else
	if ( options.configs.size() == 0 ) {
#endif
		BuildConfig config = {
			.source_files = { context.inputFile },
			.binary_name = defaultBinaryName
		};

#if USE_BUILD_INFO_FILES
		buildInfoData.configs.push_back( config );
#else
		options.configs.push_back( config );
#endif
	}

	float64 compilerBackendInitTimeMS = -1.0f;
	String compilerVersion;
	{
		// if the user never specified a compiler, we can build with the default compiler that we just built the user config DLL with
		if ( options.compiler_path.empty() ) {
			options.compiler_path = DEFAULT_COMPILER_PATH;
		} else {
			options.compiler_path = path_remove_file_extension( options.compiler_path.c_str() );

			compilerBackend.compilerPath = options.compiler_path.c_str();

			if ( string_ends_with( options.compiler_path.c_str(), "clang" ) ) {
				// get the users clang compiler version
				// use that to check against the version set in set_builder_options() later (if they set a version there)
				{
					const char* clangVersionPrefix = "clang version ";

					Array<const char*> args;
					args.add( options.compiler_path.c_str() );
					args.add( "-v" );

					Process* process = process_create( &args, NULL, PROCESS_FLAG_ASYNC | PROCESS_FLAG_COMBINE_STDOUT_AND_STDERR );

					assert( process );

					StringBuilder clangOutput = {};
					string_builder_reset( &clangOutput );
					defer( string_builder_destroy( &clangOutput ) );

					char buffer[1024] = {};
					u64 bytesRead = U64_MAX;
					while ( ( bytesRead = process_read_stdout( process, buffer, 1024 ) ) ) {
						buffer[bytesRead] = 0;

						string_builder_appendf( &clangOutput, "%s", buffer );
					}

					const char* clangOutputString = string_builder_to_string( &clangOutput );

					const char* versionStart = strstr( clangOutputString, clangVersionPrefix );

					if ( versionStart ) {
						versionStart += strlen( clangVersionPrefix );

						const char* versionEnd = NULL;
						if ( !versionEnd ) versionEnd = strchr( versionStart, '\r' );
						if ( !versionEnd ) versionEnd = strchr( versionStart, '\n' );
						assert( versionEnd );

						u64 versionLength = cast( u64, versionEnd ) - cast( u64, versionStart );

						// DM!!! 30/07/2025: this never gets freed!
						char* clangVersionString = cast( char*, mem_alloc( ( versionLength + 1 ) * sizeof( char ) ) );
						strncpy( clangVersionString, versionStart, versionLength );
						clangVersionString[versionLength] = 0;

						compilerVersion = clangVersionString;
					}

					process_join( process );

					process_destroy( process );
					process = NULL;
				}
			} else if ( string_ends_with( options.compiler_path.c_str(), "cl" ) ) {
#if _WIN32
				CreateCompilerBackend_MSVC( &compilerBackend );
#else
				error(
					"It appears you want to compile with MSVC on a non-Windows platform.\n"
					"MSVC only supports Windows.  Sorry.\n"
				);

				QUIT_ERROR();
#endif
			} else if ( string_ends_with( options.compiler_path.c_str(), "gcc" ) ) {
				CreateCompilerBackend_GCC( &compilerBackend );

				// get the users gcc compiler version
				// use that to check against the version set in set_builder_options() later (if they set a version there)
				{
					const char* gccVersionPrefix = "gcc version ";

					Array<const char*> args;
					args.add( options.compiler_path.c_str() );
					args.add( "-v" );

					Process* process = process_create( &args, NULL, PROCESS_FLAG_ASYNC | PROCESS_FLAG_COMBINE_STDOUT_AND_STDERR );

					assert( process );

					StringBuilder gccOutput = {};
					string_builder_reset( &gccOutput );
					defer( string_builder_destroy( &gccOutput ) );

					char buffer[1024] = {};
					u64 bytesRead = U64_MAX;
					while ( ( bytesRead = process_read_stdout( process, buffer, 1024 ) ) ) {
						buffer[bytesRead] = 0;

						string_builder_appendf( &gccOutput, "%s", buffer );
					}

					const char* gccOutputString = string_builder_to_string( &gccOutput );

					const char* versionStart = strstr( gccOutputString, gccVersionPrefix );

					if ( versionStart ) {
						versionStart += strlen( gccVersionPrefix );

						const char* versionEnd = strchr( versionStart, ' ' );
						assert( versionEnd );

						u64 versionLength = cast( u64, versionEnd ) - cast( u64, versionStart );

						// DM!!! 30/07/2025: this never gets freed!
						char* gccVersionString = cast( char*, mem_alloc( ( versionLength + 1 ) * sizeof( char ) ) );
						strncpy( gccVersionString, versionStart, versionLength );
						gccVersionString[versionLength] = 0;

						compilerVersion = gccVersionString;
					}

					process_join( process );

					process_destroy( process );
					process = NULL;
				}
			} else {
				error(
					"The compiler you want to build with (\"%s\") is not one that I recognise.\n"
					"Currently, I only support: Clang, GCC, and MSVC.\n"
					"So you must use one of those compilers and make the compiler path end with the name of the executable.  Sorry!\n"
					, options.compiler_path.c_str()
				);

				QUIT_ERROR();
			}

			// init new compiler backend
			{
				float64 compilerBackInitStart = time_ms();

				if ( !compilerBackend.Init( &compilerBackend ) ) {
					QUIT_ERROR();
				}

				float64 compilerBackInitEnd = time_ms();

				compilerBackendInitTimeMS = compilerBackInitEnd - compilerBackInitStart;
			}
		}

		// check that version of the compiler the user actually has is what they expect it to be
		if ( !options.compiler_version.empty() ) {
			if ( !string_equals( compilerVersion.data, options.compiler_version.c_str() ) ) {
				warning(
					"I see that you are using compiler version \"%s\", but compiler version \"%s\" was set in %s.\n"
					"I will still compile, but things may not work as you expect.\n\n"
					, compilerVersion.data, options.compiler_version.c_str(), SET_BUILDER_OPTIONS_FUNC_NAME
				);
			}
		}

		// get the linker program that we need
		const char* compilerPathOnly = path_remove_file_from_path( options.compiler_path.c_str() );

		if ( compilerPathOnly ) {
			String linkerPathOld = compilerBackend.linkerPath;
			string_printf( &compilerBackend.linkerPath, "%s%c%s", compilerPathOnly, PATH_SEPARATOR, linkerPathOld.data );
		}
	}

	std::vector<BuildConfig> configsToBuild;

#if USE_BUILD_INFO_FILES
	const std::vector<BuildConfig>& dan_configs = buildInfoData.configs;
#else
	const std::vector<BuildConfig>& dan_configs = options.configs;
#endif

	// if only one config was added (either by user or as a default build) then we know we just want that one, no config command line arg is needed
	if ( dan_configs.size() == 1 ) {
		AddBuildConfigAndDependenciesUnique( &context, &dan_configs[0], configsToBuild );
	} else {
		if ( !inputConfigName ) {
			error(
				"This build has multiple configs, but you never specified a config name.\n"
				"You must pass in a config name via " ARG_CONFIG "\n"
				"Run builder " ARG_HELP_LONG " if you need help.\n"
			);

			QUIT_ERROR();
		}

		For ( size_t, configIndex, 0, dan_configs.size() ) {
			if ( dan_configs[configIndex].name.empty() ) {
				error(
					"You have multiple BuildConfigs in your build source file, but some of them have empty names.\n"
					"When you have multiple BuildConfigs, ALL of them MUST have non-empty names.\n"
					"You need to set 'BuildConfig::name' in every BuildConfig that you add via add_build_config() (including dependencies!).\n"
				);

				QUIT_ERROR();
			}
		}
	}

#if USE_BUILD_INFO_FILES
	buildInfoData.includeDependencies.resize( dan_configs.size() );
#endif

	// none of the configs can have the same name
	// TODO(DM): 14/11/2024: can we do better than o(n^2) here?
	For ( size_t, configIndexA, 0, dan_configs.size() ) {
		const char* configNameA = dan_configs[configIndexA].name.c_str();
		u64 configNameHashA = hash_string( configNameA, 0 );

		For ( size_t, configIndexB, 0, dan_configs.size() ) {
			if ( configIndexA == configIndexB ) {
				continue;
			}

			const char* configNameB = dan_configs[configIndexB].name.c_str();
			u64 configNameHashB = hash_string( configNameB, 0 );

			if ( configNameHashA == configNameHashB ) {
				error( "I found multiple configs with the name \"%s\".  All config names MUST be unique, otherwise I don't know which specific config you want me to build.\n", configNameA );
				QUIT_ERROR();
			}
		}
	}

	// of all the configs that the user filled out inside set_builder_options
	// find the one the user asked for in the command line
	if ( inputConfigName ) {
		bool8 foundConfig = false;
		For ( u64, configIndex, 0, dan_configs.size() ) {
			const BuildConfig* config = &dan_configs[configIndex];

			if ( hash_string( config->name.c_str(), 0 ) == inputConfigNameHash ) {
				AddBuildConfigAndDependenciesUnique( &context, config, configsToBuild );

				foundConfig = true;

				break;
			}
		}

		if ( !foundConfig ) {
			error( "You passed the config name \"%s\" via the command line, but I never found a config with that name inside %s.  Make sure they match.\n", inputConfigName, SET_BUILDER_OPTIONS_FUNC_NAME );
			QUIT_ERROR();
		}
	}

	if ( preBuildFunc ) {
		printf( "Running pre-build code...\n" );

		const char* oldCWD = path_current_working_directory();
		SetCurrentDirectory( context.inputFilePath.data );
		defer( SetCurrentDirectory( oldCWD ) );

		preBuildFunc();
	}

	u32 numSuccessfulBuilds = 0;
	u32 numFailedBuilds = 0;
	u32 numSkippedBuilds = 0;

	For ( u64, configToBuildIndex, 0, configsToBuild.size() ) {
		BuildConfig* config = &configsToBuild[configToBuildIndex];

		assert( !config->binary_name.empty() );

		u64 configNameHash = hash_string( config->name.c_str(), 0 );
		u64 actualConfigIndex = hashmap_get_value( context.configIndices, configNameHash );

		// make sure that the binary folder and binary name are at least set to defaults
		if ( !config->binary_folder.empty() ) {
			config->binary_folder = tprintf( "%s%c%s", context.inputFilePath.data, PATH_SEPARATOR, config->binary_folder.c_str() );
		} else {
			config->binary_folder = context.inputFilePath.data;
		}

		{
			printf( "Building \"%s\"", config->binary_name.c_str() );

			if ( !config->name.empty() ) {
				printf( ", config \"%s\"", config->name.c_str() );
			}

			printf( "\n" );
		}

		// make all non-absolute additional include paths relative to the build source file
		For ( u64, includeIndex, 0, config->additional_includes.size() ) {
			const char* additionalInclude = config->additional_includes[includeIndex].c_str();

			if ( path_is_absolute( additionalInclude ) ) {
				config->additional_includes[includeIndex] = additionalInclude;
			} else {
				config->additional_includes[includeIndex] = tprintf( "%s%c%s", context.inputFilePath.data, PATH_SEPARATOR, additionalInclude );
			}
		}

		// make all non-absolute additional library paths relative to the build source file
		For ( u64, libPathIndex, 0, config->additional_lib_paths.size() ) {
			const char* additionalLibPath = config->additional_lib_paths[libPathIndex].c_str();

			if ( path_is_absolute( additionalLibPath ) ) {
				config->additional_lib_paths[libPathIndex] = additionalLibPath;
			} else {
				config->additional_lib_paths[libPathIndex] = tprintf( "%s%c%s", context.inputFilePath.data, PATH_SEPARATOR, additionalLibPath );
			}
		}

		// get all the "compilation units" that we are actually going to give to the compiler
		// if no source files were added in set_builder_options() then assume they only want to build the same file as the one specified via the command line
		if ( config->source_files.size() == 0 ) {
			config->source_files.push_back( context.inputFile );
		} else {
			// otherwise the user told us to build other source files, so go find and build those instead
			// keep this as a std::vector because this gets fed back into BuilderOptions::source_files
			std::vector<std::string> finalSourceFilesToBuild = BuildConfig_GetAllSourceFiles( &context, config );

			// at this point its totally acceptable for finalSourceFilesToBuild to be empty
			// this is because the compiler should be the one that tells the user they specified no valid source files to build with
			// the compiler can and will throw an error for that, so let it

			// the .build_info file wont store the full paths to the source files because the input path can change depending on whether we're building via visual studio or from the command line
			// so we need to reconstruct the full paths to the source files ourselves
			For ( u64, sourceFileIndex, 0, finalSourceFilesToBuild.size() ) {
				finalSourceFilesToBuild[sourceFileIndex] = tprintf( "%s%c%s", context.inputFilePath.data, PATH_SEPARATOR, finalSourceFilesToBuild[sourceFileIndex].c_str() );
			}

			config->source_files = finalSourceFilesToBuild;
		}

#if USE_BUILD_INFO_FILES
		buildInfoData.includeDependencies[actualConfigIndex].resize( config->source_files.size() );
#else
		context.includeDependencies.resize( config->source_files.size() );
#endif

		// now do the actual build
		{
			float64 buildTimeStart = time_ms();

#if USE_BUILD_INFO_FILES
			context.includeDependencies = buildInfoData.includeDependencies[actualConfigIndex];
#endif

			buildResult_t buildResult = BuildBinary( config, &compilerBackend, context.includeDependencies );

#if USE_BUILD_INFO_FILES
			buildInfoData.includeDependencies[actualConfigIndex] = context.includeDependencies;
#endif

			float64 buildTimeEnd = time_ms();

			switch ( buildResult ) {
				case BUILD_RESULT_SUCCESS:
					numSuccessfulBuilds++;
					printf( "Finished building \"%s\", %f ms\n\n", config->binary_name.c_str(), buildTimeEnd - buildTimeStart );
					break;

				case BUILD_RESULT_FAILED:
					numFailedBuilds++;
					error( "Build failed.\n\n" );
					QUIT_ERROR();

				case BUILD_RESULT_SKIPPED:
					numSkippedBuilds++;
					printf( "Skipped!\n\n" );
					break;
			}
		}

		mem_reset_temp_storage();
	}

	if ( postBuildFunc ) {
		printf( "Running post-build code...\n" );

		const char* oldCWD = path_current_working_directory();
		SetCurrentDirectory( context.inputFilePath.data );
		defer( SetCurrentDirectory( oldCWD ) );

		postBuildFunc();
	}

#if USE_BUILD_INFO_FILES
	// only dont want to write the .build_info if all the builds skipped or if a build failed
	bool shouldWriteBuildInfo = ( numSuccessfulBuilds > 0 ) && ( numFailedBuilds == 0 );

	if ( shouldWriteBuildInfo ) {
		float64 start = time_ms();

		if ( !BuildInfo_Write( &context, &buildInfoData ) ) {
			QUIT_ERROR();
		}

		float64 end = time_ms();

		buildInfoWriteTimeMS = end - start;
	}
#endif

	float64 totalTimeEnd = time_ms();

	printf( "Build finished:\n" );
	printf( "    User config build:   %f ms%s\n", userConfigBuildTimeMS, ( userConfigBuildResult == BUILD_RESULT_SKIPPED ) ? " (skipped)" : "" );
	if ( !doubleeq( compilerBackendInitTimeMS, -1.0 ) ) {
		printf( "    Compiler init time:  %f ms\n", compilerBackendInitTimeMS );
	}
	if ( !doubleeq( setBuilderOptionsTimeMS, -1.0 ) ) {
		printf( "    set_builder_options: %f ms\n", setBuilderOptionsTimeMS );
	}
	if ( options.generate_solution && !isVisualStudioBuild ) {
		printf( "    Generate solution:   %f ms\n", visualStudioGenerationTimeMS );
	}
#if USE_BUILD_INFO_FILES
	if ( readBuildInfo ) {
		printf( "    Read .build_info:    %f ms\n", buildInfoReadTimeMS );
	}
	if ( shouldWriteBuildInfo ) {
		printf( "    Write .build_info:   %f ms\n", buildInfoWriteTimeMS );
	}
#endif
	printf( "    Total time:          %f ms\n", totalTimeEnd - totalTimeStart );
	printf( "\n" );

	return 0;
}