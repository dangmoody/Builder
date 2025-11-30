#include "../include/builder.h"

#include "../src/core/src/core.suc.cpp"

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

static s32 RunProc( Array<const char*>* args, String* outStdout = NULL, const bool8 showStdout = false ) {
	assert( args );
	assert( args->count > 0 );

	/*For ( u64, argIndex, 0, args->count ) {
		printf( "%s ", ( *args )[argIndex] );
	}
	printf( "\n" );*/

	Process* process = process_create( args, NULL, /*PROCESS_FLAG_ASYNC | PROCESS_FLAG_COMBINE_STDOUT_AND_STDERR*/0 );

	if ( !process ) {
		// DM: 20/07/2025: I'm not 100% sure that its totally ok to have -1 as our own special exit code to mean that the process couldnt be found
		// its totally possible for other processes to return -1 and have it mean something else
		// the interpretation of the exit code of the processes we run is the responsibility of the calling code and were probably making a lot of assumptions there
		return -1;
	}

	TEMPER_CHECK_TRUE_M( process, "Failed to run process \"%s\".  Did you type the path correctly?\n", ( *args )[0] );

	defer( process_destroy( process ) );

	StringBuilder sb = {};
	string_builder_reset( &sb );

	char buffer[1024] = {};
	while ( process_read_stdout( process, buffer, count_of( buffer ) ) ) {
		string_builder_appendf( &sb, buffer );

		if ( showStdout ) {
			printf( "%s", buffer );
		}
	}

	if ( outStdout ) {
		*outStdout = string_builder_to_string( &sb );
	}

	s32 exitCode = process_join( process );

	return exitCode;
}

static void DoBinaryFilesPostCheck( const char* testName, const char* buildSourceFile ) {
	const char* buildSourceFileNoExtension = path_remove_file_extension( buildSourceFile );

	TEMPER_CHECK_TRUE( folder_exists( tprintf( "tests/%s/.builder", testName ) ) );
	TEMPER_CHECK_TRUE( file_exists(    tprintf( "tests/%s/.builder/%s%s", testName, buildSourceFileNoExtension, GetFileExtensionFromBinaryType( BINARY_TYPE_DYNAMIC_LIBRARY ) ) ) );

#ifdef _WIN32
	//TEMPER_CHECK_TRUE( file_exists(    tprintf( "tests/%s/.builder/%s.exp", testName, buildSourceFileNoExtension ) ) );	// optional
	TEMPER_CHECK_TRUE( file_exists(    tprintf( "tests/%s/.builder/%s.ilk", testName, buildSourceFileNoExtension ) ) );
	//TEMPER_CHECK_TRUE( file_exists(    tprintf( "tests/%s/.builder/%s.lib", testName, buildSourceFileNoExtension ) ) );	// optional
	TEMPER_CHECK_TRUE( file_exists(    tprintf( "tests/%s/.builder/%s.pdb", testName, buildSourceFileNoExtension ) ) );
#endif
}

TEMPER_TEST( Compile_Basic, TEMPER_FLAG_SHOULD_RUN ) {
	const char* sourceFile = "tests/test_basic/test_basic.cpp";
	const char* binaryFilename = tprintf( "tests/test_basic/test_basic%s", GetFileExtensionFromBinaryType( BINARY_TYPE_EXE ) );

	TEMPER_CHECK_TRUE( file_exists( sourceFile ) );

	Array<const char*> args;
	args.add( BUILDER_EXE_PATH );
	args.add( sourceFile );

	s32 exitCode = RunProc( &args );

	TEMPER_CHECK_TRUE_M( exitCode == 0, "Exit code actually returned %d.\n", exitCode );

	// now run the program we just built
	{
		TEMPER_CHECK_TRUE( file_exists( binaryFilename ) );

#ifdef _WIN32
		TEMPER_CHECK_TRUE( file_exists( "tests/test_basic/test_basic.pdb" ) );
		TEMPER_CHECK_TRUE( file_exists( "tests/test_basic/test_basic.ilk" ) );
#endif
	}

	DoBinaryFilesPostCheck( "test_basic", "test_basic.cpp" );
}

