#include "../src/builder_local.h"

#include <debug.h>
#include <array.inl>
#include <typecast.inl>
#include <paths.h>

#ifdef __linux__
//#include <assert.h>
#endif // __linux__

#define TEMPERDEV_ASSERT assert
#define TEMPER_IMPLEMENTATION
#include "../src/core/include/file.h"
#include "../src/core/include/string_helpers.h"
#include "temper/temper.h"

#if defined( _WIN32 )
	#if defined( _DEBUG )
		#define BUILDER_EXE_PATH "bin/builder_debug.exe"
	#elif defined( NDEBUG )
		#define BUILDER_EXE_PATH "bin/builder_release.exe"
	#endif
#elif defined( __linux__ )
	#if defined( _DEBUG )
		#define BUILDER_EXE_PATH "bin/builder_debug"
	#elif defined( NDEBUG )
		#define BUILDER_EXE_PATH "bin/builder_release"
	#endif
#endif

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

static void DoBinaryFilesPostCheck( const char* testName, const char* buildSourceFile ) {
	const char* buildSourceFileNoExtension = path_remove_file_extension( buildSourceFile );

	TEMPER_CHECK_TRUE( folder_exists( tprintf( "tests/%s/.builder", testName ) ) );
	TEMPER_CHECK_TRUE( file_exists(   tprintf( "tests/%s/.builder/%s%s", testName, buildSourceFileNoExtension, GetFileExtensionFromBinaryType( BINARY_TYPE_DYNAMIC_LIBRARY ) ) ) );

#ifdef _WIN32
	//TEMPER_CHECK_TRUE( file_exists(    tprintf( "tests/%s/.builder/%s.exp", testName, buildSourceFileNoExtension ) ) );	// optional
	TEMPER_CHECK_TRUE( file_exists(    tprintf( "tests/%s/.builder/%s.ilk", testName, buildSourceFileNoExtension ) ) );
	//TEMPER_CHECK_TRUE( file_exists(    tprintf( "tests/%s/.builder/%s.lib", testName, buildSourceFileNoExtension ) ) );	// optional
	TEMPER_CHECK_TRUE( file_exists(    tprintf( "tests/%s/.builder/%s.pdb", testName, buildSourceFileNoExtension ) ) );
#endif
}

struct buildTest_t {
	const char*	rootDir;		// whats the root folder of this test?
	const char*	buildSourceFile;
	const char*	config;			// can be NULL
	const char*	binaryFolder;	// if NULL, assumed that no folder was created as part of the build
	const char*	binaryName;		// if NULL, assumed to be the same as buildSourceFile except it ends with .exe (on windows)
	bool8		hasSymbols;
};

