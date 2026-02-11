#include "../src/builder_local.h"

#include <debug.h>
#include <array.inl>
#include <typecast.inl>
#include <paths.h>
#include <string_builder.h>

#ifdef __linux__
//#include <assert.h>
#endif // __linux__

#define TEMPERDEV_ASSERT assert
#define TEMPER_IMPLEMENTATION
#include "../src/core/include/file.h"
#include "../src/core/include/string_helpers.h"
#include "temper/temper.h"


static const char* GetFileExtensionFromBinaryType( BinaryType type ) {
#ifdef _WIN32
	switch ( type ) {
		case BINARY_TYPE_EXE:				return ".exe";
		case BINARY_TYPE_DYNAMIC_LIBRARY:	return ".dll";
		case BINARY_TYPE_STATIC_LIBRARY:	return ".lib";
	}
#elif defined( __linux__ )
	switch ( type ) {
		case BINARY_TYPE_EXE:				return "";
		case BINARY_TYPE_DYNAMIC_LIBRARY:	return ".so";
		case BINARY_TYPE_STATIC_LIBRARY:	return ".a";
	}
#else
#error Unrecognised paltform.
#endif

	assertf( false, "Something went really wrong here.\n" );

	return "ERROR";
}

struct buildTest_t {
	const char*	rootDir;			// whats the root folder of this test?
	const char*	buildSourceFile;	// if defaultCompilerOnly is enabled then this just wants to be the source file that holds your build configs
	const char*	config;				// can be NULL
	const char*	binaryFolder;		// if NULL, assumed that no folder was created as part of the build
	const char*	binaryName;			// if NULL, assumed to be the same as buildSourceFile except it ends with .exe (on windows)
	s32			expectedExitCode;
	bool8		noSymbolFiles;
	bool8 defaultCompilerOnly;		// if false will run this test for every compiler we support
};

enum compiler_t {
	COMPILER_DEFAULT	= 0,
	COMPILER_CLANG,
	COMPILER_GCC,
	COMPILER_MSVC,

	COMPILER_COUNT
};

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wswitch"

static const char* GetCompilerBuildSourceFileSuffix( const compiler_t compiler ) {
	switch ( compiler ) {
		case COMPILER_DEFAULT:	return	"default";
		case COMPILER_CLANG:	return	"clang";
		case COMPILER_GCC:		return	"gcc";
		case COMPILER_MSVC:		return	"msvc";
	}

	TEMPER_CHECK_TRUE_M( false, "Bad compiler_t passed\n" );

	return NULL;
}

static const char* GetCompilerPath( const compiler_t compiler ) {
	switch ( compiler ) {
		case COMPILER_DEFAULT:	return	NULL;
		case COMPILER_CLANG:	return	"../../clang/bin/clang";
		case COMPILER_GCC:		return	"../../tools/gcc/bin/gcc";
		case COMPILER_MSVC:		return	"cl";
	}

	TEMPER_CHECK_TRUE_M( false, "Bad compiler_t passed\n" );

	return NULL;
}

static const char* GetCompilerVersion( const compiler_t compiler ) {
	switch ( compiler ) {
		case COMPILER_DEFAULT:	return	NULL;
		case COMPILER_CLANG:	return	"20.1.5";
		case COMPILER_GCC:		return	"15.1.0";
		case COMPILER_MSVC:		return	"14.44.35207";
	}

	TEMPER_CHECK_TRUE_M( false, "Bad compiler_t passed\n" );

	return NULL;
}

#pragma clang diagnostic pop

struct buildTestGeneratedFiles_t {
	Array<const char*>	folders;
	Array<const char*>	files;
	Array<const char*>	fileExtensionsToCheck;
};

static void GetAllGeneratedFiles( const FileInfo* fileInfo, void* data ) {
	buildTestGeneratedFiles_t* generatedFiles = cast( buildTestGeneratedFiles_t*, data );

	if ( fileInfo->is_directory ) {
		generatedFiles->folders.add( fileInfo->full_filename );
	} else {
		For ( u32, fileExtensionIndex, 0, generatedFiles->fileExtensionsToCheck.count ) {
			if ( string_ends_with( fileInfo->full_filename, generatedFiles->fileExtensionsToCheck[fileExtensionIndex] ) ) {
				generatedFiles->files.add( fileInfo->full_filename );
			}
		}
	}
}

