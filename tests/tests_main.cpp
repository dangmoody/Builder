#include "../src/builder_local.h"

#include <debug.h>
#include <array.inl>
#include <typecast.inl>
#include <paths.h>
#include <string_builder.h>

#define TEMPERDEV_ASSERT assert
#define TEMPER_IMPLEMENTATION
#include "../src/core/include/file.h"
#include "../src/core/include/string_helpers.h"
#include "temper/temper.h"


enum compilerFlagBits_t {
	COMPILER_DEFAULT		= bit( 0 ),
	COMPILER_CLANG			= bit( 1 ),
	COMPILER_GCC			= bit( 2 ),
	COMPILER_MSVC_SHORT		= bit( 3 ),
	COMPILER_MSVC_FULL_PATH	= bit( 4 ),

	COMPILER_ALL			= COMPILER_DEFAULT | COMPILER_CLANG | COMPILER_GCC | COMPILER_MSVC_SHORT | COMPILER_MSVC_FULL_PATH
};
typedef u32 compilerFlags_t;

struct buildTest_t {
	const char		*rootDir;				// whats the root folder of this test?
	const char		*buildSourceFile;		// if defaultCompilerOnly is enabled then this just wants to be the source file that holds your build configs
	const char		*config;				// can be NULL
	const char		*binaryFolder;			// if NULL, assumed that no folder was created as part of the build
	const char		*binaryName;			// if NULL, assumed to be the same as buildSourceFile except it ends with .exe (on windows)
	s32				expectedExitCode;
	bool8			noSymbolFiles;
	compilerFlags_t	compilers = COMPILER_ALL;
};

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wswitch"

static const char *GetCompilerPath( const compilerFlagBits_t compiler ) {
	switch ( compiler ) {
		case COMPILER_DEFAULT:			return NULL;
		case COMPILER_CLANG:			return "../../clang/bin/clang";
		case COMPILER_GCC:				return "../../tools/gcc/bin/gcc";
		case COMPILER_MSVC_SHORT:		return "cl";
#ifdef _WIN32
		case COMPILER_MSVC_FULL_PATH: {
			msvcInstall_t msvcInstall = {};
			if ( Win_GetMSVCInstall( &msvcInstall ) ) {
				return tprintf( "%s\\bin\\Hostx64\\x64\\cl", msvcInstall.rootFolder.data );
			}
			return NULL;
		}
#endif
	}

	TEMPER_CHECK_TRUE_M( false, "Bad compilerFlagBits_t passed\n" );

	return NULL;
}

static const char *GetCompilerVersion( const compilerFlagBits_t compiler ) {
	switch ( compiler ) {
		case COMPILER_CLANG:			return "20.1.5";
		case COMPILER_GCC:				return "15.1.0";
		case COMPILER_MSVC_SHORT:		return "14.44.35207";
		case COMPILER_MSVC_FULL_PATH:	return "14.44.35207";
	}

	TEMPER_CHECK_TRUE_M( false, "Bad compilerFlagBits_t passed\n" );

	return NULL;
}

#pragma clang diagnostic pop

struct buildTestGeneratedFiles_t {
	Array<const char *>	folders;
	Array<const char *>	files;
	Array<const char *>	fileExtensionsToDelete;
};

static void GetAllGeneratedFiles( const FileInfo *fileInfo, void *data ) {
	buildTestGeneratedFiles_t *generatedFiles = cast( buildTestGeneratedFiles_t *, data );

	if ( fileInfo->is_directory ) {
		generatedFiles->folders.add( fileInfo->full_filename );
	} else {
		if ( FileIsSourceFile( fileInfo->filename ) || FileIsHeaderFile( fileInfo->filename ) ) {
			return;
		}

		For ( u32, fileExtensionIndex, 0, generatedFiles->fileExtensionsToDelete.count ) {
			if ( string_ends_with( fileInfo->full_filename, generatedFiles->fileExtensionsToDelete[fileExtensionIndex] ) ) {
				bool8 duplicate = false;

				For ( u32, fileIndex, 0, generatedFiles->files.count ) {
					if ( string_equals( generatedFiles->files[fileIndex], fileInfo->full_filename ) ) {
						duplicate = true;
						break;
					}
				}

				if ( !duplicate ) {
					printf( "Found generated file %s\n", fileInfo->full_filename );

					generatedFiles->files.add( fileInfo->full_filename );
				}

				break;
			}
		}
	}
}


struct fileMatchesFilterTest_t {
	const char	*filename;
	const char	*filter;
	bool8		expected;
};

TEMPER_TEST_PARAMETRIC( Test_FileMatchesFilter, TEMPER_FLAG_SHOULD_RUN, fileMatchesFilterTest_t test ) {
	TEMPER_CHECK_TRUE_M( FileMatchesFilter( test.filename, test.filter ) == test.expected, "FileMatchesFilter( \"%s\", \"%s\" ): expected %s.\n", test.filename, test.filter, test.expected ? "true" : "false" );
}