TEMPER_TEST_PARAMETRIC( TestBuild, TEMPER_FLAG_SHOULD_RUN, const buildTest_t& test ) {
	// move ourselves to the root folder of that test
	// run the test from that folder
	// then come back when were done
	TEMPER_CHECK_TRUE_M( test.rootDir, "Each test lives in its own folder, you need to tell me what the \"root\" folder for this test is.\n" );
	const char* oldCWD = path_current_working_directory();
	path_set_current_directory( test.rootDir );
	defer( path_set_current_directory( oldCWD ) );

	TEMPER_CHECK_TRUE( file_exists( test.buildSourceFile ) );

	// binary name doesnt have to be set by users, but we need it
	// this is the default
	const char* binaryFilename = test.binaryName;
	if ( !binaryFilename ) {
		const char* buildSourceFileNoExtension = path_remove_file_extension( test.buildSourceFile );

		binaryFilename = tprintf( "%s%s", buildSourceFileNoExtension, GetFileExtensionFromBinaryType( BINARY_TYPE_EXE ) );
	}

	s32 exitCode = 0;

	// test doing the actual build
	{
		// DM: 04/02/2026: this is stupid and ugly
		// args should also be const because we never actually modify them
		// I know how to get this done, leave this work with me
		Array<char*> args;
		args.add( cast( char*, test.buildSourceFile ) );
		if ( test.config ) {
			args.add( tprintf( "--config=%s", test.config ) );
		}

		exitCode = BuilderMain( 0, trunc_cast( int, args.count ), args.data );

		TEMPER_CHECK_TRUE_M( exitCode == 0, "Exit code actually returned %d.\n", exitCode );
	}

	const char* fullBinaryName = NULL;
	if ( test.binaryFolder ) {
		fullBinaryName = tprintf( "%s%c%s", test.binaryFolder, PATH_SEPARATOR, binaryFilename );
	} else {
		fullBinaryName = test.binaryName;
	}

	// check the binaries exist
	{
		if ( test.binaryFolder ) {
			TEMPER_CHECK_TRUE_M( file_exists( test.binaryFolder ), "Binary folder \"%s\" was expected, but it wasn't found.\n", test.binaryFolder );

			const char* intermediateFolder = tprintf( "%s%cintermediate", test.binaryFolder, PATH_SEPARATOR );
			TEMPER_CHECK_TRUE_M( file_exists( intermediateFolder ), "Intermediate folder \"%s\" was expected, but it wasn't found.\n", intermediateFolder );
		}

		TEMPER_CHECK_TRUE( file_exists( fullBinaryName ) );

		if ( test.hasSymbols ) {
			// windows generates PDBs and ILKs by default with symbols enabled
#ifdef _WIN32
			const char* pdbName = tprintf( "%s.pdb", test.binaryName );
			const char* ilkName = tprintf( "%s.ilk", test.binaryName );

			TEMPER_CHECK_TRUE_M( file_exists( pdbName ), "Symbol file \"%s\" was expected, but it wasn't found.\n", pdbName );
			TEMPER_CHECK_TRUE_M( file_exists( ilkName ), "Symbol file \"%s\" was expected, but it wasn't found.\n", ilkName );
#endif
		}
	}

	// now run the program we just built
	{
		Array<const char*> args;
		args.add( fullBinaryName );

		exitCode = RunProc( &args, NULL );

		TEMPER_CHECK_TRUE_M( exitCode == 0, "When trying to run \"%s\", the exit code actually returned %d.\n", test.binaryName, exitCode );
	}
}

TEMPER_INVOKE_PARAMETRIC_TEST( TestBuild, {
	.rootDir			= "tests/test_basic",
	.buildSourceFile	= "test_basic.cpp",
	.binaryName			= "test_basic",
	.hasSymbols			= true,
} );

TEMPER_INVOKE_PARAMETRIC_TEST( TestBuild, {
	.rootDir			= "tests/test_set_builder_options",
	.buildSourceFile	= "test_set_builder_options.cpp",
	.config				= "debug",
	.binaryFolder		= "bin/debug",
	.binaryName			= "kenneth",
} );

TEMPER_INVOKE_PARAMETRIC_TEST( TestBuild, {
	.rootDir			= "tests/test_set_builder_options",
	.buildSourceFile	= "test_set_builder_options.cpp",
	.config				= "release",
	.binaryFolder		= "bin/release",
	.binaryName			= "kenneth",
} );

enum compiler_t {
	COMPILER_CLANG	= 0,
	COMPILER_GCC,
	COMPILER_MSVC,
};