TEMPER_TEST_PARAMETRIC( TestBuild, TEMPER_FLAG_SHOULD_RUN, buildTest_t test ) {
	TEMPER_CHECK_TRUE_M( test.buildSourceFile, "A test MUST have its own build source file, otherwise Builder doesn't know what to build.\n" );
	TEMPER_CHECK_TRUE_M( test.rootDir, "A test MUST live in its own folder, you need to tell me what the \"root\" folder for this test is.\n" );

	auto DoBuildTest = []( buildTest_t testData, const char* buildSourceFilename ) {
		if ( !buildSourceFilename ) {
			buildSourceFilename = testData.buildSourceFile;
		}

		const char* buildSourceFileWithoutExtension = path_remove_file_extension( buildSourceFilename );

		// binary name doesnt have to be set by users, but we need it
		// this is the default
		if ( !testData.binaryName ) {
			testData.binaryName = buildSourceFileWithoutExtension;
		}

		s32 exitCode = 0;

		// test doing the actual build
		{
			// DM: 04/02/2026: this is stupid and ugly
			// args should also be const because we never actually modify them
			// I know how to get this done, leave this work with me
			Array<char*> args;
	#pragma clang diagnostic push
	#pragma clang diagnostic ignored "-Wcast-qual"
			args.add( cast( char*, buildSourceFilename ) );
	#pragma clang diagnostic pop
			if ( testData.config ) {
				args.add( tprintf( "--config=%s", testData.config ) );
			}

			exitCode = BuilderMain( 0, trunc_cast( int, args.count ), args.data );

			TEMPER_CHECK_TRUE_M( exitCode == 0, "BuilderMain() actually returned %d.\n", exitCode );
		}

		const char* fullBinaryName = NULL;
		if ( testData.binaryFolder ) {
			fullBinaryName = tprintf( "%s%c%s%s", testData.binaryFolder, PATH_SEPARATOR, testData.binaryName, GetFileExtensionFromBinaryType( BINARY_TYPE_EXE ) );
		} else {
			fullBinaryName = tprintf( "%s%s", testData.binaryName, GetFileExtensionFromBinaryType( BINARY_TYPE_EXE ) );
		}

		const char* dotBuilderFolder = ".builder";

		// get all the files that this test will generate
		// we will want these later to check if they got successfully deleted (tests should clean up after themselves properly)
		buildTestGeneratedFiles_t generatedFiles = {};
		generatedFiles.fileExtensionsToCheck.add( GetFileExtensionFromBinaryType( BINARY_TYPE_EXE ) );
		generatedFiles.fileExtensionsToCheck.add( GetFileExtensionFromBinaryType( BINARY_TYPE_DYNAMIC_LIBRARY ) );
		generatedFiles.fileExtensionsToCheck.add( GetFileExtensionFromBinaryType( BINARY_TYPE_STATIC_LIBRARY ) );
		generatedFiles.fileExtensionsToCheck.add( ".include_dependencies" );
		generatedFiles.fileExtensionsToCheck.add( ".pdb" );
		generatedFiles.fileExtensionsToCheck.add( ".exp" );
		generatedFiles.fileExtensionsToCheck.add( ".ilk" );
		generatedFiles.fileExtensionsToCheck.add( ".o" );
		generatedFiles.fileExtensionsToCheck.add( ".d" );

		TEMPER_CHECK_TRUE( file_get_all_files_in_folder( dotBuilderFolder, true, true, GetAllGeneratedFiles, &generatedFiles ) );
		if ( testData.binaryFolder ) {
			TEMPER_CHECK_TRUE( file_get_all_files_in_folder( testData.binaryFolder, true, true, GetAllGeneratedFiles, &generatedFiles ) );
		} else {
			// if there is no binary folder specified then binaries get made in the same folder as the build source file
			TEMPER_CHECK_TRUE( file_get_all_files_in_folder( "./", false, false, GetAllGeneratedFiles, &generatedFiles ) );

			// if there is no binary folder specified then an intermediate folder gets made in the same folder as the build source file
			TEMPER_CHECK_TRUE( file_get_all_files_in_folder( "intermediate", true, true, GetAllGeneratedFiles, &generatedFiles ) );
		}

		// we only care that certain files and folders got generated
		{
			TEMPER_CHECK_TRUE( file_exists( fullBinaryName ) );
			TEMPER_CHECK_TRUE( folder_exists( dotBuilderFolder ) );

			const char* userConfigBuildDLLFilename = tprintf( "%s%c%s%s", dotBuilderFolder, PATH_SEPARATOR, buildSourceFileWithoutExtension, GetFileExtensionFromBinaryType( BINARY_TYPE_DYNAMIC_LIBRARY ) );
			TEMPER_CHECK_TRUE( file_exists( userConfigBuildDLLFilename ) );
		}

		// now run the program we just built
		{
			Array<const char*> args;
			args.add( fullBinaryName );

			exitCode = RunProc( &args, NULL );

			TEMPER_CHECK_TRUE_M( exitCode == testData.expectedExitCode, "When trying to run \"%s\", the exit code was expected to be %d but it actually returned %d.\n", testData.binaryName, testData.expectedExitCode, exitCode );
		}

		// cleanup all generated files
		{
			For ( u32, fileIndex, 0, generatedFiles.files.count ) {
				const char* generatedFile = generatedFiles.files[fileIndex];

				TEMPER_CHECK_TRUE_M(  file_delete( generatedFile ), "Couldn't delete file \"%s\".\n", generatedFile );
				TEMPER_CHECK_TRUE_M( !file_exists( generatedFile ), "We deleted the file \"%s\" just now, but the OS tells us it still exists?\n", generatedFile );
			}

			For ( u32, folderIndex, 0, generatedFiles.folders.count ) {
				const char* generatedFolder = generatedFiles.folders[folderIndex];

				TEMPER_CHECK_TRUE_M(  folder_delete( generatedFolder ), "Couldn't delete folder \"%s\".\n", generatedFolder );
				TEMPER_CHECK_TRUE_M( !folder_exists( generatedFolder ), "We deleted the folder \"%s\" just now, but the OS tells us it still exists?\n", generatedFolder );
			}
		}
	};

	// move ourselves to the root folder of that test
	// run the test from that folder
	// then come back when were done
	const char* oldCWD = path_current_working_directory();
	TEMPER_CHECK_TRUE_M( path_set_current_directory( test.rootDir ), "Failed to cd into the test folder \"%s\".\n", test.rootDir );
	defer( TEMPER_CHECK_TRUE_M( path_set_current_directory( oldCWD ), "Failed to cd back out of the test folder.\n" ) );

	if ( test.defaultCompilerOnly ) {
		TEMPER_CHECK_TRUE( file_exists( test.buildSourceFile ) );

		DoBuildTest( test, NULL );
	} else {
		TEMPER_CHECK_TRUE_M( file_exists( "build_configs.cpp" ), "This test is building for all the compilers, which means you must have a \"build_config.cpp\" file that holds all your build configs inside a function called \"get_build_configs()\".\n" );

		For ( u32, compilerIndex, 0, COMPILER_COUNT ) {
			compiler_t compiler = cast( compiler_t, compilerIndex );

			const char* compilerSuffix = GetCompilerBuildSourceFileSuffix( compiler );

			const char* filename = tprintf( "build_%s.cpp", compilerSuffix );

			const char* compilerPath = GetCompilerPath( compiler );
			const char* compilerVersion = GetCompilerVersion( compiler );

			printf( "Building for compiler: %s\n", compilerSuffix );

			StringBuilder sb = {};
			string_builder_reset( &sb );
			string_builder_appendf( &sb, "// this file was auto generated\n" );
			string_builder_appendf( &sb, "// do not edit\n" );
			string_builder_appendf( &sb, "\n" );
			string_builder_appendf( &sb, "#include <builder.h>\n" );
			string_builder_appendf( &sb, "\n" );
			string_builder_appendf( &sb, "#include \"build_configs.cpp\"\n" );
			string_builder_appendf( &sb, "\n" );
			string_builder_appendf( &sb, "BUILDER_CALLBACK void set_builder_options( BuilderOptions* options ) {\n" );
			if ( compilerPath ) {
				string_builder_appendf( &sb, "\toptions->compiler_path = \"%s\";\n", compilerPath );
			}
			if ( compilerVersion ) {
				string_builder_appendf( &sb, "\toptions->compiler_version = \"%s\";\n", compilerVersion );
			}
			string_builder_appendf( &sb, "\tget_build_configs( options );\n" );
			string_builder_appendf( &sb, "}\n" );

			const char* buildSourceFileData = string_builder_to_string( &sb );

			TEMPER_CHECK_TRUE( file_write_entire( filename, buildSourceFileData, strlen( buildSourceFileData ) ) );

			DoBuildTest( test, filename );
		}
	}
}