TEMPER_INVOKE_PARAMETRIC_TEST( Test_FileMatchesFilter, { "main.cpp",          "main.cpp",     true  } );
TEMPER_INVOKE_PARAMETRIC_TEST( Test_FileMatchesFilter, { "src/main.cpp",      "src/main.cpp", true  } );
TEMPER_INVOKE_PARAMETRIC_TEST( Test_FileMatchesFilter, { "main.h",            "main.cpp",     false } );
TEMPER_INVOKE_PARAMETRIC_TEST( Test_FileMatchesFilter, { "main.cpp",          "main.cpp.bak", false } );
TEMPER_INVOKE_PARAMETRIC_TEST( Test_FileMatchesFilter, { "main.cpp",          "*.cpp",        true  } );
TEMPER_INVOKE_PARAMETRIC_TEST( Test_FileMatchesFilter, { "main.h",            "*.cpp",        false } );
TEMPER_INVOKE_PARAMETRIC_TEST( Test_FileMatchesFilter, { "src/main.cpp",      "src/*.cpp",    true  } );
TEMPER_INVOKE_PARAMETRIC_TEST( Test_FileMatchesFilter, { "other/main.cpp",    "src/*.cpp",    false } );
TEMPER_INVOKE_PARAMETRIC_TEST( Test_FileMatchesFilter, { "src/sub/other.cpp", "src/*.cpp",    false } );
TEMPER_INVOKE_PARAMETRIC_TEST( Test_FileMatchesFilter, { "src/sub/other.cpp", "src/**/*.cpp", true  } );
TEMPER_INVOKE_PARAMETRIC_TEST( Test_FileMatchesFilter, { "src/a/b/other.cpp", "src/**/*.cpp", true  } );
TEMPER_INVOKE_PARAMETRIC_TEST( Test_FileMatchesFilter, { "src/main.cpp",      "src/**/*.cpp", true  } );


static bool SourceFileListContains( const std::vector<std::string> &files, const char *filename ) {
	For ( u64, fileIndex, 0, files.size() ) {
		if ( string_ends_with( files[fileIndex].c_str(), filename ) ) {
			return true;
		}
	}
	return false;
}

struct sourceFilesPatternTest_t {
	const char					*basePath;
	const char					*pattern;
	std::vector<const char *>	expectedPresent;
	std::vector<const char *>	expectedAbsent;
	u64							expectedCount;
};

TEMPER_TEST_PARAMETRIC( Test_GetSourceFilesMatchingPattern, TEMPER_FLAG_SHOULD_RUN, sourceFilesPatternTest_t test ) {
	std::vector<std::string> files = GetSourceFilesMatchingPattern( test.basePath, test.pattern );

	TEMPER_CHECK_TRUE_M( files.size() == test.expectedCount, "Expected %zu files, got %zu.\n", test.expectedCount, files.size() );

	For ( u64, fileIndex, 0, test.expectedPresent.size() ) {
		TEMPER_CHECK_TRUE_M( SourceFileListContains( files, test.expectedPresent[fileIndex] ), "Expected \"%s\" in results.\n", test.expectedPresent[fileIndex] );
	}

	For ( u64, fileIndex, 0, test.expectedAbsent.size() ) {
		TEMPER_CHECK_FALSE_M( SourceFileListContains( files, test.expectedAbsent[fileIndex] ), "Did not expect \"%s\" in results.\n", test.expectedAbsent[fileIndex] );
	}
}

TEMPER_INVOKE_PARAMETRIC_TEST( Test_GetSourceFilesMatchingPattern, {
	.basePath        = "test_file_globbing/src",
	.pattern         = "test_file_globbing/src/*.cpp",
	.expectedPresent = { "main.cpp", "helper.cpp" },
	.expectedAbsent  = { "main.h", "other.cpp" },
	.expectedCount   = 2,
} );

TEMPER_INVOKE_PARAMETRIC_TEST( Test_GetSourceFilesMatchingPattern, {
	.basePath        = "test_file_globbing/src",
	.pattern         = "test_file_globbing/src/**/*.cpp",
	.expectedPresent = { "main.cpp", "helper.cpp", "other.cpp" },
	.expectedAbsent  = { "main.h" },
	.expectedCount   = 3,
} );

TEMPER_INVOKE_PARAMETRIC_TEST( Test_GetSourceFilesMatchingPattern, {
	.basePath        = "test_file_globbing/src",
	.pattern         = "test_file_globbing/src/main.cpp",
	.expectedPresent = { "main.cpp" },
	.expectedCount   = 1,
} );

TEMPER_INVOKE_PARAMETRIC_TEST( Test_GetSourceFilesMatchingPattern, {
	.basePath      = "test_file_globbing/src",
	.pattern       = "test_file_globbing/src/*.xyz",
	.expectedCount = 0,
} );