TEMPER_TEST_PARAMETRIC( Compile_SetBuilderOptions, TEMPER_FLAG_SHOULD_RUN, const char* config ) {
	const char* sourceFile = "tests/test_set_builder_options/test_set_builder_options.cpp";
	const char* binaryFilename = tprintf( "tests/test_set_builder_options/bin/%s/kenneth%s", config, GetFileExtensionFromBinaryType( BINARY_TYPE_EXE ) );

	TEMPER_CHECK_TRUE( file_exists( sourceFile ) );

	Array<const char*> args;
	args.add( BUILDER_EXE_PATH );
	args.add( sourceFile );
	args.add( tprintf( "--config=%s", config ) );

	s32 exitCode = RunProc( &args );
	TEMPER_CHECK_TRUE_M( exitCode == 0, "Exit code actually returned %d.\n", exitCode );

	TEMPER_CHECK_TRUE( folder_exists( tprintf( "tests/test_set_builder_options/bin/%s", config ) ) );
	TEMPER_CHECK_TRUE( file_exists( binaryFilename ) );

#ifdef _WIN32
	if ( string_equals( config, "debug" ) ) {
		TEMPER_CHECK_TRUE( file_exists( tprintf( "tests/test_set_builder_options/bin/%s/kenneth.pdb", config ) ) );
		TEMPER_CHECK_TRUE( file_exists( tprintf( "tests/test_set_builder_options/bin/%s/kenneth.ilk", config ) ) );
	} else {
		TEMPER_CHECK_TRUE( !file_exists( tprintf( "tests/test_set_builder_options/bin/%s/kenneth.pdb", config ) ) );
		TEMPER_CHECK_TRUE( !file_exists( tprintf( "tests/test_set_builder_options/bin/%s/kenneth.ilk", config ) ) );
	}
#endif

	DoBinaryFilesPostCheck( "test_set_builder_options", "test_set_builder_options.cpp" );
}

TEMPER_INVOKE_PARAMETRIC_TEST( Compile_SetBuilderOptions, "release" );
TEMPER_INVOKE_PARAMETRIC_TEST( Compile_SetBuilderOptions, "debug" );

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

	const char* buildSourceFile = tprintf( "tests/test_override_path_%s/test_override_path_%s.cpp", compilerName, compilerName );

	TEMPER_CHECK_TRUE( file_exists( buildSourceFile ) );

	s32 exitCode = 0;

	Array<const char*> args;

	// compile the program
	{
		args.reset();
		args.add( BUILDER_EXE_PATH );
		args.add( buildSourceFile );
		exitCode = RunProc( &args );

		TEMPER_CHECK_TRUE_M( exitCode == 0, "Exit code actually returned %d.\n", exitCode );

		TEMPER_CHECK_TRUE( file_exists( tprintf( "tests/test_override_path_%s/test_override_path_%s%s", compilerName, compilerName, GetFileExtensionFromBinaryType( BINARY_TYPE_EXE ) ) ) );
	}

	// run the program
	{
		args.reset();
		args.add( tprintf( "tests/test_override_path_%s/test_override_path_%s%s", compilerName, compilerName, GetFileExtensionFromBinaryType( BINARY_TYPE_EXE ) ) );
		exitCode = RunProc( &args );

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
		Array<const char*> args;
		args.add( BUILDER_EXE_PATH );
		args.add( buildSourceFile );

		s32 exitCode = RunProc( &args );

		TEMPER_CHECK_TRUE_M( exitCode == 0, "Exit code actually returned %d.\n", exitCode );

		TEMPER_CHECK_TRUE( file_exists( binaryFilename ) );
	}

	// run the program
	{
		// TODO(DM): this
	}

	DoBinaryFilesPostCheck( "test_multiple_source_files", "build.cpp" );
}