TEMPER_INVOKE_PARAMETRIC_TEST( TestBuild, {
	.rootDir				= "test_basic",
	.buildSourceFile		= "test_basic.cpp",
	.defaultCompilerOnly	= true,
} );

TEMPER_INVOKE_PARAMETRIC_TEST( TestBuild, {
	.rootDir				= "test_set_builder_options",
	.buildSourceFile		= "test_set_builder_options.cpp",
	.config					= "debug",
	.binaryFolder			= "bin/debug",
	.binaryName				= "kenneth",
	.expectedExitCode		= 420,
	.defaultCompilerOnly	= true,
} );

TEMPER_INVOKE_PARAMETRIC_TEST( TestBuild, {
	.rootDir				= "test_set_builder_options",
	.buildSourceFile		= "test_set_builder_options.cpp",
	.config					= "release",
	.binaryFolder			= "bin/release",
	.binaryName				= "kenneth",
	.expectedExitCode		= 420,
	.noSymbolFiles			= true,
	.defaultCompilerOnly	= true,
} );

TEMPER_INVOKE_PARAMETRIC_TEST( TestBuild, {
	.rootDir			= "test_multiple_source_files",
	.binaryFolder		= "bin",
	.binaryName			= "marco_polo",
} );

TEMPER_INVOKE_PARAMETRIC_TEST( TestBuild, {
	.rootDir			= "test_static_lib",
	.config				= "program",
	.binaryFolder		= "bin",
	.binaryName			= "test_static_library_program",
} );