TEMPER_TEST_PARAMETRIC( TestBuild, TEMPER_FLAG_SHOULD_RUN, buildTest_t test ) {
	TEMPER_CHECK_TRUE_M( test.rootDir, "A test MUST live in its own folder, you need to tell me what the \"root\" folder for this test is.\n" );

	// move ourselves to the root folder of that test
	// run the test from that folder
	// then come back when were done
	const char *oldCWD = path_current_working_directory();
	TEMPER_CHECK_TRUE_M( path_set_current_directory( test.rootDir ), "Failed to cd into the test folder \"%s\".\n", test.rootDir );
	defer( TEMPER_CHECK_TRUE_M( path_set_current_directory( oldCWD ), "Failed to cd back out of the test folder.\n" ) );

	// get all the files that this test will generate
	// we will want these later to check if they got successfully deleted (tests should clean up after themselves properly)
	buildTestGeneratedFiles_t generatedFiles = {};
#ifdef _WIN32
	// exes have no file extension on linux
	// which means when we check for this "extension" on linux we actually check if the string ends with "", which always passes
	// so only do this on windows, because we actually have a file extension to check against there
	generatedFiles.fileExtensionsToDelete.add( GetFileExtensionFromBinaryType( BINARY_TYPE_EXE ) );
#endif
	generatedFiles.fileExtensionsToDelete.add( GetFileExtensionFromBinaryType( BINARY_TYPE_DYNAMIC_LIBRARY ) );
	generatedFiles.fileExtensionsToDelete.add( GetFileExtensionFromBinaryType( BINARY_TYPE_STATIC_LIBRARY ) );
	generatedFiles.fileExtensionsToDelete.add( ".include_dependencies" );
	generatedFiles.fileExtensionsToDelete.add( ".pdb" );
	generatedFiles.fileExtensionsToDelete.add( ".exp" );
	generatedFiles.fileExtensionsToDelete.add( ".ilk" );
	generatedFiles.fileExtensionsToDelete.add( ".o" );
	generatedFiles.fileExtensionsToDelete.add( ".d" );
	generatedFiles.fileExtensionsToDelete.add( ".json" );

	const char *buildSourceFileWithoutExtension = path_remove_file_extension( test.buildSourceFile );

	// binary name doesnt have to be set by users, but we need it
	// this is the default
	if ( !test.binaryName ) {
		test.binaryName = buildSourceFileWithoutExtension;
	}

	For ( u32, compilerIndex, 0, COMPILER_ALL ) {
		compilerFlagBits_t compiler = cast( compilerFlagBits_t, bit( compilerIndex ) );

#ifdef __linux__
		if ( compiler == COMPILER_MSVC_SHORT || compiler == COMPILER_MSVC_FULL_PATH ) {
			continue;
		}
#endif

		if ( ( test.compilers & compiler ) == 0 ) {
			continue;
		}

		s32 exitCode = 0;

		// test doing the actual build
		{
			// DM: 04/02/2026: this is stupid and ugly
			// args should also be const because we never actually modify them
			// I know how to get this done, leave this work with me
			Array<const char *> args;
			args.add( test.buildSourceFile );

			if ( test.config ) {
				args.add( tprintf( "--config=%s", test.config ) );
			}

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wswitch"
			switch ( compiler ) {
				case COMPILER_CLANG:			args.add( "--clang" );	break;
				case COMPILER_GCC:				args.add( "--gcc" );	break;
				case COMPILER_MSVC_SHORT:		args.add( "--msvc" );	break;
#ifdef _WIN32
				case COMPILER_MSVC_FULL_PATH: {
					msvcInstall_t msvcInstall = {};
					TEMPER_CHECK_TRUE_M( Win_GetMSVCInstall( &msvcInstall ), "Failed to find MSVC install for full-path compiler test.\n" );
					args.add( tprintf( "--msvc-full-path=%s\\bin\\Hostx64\\x64\\cl", msvcInstall.rootFolder.data ) );
				} break;
#endif
			}
#pragma clang diagnostic pop

			exitCode = BuilderMain( 0, trunc_cast( int, args.count ), args.data );

			TEMPER_CHECK_TRUE_M( exitCode == 0, "BuilderMain() actually returned %d.\n", exitCode );
		}

		// linux requires the "./" prefix because without that it tries to run the subprocess from your PATH
		const char *fullBinaryName = NULL;
		if ( test.binaryFolder ) {
			fullBinaryName = tprintf( "./%s%c%s%s", test.binaryFolder, PATH_SEPARATOR, test.binaryName, GetFileExtensionFromBinaryType( BINARY_TYPE_EXE ) );
		} else {
			fullBinaryName = tprintf( "./%s%s", test.binaryName, GetFileExtensionFromBinaryType( BINARY_TYPE_EXE ) );
		}

		const char *dotBuilderFolder = ".builder";

		generatedFiles.files.reset();
		generatedFiles.folders.reset();

		TEMPER_CHECK_TRUE( file_get_all_files_in_folder( dotBuilderFolder, true, true, GetAllGeneratedFiles, &generatedFiles ) );
		if ( test.binaryFolder ) {
			TEMPER_CHECK_TRUE( file_get_all_files_in_folder( test.binaryFolder, true, true, GetAllGeneratedFiles, &generatedFiles ) );
		} else {
			// if there is no binary folder specified then binaries get made in the same folder as the build source file
			TEMPER_CHECK_TRUE( file_get_all_files_in_folder( "./", false, false, GetAllGeneratedFiles, &generatedFiles ) );

			// if there is no binary folder specified then an intermediate folder gets made in the same folder as the build source file
			// TEMPER_CHECK_TRUE( file_get_all_files_in_folder( "intermediate", true, true, GetAllGeneratedFiles, &generatedFiles ) );
		}

		// we only care that certain files and folders got generated
		{
			TEMPER_CHECK_TRUE( file_exists( fullBinaryName ) );
			TEMPER_CHECK_TRUE( folder_exists( dotBuilderFolder ) );

			const char *userConfigBuildDLLFilename = tprintf( "%s%c%s%s", dotBuilderFolder, PATH_SEPARATOR, buildSourceFileWithoutExtension, GetFileExtensionFromBinaryType( BINARY_TYPE_DYNAMIC_LIBRARY ) );
			TEMPER_CHECK_TRUE( file_exists( userConfigBuildDLLFilename ) );
		}

		// now run the program we just built
		{
			Array<const char *> args;
			args.add( fullBinaryName );

			exitCode = RunProc( &args, NULL );

			TEMPER_CHECK_TRUE_M( exitCode == test.expectedExitCode, "When trying to run \"%s\", the exit code was expected to be %d but it actually returned %d.\n", test.binaryName, test.expectedExitCode, exitCode );
		}

		// cleanup all generated files
		{
			For ( u32, fileIndex, 0, generatedFiles.files.count ) {
				const char *generatedFile = generatedFiles.files[fileIndex];

				printf( "Deleting file %s ... ", generatedFile );

				TEMPER_CHECK_TRUE_M( file_delete( generatedFile ), "Couldn't delete file \"%s\".\n", generatedFile );
				TEMPER_CHECK_TRUE_M( !file_exists( generatedFile ), "We deleted the file \"%s\" just now, but the OS tells us it still exists?\n", generatedFile );

				printf( "Done\n" );
			}

			For ( u32, folderIndex, 0, generatedFiles.folders.count ) {
				const char *generatedFolder = generatedFiles.folders[folderIndex];

				TEMPER_CHECK_TRUE_M( folder_delete( generatedFolder ), "Couldn't delete folder \"%s\".\n", generatedFolder );
				TEMPER_CHECK_TRUE_M( !folder_exists( generatedFolder ), "We deleted the folder \"%s\" just now, but the OS tells us it still exists?\n", generatedFolder );
			}
		}
	}
}