/*TEMPER_TEST( SetThirdPartyLibrariesViaSetBuilderOptions, TEMPER_FLAG_SHOULD_RUN ) {
	const char* buildSourceFile = "tests/test_third_party_libraries/build.cpp";
	const char* binaryFilename = tprintf( "tests/test_third_party_libraries/bin/sdl_test%s", GetFileExtensionFromBinaryType( BINARY_TYPE_EXE ) );

	TEMPER_CHECK_TRUE( file_exists( buildSourceFile ) );

	// compile the program
	{
		Array<const char*> args;
		args.add( BUILDER_EXE_PATH );
		args.add( buildSourceFile );

		s32 exitCode = RunProc( &args );

		TEMPER_CHECK_TRUE_M( exitCode == 0, "Exit code actually returned %d.\n", exitCode );
	}

	TEMPER_CHECK_TRUE( file_exists( binaryFilename ) );

	DoBinaryFilesPostCheck( "test_third_party_libraries", "build.cpp" );

	// run the program
	{
		Array<const char*> args;
		args.add( binaryFilename );

		s32 testProgramExitCode = RunProc( &args );

		TEMPER_CHECK_TRUE( testProgramExitCode == 0 );
	}
}*/

TEMPER_TEST( Compile_StaticLibrary, TEMPER_FLAG_SHOULD_RUN ) {
	// now build the exe that uses it
	{
		Array<const char*> args;
		args.add( BUILDER_EXE_PATH );
		args.add( "tests/test_static_lib/build.cpp" );
		args.add( "--config=program" );
		s32 exitCode = RunProc( &args );

		TEMPER_CHECK_TRUE_M( exitCode == 0, "Exit code actually returned %d.\n", exitCode );

		DoBinaryFilesPostCheck( "test_static_lib", "build.cpp" );
	}

	// run the program to make sure everything actually works
	{
		Array<const char*> args;
		args.add( tprintf( "tests/test_static_lib/bin/test_static_library_program%s", GetFileExtensionFromBinaryType( BINARY_TYPE_EXE ) ) );
		s32 exitCode = RunProc( &args );

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
		Array<const char*> args;
		args.add( BUILDER_EXE_PATH );
		args.add( "tests/test_dynamic_lib/build.cpp" );
		args.add( "--config=program" );

		s32 buildExitCode = RunProc( &args );

		TEMPER_CHECK_TRUE_M( buildExitCode == 0, "Exit code actually returned %d.\n", buildExitCode );

		TEMPER_CHECK_TRUE( file_exists( dynamicLibraryFilename ) );
		TEMPER_CHECK_TRUE( file_exists( exeFilename ) );

		DoBinaryFilesPostCheck( "test_dynamic_lib", "build.cpp" );
	}

	// run the program to make sure everything actually works
	{
		Array<const char*> args;
		args.add( exeFilename );

		s32 runProgramExitCode = RunProc( &args );

		TEMPER_CHECK_TRUE_M( runProgramExitCode == 0, "Exit code actually returned %d.\n", runProgramExitCode );
	}
}

// TODO(DM): this!
TEMPER_TEST( RebuildSkipping, TEMPER_FLAG_SHOULD_SKIP ) {
}

TEMPER_TEST( GenerateVisualStudioSolution, TEMPER_FLAG_SHOULD_RUN ) {
	Array<const char*> args;
	
	// need to find where msbuild lives on windows
#ifdef _WIN32
	std::string msbuildInstallationPath;

	// detect where msbuild is stored
	{
		String vswhereStdout;

		args.reset();
		args.add( "C:\\Program Files (x86)\\Microsoft Visual Studio\\Installer\\vswhere.exe" );
		args.add( "-latest" );
		args.add( "-products" );
		args.add( "*" );
		args.add( "-requires" );
		args.add( "Microsoft.Component.MSBuild" );
		s32 exitCode = RunProc( &args, &vswhereStdout, true );

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
		args.reset();
		args.add( BUILDER_EXE_PATH );
		args.add( "tests/test_generate_visual_studio_files/generate_solution.cpp" );

		s32 exitCode = RunProc( &args );

		TEMPER_CHECK_TRUE_M( exitCode == 0, "Exit code actually returned %d.\n", exitCode );
	}

	// DM: apparently MSBuild isnt properly supported on linux so this isnt possible
	// I'm not convinced by that answer, but all of my reading says so
#ifdef _WIN32
	// build the app project in the solution via MSBuild
	{
		args.reset();
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
		args.reset();
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