TEMPER_INVOKE_PARAMETRIC_TEST( TestBuild, {
	.rootDir			= "test_dynamic_lib",
	.config				= "program",
	.binaryFolder		= "bin",
	.binaryName			= "test_dynamic_library_program",
} );

TEMPER_TEST( GenerateVisualStudioSolution, TEMPER_FLAG_SHOULD_RUN ) {
	s32 exitCode = -1;

	// need to find where msbuild lives on windows
#ifdef _WIN32
	std::string msbuildInstallationPath;

	// detect where msbuild is stored
	{
		String vswhereStdout;

		Array<const char*> args;
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
		auto ParseTagString = []( const char* fileBuffer, const char* tag, std::string& outString ) -> bool8 {
			const char* lineStart = strstr( fileBuffer, tag );
			if ( !lineStart ) {
				return false;
			}

			lineStart += strlen( tag );

			while ( *lineStart == ' ' ) {
				lineStart++;
			}

			const char* lineEnd = NULL;
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
		Array<char*> args;
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wcast-qual"
		args.add( cast( char*, "test_generate_visual_studio_files/generate_solution.cpp" ) );
#pragma clang diagnostic pop

		exitCode = BuilderMain( 0, trunc_cast( int, args.count ), args.data );

		TEMPER_CHECK_TRUE_M( exitCode == 0, "Exit code actually returned %d.\n", exitCode );
	}

	// DM: apparently msbuild isnt properly supported on linux so this isnt possible
	// I'm not convinced by that answer, but all of my reading says so
#ifdef _WIN32
	// build the app project in the solution via msbuild
	if ( exitCode == 0 ) {
		String msbuildStdout;

		Array<const char*> args;
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
		Array<const char*> args;
		args.add( "test_generate_visual_studio_files/bin/debug/the-app.exe" );
		exitCode = RunProc( &args, NULL );

		TEMPER_CHECK_TRUE_M( exitCode == 69420, "Exit code actually returned %d.\n", exitCode );
	}
#endif // _WIN32
}

int main( int argc, char** argv ) {
	core_init( MEM_KILOBYTES( 64 ) );
	defer( core_shutdown() );

	TEMPER_RUN( argc, argv );

	int exitCode = TEMPER_GET_EXIT_CODE();

	return exitCode;
}