TEMPER_INVOKE_PARAMETRIC_TEST( TestBuild, {
	.rootDir			= "test_basic",
	.buildSourceFile	= "test_basic.cpp",
	.compilers			= COMPILER_DEFAULT,
} );

TEMPER_INVOKE_PARAMETRIC_TEST( TestBuild, {
	.rootDir			= "test_basic_stdlib",
	.buildSourceFile	= "test_basic_stdlib.cpp",
	.compilers			= COMPILER_DEFAULT,
} );

TEMPER_INVOKE_PARAMETRIC_TEST( TestBuild, {
	.rootDir			= "test_dynamic_runtime_linking",
	.buildSourceFile	= "test_dynamic_runtime_linking.cpp",
	.compilers			= COMPILER_DEFAULT,
} );

TEMPER_INVOKE_PARAMETRIC_TEST( TestBuild, {
	.rootDir			= "test_set_builder_options",
	.buildSourceFile	= "test_set_builder_options.cpp",
	.config				= "debug",
	.binaryFolder		= "bin/debug",
	.binaryName			= "kenneth",
	.expectedExitCode	= 69,
	.compilers			= COMPILER_DEFAULT,
} );

TEMPER_INVOKE_PARAMETRIC_TEST( TestBuild, {
	.rootDir			= "test_set_builder_options",
	.buildSourceFile	= "test_set_builder_options.cpp",
	.config				= "release",
	.binaryFolder		= "bin/release",
	.binaryName			= "kenneth",
	.expectedExitCode	= 69,
	.noSymbolFiles		= true,
	.compilers			= COMPILER_DEFAULT,
} );

TEMPER_INVOKE_PARAMETRIC_TEST( TestBuild, {
	.rootDir			= "test_multiple_source_files",
	.buildSourceFile	= "build.cpp",
	.binaryFolder		= "bin",
	.binaryName			= "marco_polo",
} );

TEMPER_INVOKE_PARAMETRIC_TEST( TestBuild, {
	.rootDir			= "test_static_lib",
	.buildSourceFile	= "build.cpp",
	.config				= "program",
	.binaryFolder		= "bin",
	.binaryName			= "test_static_library_program",
} );

TEMPER_INVOKE_PARAMETRIC_TEST( TestBuild, {
	.rootDir			= "test_dynamic_lib",
	.buildSourceFile	= "build.cpp",
	.config				= "program",
	.binaryFolder		= "bin",
	.binaryName			= "test_dynamic_library_program",
} );

TEMPER_INVOKE_PARAMETRIC_TEST( TestBuild, {
	.rootDir			= "test_compilation_database",
	.buildSourceFile	= "build.cpp",
	.binaryFolder		= "bin",
	.binaryName			= "test_compilation_database_program",
} );