TEMPER_TEST_PARAMETRIC( SetCompilerPath, TEMPER_FLAG_SHOULD_RUN, const compiler_t compiler ) {
	auto GetCompilerName = []( const compiler_t compilerType ) -> const char* {
		switch ( compilerType ) {
			case COMPILER_CLANG:	return "clang";
			case COMPILER_GCC:		return "gcc";
			case COMPILER_MSVC:		return "msvc";
		}
	};

	const char* compilerName = GetCompilerName( compiler );

	printf( "Overriding default Builder compiler for %s\n", compilerName );

	char* buildSourceFile = tprintf( "tests/test_override_path_%s/test_override_path_%s.cpp", compilerName, compilerName );

	TEMPER_CHECK_TRUE( file_exists( buildSourceFile ) );

	s32 exitCode = 0;

	// compile the program
	{
		Array<char*> args;
		args.add( buildSourceFile );
		exitCode = BuilderMain( 0, trunc_cast( int, args.count ), args.data );

		TEMPER_CHECK_TRUE_M( exitCode == 0, "Exit code actually returned %d.\n", exitCode );

		TEMPER_CHECK_TRUE( file_exists( tprintf( "tests/test_override_path_%s/test_override_path_%s%s", compilerName, compilerName, GetFileExtensionFromBinaryType( BINARY_TYPE_EXE ) ) ) );
	}

	// run the program
	{
		Array<const char*> args;
		args.add( tprintf( "tests/test_override_path_%s/test_override_path_%s%s", compilerName, compilerName, GetFileExtensionFromBinaryType( BINARY_TYPE_EXE ) ) );
		exitCode = RunProc( &args, NULL );

		TEMPER_CHECK_TRUE_M( exitCode == 0, "Exit code actually returned %d.\n", exitCode );
	}

	DoBinaryFilesPostCheck( tprintf( "test_override_path_%s", compilerName ), tprintf( "test_override_path_%s.cpp", compilerName ) );
}

TEMPER_INVOKE_PARAMETRIC_TEST( SetCompilerPath, COMPILER_CLANG );
#ifdef _WIN32
TEMPER_INVOKE_PARAMETRIC_TEST( SetCompilerPath, COMPILER_GCC );
TEMPER_INVOKE_PARAMETRIC_TEST( SetCompilerPath, COMPILER_MSVC );
#endif

TEMPER_TEST( Compile_MultipleSourceFiles, TEMPER_FLAG_SHOULD_RUN ) {
	const char* buildSourceFile = "tests/test_multiple_source_files/build.cpp";
	const char* binaryFilename = tprintf( "tests/test_multiple_source_files/bin/marco_polo%s", GetFileExtensionFromBinaryType( BINARY_TYPE_EXE ) );

	TEMPER_CHECK_TRUE( file_exists( buildSourceFile ) );

	// compile the program
	{
		Array<char*> args;
		args.add( cast( char*, buildSourceFile ) );

		s32 exitCode = BuilderMain( 0, trunc_cast( int, args.count ), args.data );

		TEMPER_CHECK_TRUE_M( exitCode == 0, "Exit code actually returned %d.\n", exitCode );

		TEMPER_CHECK_TRUE( file_exists( binaryFilename ) );
	}

	// run the program
	{
		// TODO(DM): this
	}

	DoBinaryFilesPostCheck( "test_multiple_source_files", "build.cpp" );
}

TEMPER_TEST( Compile_StaticLibrary, TEMPER_FLAG_SHOULD_RUN ) {
	// now build the exe that uses it
	{
		Array<char*> args;
		args.add( cast( char*, "tests/test_static_lib/build.cpp" ) );
		args.add( cast( char*, "--config=program" ) );
		s32 exitCode = BuilderMain( 0, trunc_cast( int, args.count ), args.data );

		TEMPER_CHECK_TRUE_M( exitCode == 0, "Exit code actually returned %d.\n", exitCode );

		DoBinaryFilesPostCheck( "test_static_lib", "build.cpp" );
	}

	// run the program to make sure everything actually works
	{
		Array<const char*> args;
		args.add( tprintf( "tests/test_static_lib/bin/test_static_library_program%s", GetFileExtensionFromBinaryType( BINARY_TYPE_EXE ) ) );
		s32 exitCode = RunProc( &args, NULL );

		TEMPER_CHECK_TRUE_M( exitCode == 0, "Exit code actually returned %d.\n", exitCode );
	}
}