TEMPER_TEST( GenerateVisualStudioSolution, TEMPER_FLAG_SHOULD_RUN ) {
	s32 exitCode = -1;

	// need to find where msbuild lives on windows
#ifdef _WIN32
	std::string msbuildInstallationPath;

	// detect where msbuild is stored
	{
		String vswhereStdout;

		Array<const char *> args;
		args.add( "C:\\Program Files (x86)\\Microsoft Visual Studio\\Installer\\vswhere.exe" );
		args.add( "-latest" );
		args.add( "-products" );
		args.add( "*" );
		args.add( "-requires" );
		args.add( "Microsoft.Component.MSBuild" );
		exitCode = RunProc( &args, NULL, 0, &vswhereStdout );

		// fail test if vswhere errors
		TEMPER_CHECK_TRUE_M( exitCode == 0, "Failed to run vswhere.exe properly.  Exit code actually returned %d.\n", exitCode );

		// fail test if we cant find the tag in the output that were looking for
		// DM!!! this lambda is duplicated! unify!
		auto ParseTagString = []( const char *fileBuffer, const char *tag, std::string &outString ) -> bool8 {
			const char *lineStart = strstr( fileBuffer, tag );
			if ( !lineStart ) {
				return false;
			}

			lineStart += strlen( tag );

			while ( *lineStart == ' ' ) {
				lineStart++;
			}

			const char *lineEnd = NULL;
			if ( !lineEnd ) lineEnd = strchr( lineStart, '\r' );
			if ( !lineEnd ) lineEnd = strchr( lineStart, '\n' );
			assert( lineEnd );

			outString = std::string( lineStart, lineEnd );

			return true;
		};
		TEMPER_CHECK_TRUE_M( ParseTagString( vswhereStdout.data, "installationPath:", msbuildInstallationPath ), "Failed to query for MSBuild installation path using vswhere.exe.\n" );
	}
#endif

	// generate the solution
	if ( exitCode == 0 ) {
		Array<const char *> args;
		args.add( "test_generate_visual_studio_files/generate_solution.cpp" );

		exitCode = BuilderMain( 0, trunc_cast( int, args.count ), args.data );

		TEMPER_CHECK_TRUE_M( exitCode == 0, "Exit code actually returned %d.\n", exitCode );
	}

	// DM: apparently msbuild isnt properly supported on linux so this isnt possible
	// I'm not convinced by that answer, but all of my reading says so
#ifdef _WIN32
	// build the app project in the solution via msbuild
	if ( exitCode == 0 ) {
		String msbuildStdout;

		Array<const char *> args;
#if defined( _WIN32 )
		args.add( tprintf( "%s%cMSBuild%cCurrent%cBin%cMSBuild.exe", msbuildInstallationPath.c_str(), PATH_SEPARATOR, PATH_SEPARATOR, PATH_SEPARATOR, PATH_SEPARATOR ) );	// TODO(DM): query for this instead
#elif defined( __linux__ )
		args.add( "msbuild" );
#endif
		args.add( "test_generate_visual_studio_files/visual_studio/app.vcxproj" );
		args.add( "/property:Platform=x64" );
		exitCode = RunProc( &args, NULL, PROC_FLAG_SHOW_STDOUT );

		TEMPER_CHECK_TRUE_M( exitCode == 0, "Exit code actually returned %d.\n", exitCode );
	}

	// run the program, make sure it returns the correct exit code
	if ( exitCode == 0 ) {
		Array<const char *> args;
		args.add( "test_generate_visual_studio_files/bin/debug/the-app.exe" );
		exitCode = RunProc( &args, NULL );

		TEMPER_CHECK_TRUE_M( exitCode == 69420, "Exit code actually returned %d.\n", exitCode );
	}
#endif // _WIN32
}

TEMPER_TEST( GenerateVSCodeJSONFiles, TEMPER_FLAG_SHOULD_RUN ) {
	const char *buildFile				= "test_generate_vscode_json_files/generate_vscode_json.cpp";
	const char *dotBuilderFolder		= "test_generate_vscode_json_files/.builder";
	const char *vsCodeFolder			= "test_generate_vscode_json_files/.vscode";
	const char *cppPropertiesJSONPath	= "test_generate_vscode_json_files/.vscode/c_cpp_properties.json";
	const char *tasksJSONPath			= "test_generate_vscode_json_files/.vscode/tasks.json";
	const char *launchJSONPath			= "test_generate_vscode_json_files/.vscode/launch.json";

	// generate the VS Code JSON files
	{
		Array<const char *> args;
		args.add( buildFile );

		s32 exitCode = BuilderMain( 0, trunc_cast( int, args.count ), args.data );
		TEMPER_CHECK_TRUE_M( exitCode == 0, "BuilderMain() returned %d.\n", exitCode );
	}

	// c_cpp_properties.json
	{
		TEMPER_CHECK_TRUE_M( file_exists( cppPropertiesJSONPath ), "c_cpp_properties.json was not generated at \"%s\".\n", cppPropertiesJSONPath );

		char *content = NULL;
		u64 contentLength = 0;
		TEMPER_CHECK_TRUE_M( file_read_entire( cppPropertiesJSONPath, &content, &contentLength ), "Failed to read c_cpp_properties.json.\n" );
		defer( file_free_buffer( &content ) );

		TEMPER_CHECK_TRUE_M( string_contains( content, "\"configurations\"" ),         "c_cpp_properties.json is missing \"configurations\" array.\n" );
		TEMPER_CHECK_TRUE_M( string_contains( content, "\"version\": 4" ),             "c_cpp_properties.json is missing \"version\": 4.\n" );
		TEMPER_CHECK_TRUE_M( string_contains( content, "\"name\": \"config\"" ),       "c_cpp_properties.json is missing config name.\n" );
		TEMPER_CHECK_TRUE_M( string_contains( content, "\"includePath\"" ),            "c_cpp_properties.json is missing \"includePath\".\n" );
		TEMPER_CHECK_TRUE_M( string_contains( content, "${workspaceFolder}/include" ), "c_cpp_properties.json is missing include path.\n" );
		TEMPER_CHECK_TRUE_M( string_contains( content, "\"defines\"" ),                "c_cpp_properties.json is missing \"defines\".\n" );
		TEMPER_CHECK_TRUE_M( string_contains( content, "MY_DEFINE=1" ),                "c_cpp_properties.json is missing define.\n" );
		TEMPER_CHECK_TRUE_M( string_contains( content, "\"cppStandard\": \"c++17\"" ), "c_cpp_properties.json is missing cppStandard.\n" );
#if defined( _WIN32 )
		TEMPER_CHECK_TRUE_M( string_contains( content, "\"windows-clang-x64\"" ),      "c_cpp_properties.json is missing intelliSenseMode.\n" );
#else
		TEMPER_CHECK_TRUE_M( string_contains( content, "\"linux-clang-x64\"" ),        "c_cpp_properties.json is missing intelliSenseMode.\n" );
#endif
	}

	// tasks.json
	{
		TEMPER_CHECK_TRUE_M( file_exists( tasksJSONPath ), "tasks.json was not generated at \"%s\".\n", tasksJSONPath );

		char *content = NULL;
		u64 contentLength = 0;
		TEMPER_CHECK_TRUE_M( file_read_entire( tasksJSONPath, &content, &contentLength ), "Failed to read tasks.json.\n" );
		defer( file_free_buffer( &content ) );

		TEMPER_CHECK_TRUE_M( string_contains( content, "\"version\"" ),              "tasks.json is missing \"version\".\n" );
		TEMPER_CHECK_TRUE_M( string_contains( content, "\"tasks\"" ),                "tasks.json is missing \"tasks\" array.\n" );
		TEMPER_CHECK_TRUE_M( string_contains( content, "\"Build config\"" ),         "tasks.json is missing \"Build config\" task.\n" );
		TEMPER_CHECK_TRUE_M( string_contains( content, "\"--config=config\"" ),      "tasks.json is missing --config=config arg.\n" );
		TEMPER_CHECK_TRUE_M( string_contains( content, "\"--release\"" ),            "tasks.json is missing --release arg for second task.\n" );
		TEMPER_CHECK_TRUE_M( string_contains( content, "generate_vscode_json.cpp" ), "tasks.json is missing the build file arg.\n" );
	}

	// launch.json
	{
		TEMPER_CHECK_TRUE_M( file_exists( launchJSONPath ), "launch.json was not generated at \"%s\".\n", launchJSONPath );

		char *content = NULL;
		u64 contentLength = 0;
		TEMPER_CHECK_TRUE_M( file_read_entire( launchJSONPath, &content, &contentLength ), "Failed to read launch.json.\n" );
		defer( file_free_buffer( &content ) );

		TEMPER_CHECK_TRUE_M( string_contains( content, "\"version\"" ),                     "launch.json is missing \"version\".\n" );
		TEMPER_CHECK_TRUE_M( string_contains( content, "\"configurations\"" ),              "launch.json is missing \"configurations\" array.\n" );
		TEMPER_CHECK_TRUE_M( string_contains( content, "bin/debug/test_app" ),              "launch.json is missing debug binary path.\n" );
		TEMPER_CHECK_TRUE_M( string_contains( content, "bin/release/test_app" ),            "launch.json is missing release binary path.\n" );
		TEMPER_CHECK_TRUE_M( string_contains( content, "\"request\": \"launch\"" ),         "launch.json is missing \"request\": \"launch\".\n" );
		TEMPER_CHECK_TRUE_M( string_contains( content, "\"cwd\": \"${workspaceFolder}\"" ), "launch.json is missing default cwd.\n" );
#ifdef _WIN32
		TEMPER_CHECK_TRUE_M( string_contains( content, "\"type\": \"cppvsdbg\"" ),          "launch.json is missing \"type\": \"cppvsdbg\" on Windows.\n" );
#else
		TEMPER_CHECK_TRUE_M( string_contains( content, "\"type\": \"cppdbg\"" ),            "launch.json is missing \"type\": \"cppdbg\" on Linux.\n" );
		TEMPER_CHECK_TRUE_M( string_contains( content, "\"MIMode\": \"gdb\"" ),             "launch.json is missing \"MIMode\": \"gdb\" on Linux.\n" );
#endif
	}

	// cleanup
	{
		buildTestGeneratedFiles_t generatedFiles = {};
		generatedFiles.fileExtensionsToDelete.add( GetFileExtensionFromBinaryType( BINARY_TYPE_DYNAMIC_LIBRARY ) );
		generatedFiles.fileExtensionsToDelete.add( ".d" );
		generatedFiles.fileExtensionsToDelete.add( ".o" );
		generatedFiles.fileExtensionsToDelete.add( ".include_dependencies" );
		generatedFiles.fileExtensionsToDelete.add( ".lib" );
		generatedFiles.fileExtensionsToDelete.add( ".exp" );
		generatedFiles.fileExtensionsToDelete.add( ".pdb" );
		generatedFiles.fileExtensionsToDelete.add( ".ilk" );
		generatedFiles.fileExtensionsToDelete.add( ".json" );

		TEMPER_CHECK_TRUE( file_get_all_files_in_folder( dotBuilderFolder, true, true, GetAllGeneratedFiles, &generatedFiles ) );
		TEMPER_CHECK_TRUE( file_get_all_files_in_folder( vsCodeFolder, true, true, GetAllGeneratedFiles, &generatedFiles ) );

		For ( u32, fileIndex, 0, generatedFiles.files.count ) {
			TEMPER_CHECK_TRUE_M( file_delete( generatedFiles.files[fileIndex] ), "Failed to delete \"%s\".\n", generatedFiles.files[fileIndex] );
		}

		TEMPER_CHECK_TRUE_M( folder_delete( dotBuilderFolder ), "Failed to delete \"%s\".\n", dotBuilderFolder );
		TEMPER_CHECK_TRUE_M( folder_delete( vsCodeFolder ), "Failed to delete .vscode folder.\n" );
	}
}