TEMPER_TEST( Compile_DynamicLibrary, TEMPER_FLAG_SHOULD_RUN ) {
#if defined( _WIN32 )
	const char* dynamicLibraryFilename = tprintf( "tests/test_dynamic_lib/bin/test_dynamic_lib%s", GetFileExtensionFromBinaryType( BINARY_TYPE_DYNAMIC_LIBRARY ) );
#elif defined( __linux__ )
	const char* dynamicLibraryFilename = tprintf( "tests/test_dynamic_lib/bin/libtest_dynamic_lib%s", GetFileExtensionFromBinaryType( BINARY_TYPE_DYNAMIC_LIBRARY ) );
#endif

	const char* exeFilename = tprintf( "tests/test_dynamic_lib/bin/test_dynamic_library_program%s", GetFileExtensionFromBinaryType( BINARY_TYPE_EXE ) );

	if ( file_exists( dynamicLibraryFilename ) ) {
		file_delete( dynamicLibraryFilename );
		TEMPER_CHECK_TRUE( !file_exists( dynamicLibraryFilename ) );
	}

	if ( file_exists( exeFilename ) ) {
		file_delete( exeFilename );
		TEMPER_CHECK_TRUE( !file_exists( exeFilename ) );
	}

	// now build the exe
	{
		Array<char*> args;
		args.add( cast( char*, "tests/test_dynamic_lib/build.cpp" ) );
		args.add( cast( char*, "--config=program" ) );

		s32 buildExitCode = BuilderMain( 0, trunc_cast( int, args.count ), args.data );

		TEMPER_CHECK_TRUE_M( buildExitCode == 0, "Exit code actually returned %d.\n", buildExitCode );

		TEMPER_CHECK_TRUE( file_exists( dynamicLibraryFilename ) );
		TEMPER_CHECK_TRUE( file_exists( exeFilename ) );

		DoBinaryFilesPostCheck( "test_dynamic_lib", "build.cpp" );
	}

	// run the program to make sure everything actually works
	{
		Array<const char*> args;
		args.add( exeFilename );

		s32 runProgramExitCode = RunProc( &args, NULL );

		TEMPER_CHECK_TRUE_M( runProgramExitCode == 0, "Exit code actually returned %d.\n", runProgramExitCode );
	}
}

TEMPER_TEST( GenerateVisualStudioSolution, TEMPER_FLAG_SHOULD_RUN ) {
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
		s32 exitCode = RunProc( &args, &vswhereStdout );

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
	{
		Array<char*> args;
		args.add( cast( char*, "tests/test_generate_visual_studio_files/generate_solution.cpp" ) );

		s32 exitCode = BuilderMain( 0, trunc_cast( int, args.count ), args.data );

		TEMPER_CHECK_TRUE_M( exitCode == 0, "Exit code actually returned %d.\n", exitCode );
	}

	// DM: apparently MSBuild isnt properly supported on linux so this isnt possible
	// I'm not convinced by that answer, but all of my reading says so
#ifdef _WIN32
	// build the app project in the solution via MSBuild
	{
		Array<const char*> args;
#if defined( _WIN32 )
		args.add( tprintf( "%s/MSBuild/Current/Bin/MSBuild.exe", msbuildInstallationPath.c_str() ) );	// TODO(DM): query for this instead
#elif defined( __linux__ )
		args.add( "msbuild" );
#endif
		args.add( "tests/test_generate_visual_studio_files/visual_studio/app.vcxproj" );
		args.add( "/property:Platform=x64" );
		s32 exitCode = RunProc( &args );

		TEMPER_CHECK_TRUE_M( exitCode == 0, "Exit code actually returned %d.\n", exitCode );
	}

	// run the program, make sure it returns the correct exit code
	{
		Array<const char*> args;
		args.add( "tests/test_generate_visual_studio_files/bin/debug/the-app.exe" );
		s32 exitCode = RunProc( &args );

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