TEMPER_TEST( GenerateZedJSONFiles, TEMPER_FLAG_SHOULD_RUN ) {
	const char *dotBuilderFolder = "test_generate_zed_json_files/.builder";
	const char *tasksJSONPath    = "test_generate_zed_json_files/.zed/tasks.json";
	const char *debugJSONPath    = "test_generate_zed_json_files/.zed/debug.json";

	// generate the Zed JSON files
	{
		Array<const char *> args;
		args.add( "test_generate_zed_json_files/generate_zed_json.cpp" );

		s32 exitCode = BuilderMain( 0, trunc_cast( int, args.count ), args.data );
		TEMPER_CHECK_TRUE_M( exitCode == 0, "BuilderMain() returned %d.\n", exitCode );
	}

	// tasks.json
	{
		TEMPER_CHECK_TRUE_M( file_exists( tasksJSONPath ), "tasks.json was not generated at \"%s\".\n", tasksJSONPath );

		char *content = NULL;
		u64 contentLength = 0;
		TEMPER_CHECK_TRUE_M( file_read_entire( tasksJSONPath, &content, &contentLength ), "Failed to read tasks.json.\n" );
		defer( file_free_buffer( &content ) );

		TEMPER_CHECK_TRUE_M( string_contains( content, "\"label\""             ), "tasks.json is missing \"label\".\n" );
		TEMPER_CHECK_TRUE_M( string_contains( content, "\"Build config\""      ), "tasks.json is missing \"Build config\" task.\n" );
		TEMPER_CHECK_TRUE_M( string_contains( content, "\"command\""           ), "tasks.json is missing \"command\".\n" );
		TEMPER_CHECK_TRUE_M( string_contains( content, "\"args\""              ), "tasks.json is missing \"args\" array.\n" );
		TEMPER_CHECK_TRUE_M( string_contains( content, "\"--config=config\""   ), "tasks.json is missing --config=config arg.\n" );
		TEMPER_CHECK_TRUE_M( string_contains( content, "\"--release\""         ), "tasks.json is missing --release arg for second task.\n" );
		TEMPER_CHECK_TRUE_M( string_contains( content, "generate_zed_json.cpp" ), "tasks.json is missing the build file arg.\n" );
	}

	// debug.json
	{
		TEMPER_CHECK_TRUE_M( file_exists( debugJSONPath ), "debug.json was not generated at \"%s\".\n", debugJSONPath );

		char *content = NULL;
		u64 contentLength = 0;
		TEMPER_CHECK_TRUE_M( file_read_entire( debugJSONPath, &content, &contentLength ), "Failed to read debug.json.\n" );
		defer( file_free_buffer( &content ) );

		TEMPER_CHECK_TRUE_M( string_contains( content, "\"label\""                ), "debug.json is missing \"label\".\n" );
		TEMPER_CHECK_TRUE_M( string_contains( content, "\"config (debug)\""       ), "debug.json is missing config (debug) binary label.\n" );
		TEMPER_CHECK_TRUE_M( string_contains( content, "\"config (release)\""     ), "debug.json is missing config (release) binary label.\n" );
		TEMPER_CHECK_TRUE_M( string_contains( content, "\"program\""              ), "debug.json is missing \"program\".\n" );
		TEMPER_CHECK_TRUE_M( string_contains( content, "bin/debug/test_app"       ), "debug.json is missing debug binary path.\n" );
		TEMPER_CHECK_TRUE_M( string_contains( content, "bin/release/test_app"     ), "debug.json is missing release binary path.\n" );
		TEMPER_CHECK_TRUE_M( string_contains( content, "\"${ZED_WORKTREE_ROOT}\"" ), "debug.json is missing default cwd.\n" );
		TEMPER_CHECK_TRUE_M( string_contains( content, "\"adapter\""              ), "debug.json is missing \"adapter\".\n" );
		TEMPER_CHECK_TRUE_M( string_contains( content, "\"CodeLLDB\""             ), "debug.json is missing \"CodeLLDB\" adapter.\n" );
		TEMPER_CHECK_TRUE_M( string_contains( content, "\"request\""              ), "debug.json is missing \"request\".\n" );
		TEMPER_CHECK_TRUE_M( string_contains( content, "\"launch\""               ), "debug.json is missing \"launch\" request.\n" );
	}

	// cleanup
	{
		// delete all files in .builder before removing the folder
		// (rmdir requires an empty directory)
		{
			buildTestGeneratedFiles_t generatedFiles = {};
			generatedFiles.fileExtensionsToDelete.add( GetFileExtensionFromBinaryType( BINARY_TYPE_DYNAMIC_LIBRARY ) );
			generatedFiles.fileExtensionsToDelete.add( ".d" );
			generatedFiles.fileExtensionsToDelete.add( ".o" );
			generatedFiles.fileExtensionsToDelete.add( ".include_dependencies" );
			generatedFiles.fileExtensionsToDelete.add( ".lib" );
			generatedFiles.fileExtensionsToDelete.add( ".exp" );
			generatedFiles.fileExtensionsToDelete.add( ".pdb" );
			generatedFiles.fileExtensionsToDelete.add( ".ilk" );

			TEMPER_CHECK_TRUE( file_get_all_files_in_folder( dotBuilderFolder, true, true, GetAllGeneratedFiles, &generatedFiles ) );

			For ( u32, fileIndex, 0, generatedFiles.files.count ) {
				TEMPER_CHECK_TRUE_M( file_delete( generatedFiles.files[fileIndex] ), "Failed to delete \"%s\".\n", generatedFiles.files[fileIndex] );
			}

			TEMPER_CHECK_TRUE_M( folder_delete( dotBuilderFolder ), "Failed to delete \"%s\".\n", dotBuilderFolder );
		}

		// delete .zed contents then the folder
		if ( file_exists( tasksJSONPath ) ) {
			TEMPER_CHECK_TRUE_M( file_delete( tasksJSONPath ), "Failed to delete tasks.json.\n" );
		}
		if ( file_exists( debugJSONPath ) ) {
			TEMPER_CHECK_TRUE_M( file_delete( debugJSONPath ), "Failed to delete debug.json.\n" );
		}
		TEMPER_CHECK_TRUE_M( folder_delete( "test_generate_zed_json_files/.zed" ), "Failed to delete .zed folder.\n" );
	}
}

// Validates the generated compile_commands.json by feeding it to clang-tidy.
// If clang-tidy can successfully load the compilation database, it proves
// the JSON is correctly formatted according to the specification.
// https://clang.llvm.org/docs/JSONCompilationDatabase.html
TEMPER_TEST( ValidateCompilationDatabase, TEMPER_FLAG_SHOULD_RUN ) {
	const char *sourceFile = "tests/test_compilation_database/main.cpp";
	const char *compileCommandsDir = "test_compilation_database";
	const char *compileCommandsPath = "test_compilation_database/compile_commands.json";

	TEMPER_CHECK_TRUE_M( file_exists( compileCommandsPath ), "compile_commands.json does not exist at %s\n", compileCommandsPath );

	char *content = NULL;
	u64 contentLength = 0;
	file_read_entire( compileCommandsPath, &content, &contentLength );
	defer( file_free_buffer( &content ) );

	// Count occurrences of "file": which indicates individual entries
	u64 entriesCount = 0;
	const char *ptr = content;
	while ( ( ptr = strstr( ptr, "\"file\":" ) ) != NULL ) {
		entriesCount++;
		ptr++;
	}

	TEMPER_CHECK_TRUE_M( entriesCount == 2, "compile_commands.json file contains an unexpected number of entries (2)" );

	// clang-tidy -p <build-path> <source-file> --checks=-*
	//
	// Using --checks=-* disables all actual checks, so we only test whether
	// clang-tidy can successfully load the compilation database.
	// If the compile_commands.json is malformed, clang-tidy will fail with an error like:
	//     "Error while trying to load a compilation database"

	Array<const char *> args;
	args.add( "../clang/bin/clang-tidy" );
	args.add( sourceFile );
	args.add( tprintf( "-p=%s", compileCommandsDir ) );
	args.add( "--checks=-*" );  // Disable all checks - we only want to test DB loading

	Array<const char *> stdoutOutput;
	s32 exitCode = RunProc( &args, &stdoutOutput, false );
	bool isValid = true;
	// Check for specific error messages that indicate database problems
	if ( stdoutOutput.data ) {
		if ( strstr( *stdoutOutput.data, "Error while trying to load a compilation database" ) ) {
			printf( "clang-tidy failed to load compilation database: %s\n", *stdoutOutput.data );
			isValid = false;
		}
		if ( strstr( *stdoutOutput.data, "error: no compilation database found" ) ) {
			printf( "clang-tidy could not find compilation database\n" );
			isValid = false;
		}
	}

	TEMPER_CHECK_TRUE_M( isValid, "clang-tidy failed to load compile_commands.json - the file may be malformed\n" );
}


int main( int argc, char **argv ) {
	core_init( MEM_KILOBYTES( 64 ) );
	defer( core_shutdown() );

	TEMPER_RUN( argc, argv );

	int exitCode = TEMPER_GET_EXIT_CODE();

	if ( exitCode != 0 ) {
		debug_break();
	}

	return exitCode;
}
