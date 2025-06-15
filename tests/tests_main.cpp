#include "../src/core/src/core.suc.cpp"

#ifdef __linux__
//#include <assert.h>
#endif // __linux__

#define TEMPERDEV_ASSERT assert
#define TEMPER_IMPLEMENTATION
#include "temper/temper.h"

// TODO (MY) - Ensure that we remove this and add an appropriate system agnostic null file handle later.
#ifdef _WIN32
#define DEAD_HANDLE INVALID_HANDLE_VALUE
#elif defined(__linux__)
#define DEAD_HANDLE ((void*)-1)
#endif

static bool8 FileExists( const char* filename ) {
	FileInfo fileInfo;
	File file = file_find_first( filename, &fileInfo );

	return file.ptr != DEAD_HANDLE;
}

static s32 RunProc( Array<const char*>* args ) {
	Process* process = process_create( args, NULL );
	defer( process_destroy( process ) );

	{
		char buffer[1024];
		while ( process_read_stdout( process, buffer, count_of( buffer ) ) ) {
			printf( "%s", buffer );
		}
	}

	s32 exitCode = process_join( process );

	return exitCode;
}

static void DoBuildInfoPreTest( const char* buildInfoFilename ) {
	if ( FileExists( buildInfoFilename ) ) {
		file_delete( buildInfoFilename );
	}

	TEMPER_CHECK_TRUE( !FileExists( buildInfoFilename ) );
}

static void DoBuildInfoPostTest( const char* testName, const char* buildSourceFile ) {
	const char* buildSourceFileNoExtension = path_remove_file_extension( buildSourceFile );

	TEMPER_CHECK_TRUE( folder_exists( tprintf( "tests\\%s\\.builder", testName ) ) );
	TEMPER_CHECK_TRUE( FileExists(    tprintf( "tests\\%s\\.builder\\%s.build_info", testName, buildSourceFileNoExtension ) ) );
	TEMPER_CHECK_TRUE( FileExists(    tprintf( "tests\\%s\\.builder\\%s.dll", testName, buildSourceFileNoExtension ) ) );
	//TEMPER_CHECK_TRUE( FileExists(    tprintf( "tests\\%s\\.builder\\%s.exp", testName, buildSourceFileNoExtension ) ) );	// optional
	TEMPER_CHECK_TRUE( FileExists(    tprintf( "tests\\%s\\.builder\\%s.ilk", testName, buildSourceFileNoExtension ) ) );
	//TEMPER_CHECK_TRUE( FileExists(    tprintf( "tests\\%s\\.builder\\%s.lib", testName, buildSourceFileNoExtension ) ) );	// optional
	TEMPER_CHECK_TRUE( FileExists(    tprintf( "tests\\%s\\.builder\\%s.pdb", testName, buildSourceFileNoExtension ) ) );
}

TEMPER_TEST( Compile_Basic, TEMPER_FLAG_SHOULD_RUN ) {
	DoBuildInfoPreTest( "tests\\test_basic\\.builder\\test_basic.build_info" );

	const char* sourceFile = "tests\\test_basic\\test_basic.cpp";

	TEMPER_CHECK_TRUE( FileExists( sourceFile ) );

	Array<const char*> args;
	args.add( "builder.exe" );
	args.add( sourceFile );

	s32 exitCode = RunProc( &args );

	TEMPER_CHECK_TRUE_M( exitCode == 0, "Exit code actually returned %d.\n", exitCode );

	TEMPER_CHECK_TRUE( FileExists( "tests\\test_basic\\test_basic.exe" ) );
	TEMPER_CHECK_TRUE( FileExists( "tests\\test_basic\\test_basic.pdb" ) );
	TEMPER_CHECK_TRUE( FileExists( "tests\\test_basic\\test_basic.ilk" ) );

	DoBuildInfoPostTest( "test_basic", "test_basic.cpp" );
}

TEMPER_TEST_PARAMETRIC( Compile_SetBuilderOptions, TEMPER_FLAG_SHOULD_RUN, const char* config ) {
	DoBuildInfoPreTest( "tests\\test_set_builder_options\\.builder\\test_set_builder_options.build_info" );

	const char* sourceFile = "tests\\test_set_builder_options\\test_set_builder_options.cpp";

	TEMPER_CHECK_TRUE( FileExists( sourceFile ) );

	Array<const char*> args;
	args.add( "builder.exe" );
	args.add( sourceFile );
	args.add( tprintf( "--config=%s", config ) );

	s32 exitCode = RunProc( &args );

	TEMPER_CHECK_TRUE_M( exitCode == 0, "Exit code actually returned %d.\n", exitCode );

	TEMPER_CHECK_TRUE( folder_exists( tprintf( "tests\\test_set_builder_options\\bin\\%s", config ) ) );
	TEMPER_CHECK_TRUE( FileExists( tprintf( "tests\\test_set_builder_options\\bin\\%s\\kenneth.exe", config ) ) );

	if ( string_equals( config, "debug" ) ) {
		TEMPER_CHECK_TRUE( FileExists( tprintf( "tests\\test_set_builder_options\\bin\\%s\\kenneth.pdb", config ) ) );
		TEMPER_CHECK_TRUE( FileExists( tprintf( "tests\\test_set_builder_options\\bin\\%s\\kenneth.ilk", config ) ) );
	} else {
		TEMPER_CHECK_TRUE( !FileExists( tprintf( "tests\\test_set_builder_options\\bin\\%s\\kenneth.pdb", config ) ) );
		TEMPER_CHECK_TRUE( !FileExists( tprintf( "tests\\test_set_builder_options\\bin\\%s\\kenneth.ilk", config ) ) );
	}

	DoBuildInfoPostTest( "test_set_builder_options", "test_set_builder_options.cpp" );
}

TEMPER_INVOKE_PARAMETRIC_TEST( Compile_SetBuilderOptions, "release" );
TEMPER_INVOKE_PARAMETRIC_TEST( Compile_SetBuilderOptions, "debug" );

TEMPER_TEST( Compile_MultipleSourceFiles, TEMPER_FLAG_SHOULD_RUN ) {
	DoBuildInfoPreTest( "tests\\test_multiple_source_files\\.builder\\build.build_info" );

	const char* buildSourceFile = "tests\\test_multiple_source_files\\build.cpp";

	TEMPER_CHECK_TRUE( FileExists( buildSourceFile ) );

	Array<const char*> args;
	args.add( "builder.exe" );
	args.add( buildSourceFile );

	s32 exitCode = RunProc( &args );

	TEMPER_CHECK_TRUE_M( exitCode == 0, "Exit code actually returned %d.\n", exitCode );

	TEMPER_CHECK_TRUE( FileExists( "tests\\test_multiple_source_files\\bin\\marco_polo.exe" ) );

	DoBuildInfoPostTest( "test_multiple_source_files", "build.cpp" );
}

TEMPER_TEST( SetThirdPartyLibrariesViaSetBuilderOptions, TEMPER_FLAG_SHOULD_RUN ) {
	DoBuildInfoPreTest( "tests\\test_third_party_libraries\\.builder\\build.build_info" );

	const char* buildSourceFile = "tests\\test_third_party_libraries\\build.cpp";

	TEMPER_CHECK_TRUE( FileExists( buildSourceFile ) );

	{
		Array<const char*> args;
		args.add( "builder.exe" );
		args.add( buildSourceFile );

		s32 exitCode = RunProc( &args );

		TEMPER_CHECK_TRUE_M( exitCode == 0, "Exit code actually returned %d.\n", exitCode );
	}

	TEMPER_CHECK_TRUE( FileExists( "tests\\test_third_party_libraries\\bin\\sdl_test.exe" ) );

	DoBuildInfoPostTest( "test_third_party_libraries", "build.cpp" );

	{
		Array<const char*> args;
		args.add( "tests\\test_third_party_libraries\\bin\\sdl_test.exe" );

		s32 testProgramExitCode = RunProc( &args );

		TEMPER_CHECK_TRUE( testProgramExitCode == 0 );
	}
}

TEMPER_TEST( Compile_StaticLibrary, TEMPER_FLAG_SHOULD_RUN ) {
	// now build the exe that uses it
	{
		DoBuildInfoPreTest( "tests\\test_static_lib\\.builder\\build.build_info" );

		Array<const char*> args;
		args.add( "builder.exe" );
		args.add( "tests\\test_static_lib\\build.cpp" );
		args.add( "--config=program" );

		s32 exitCode = RunProc( &args );

		TEMPER_CHECK_TRUE_M( exitCode == 0, "Exit code actually returned %d.\n", exitCode );

		DoBuildInfoPostTest( "test_static_lib", "build.cpp" );
	}

	// run the program to make sure everything actually works
	{
		Array<const char*> args;
		args.add( "tests\\test_static_lib\\bin\\test_static_library_program.exe" );

		s32 exitCode = RunProc( &args );

		TEMPER_CHECK_TRUE_M( exitCode == 0, "Exit code actually returned %d.\n", exitCode );
	}
}

TEMPER_TEST( Compile_DynamicLibrary, TEMPER_FLAG_SHOULD_RUN ) {
	const char* testDynamicLibDLLPath = "tests\\test_dynamic_lib\\bin\\test_dynamic_lib.dll";

	if ( FileExists( testDynamicLibDLLPath ) ) {
		file_delete( testDynamicLibDLLPath );
		TEMPER_CHECK_TRUE( !FileExists( testDynamicLibDLLPath ) );
	}

	// now build the exe
	{
		DoBuildInfoPreTest( "tests\\test_dynamic_lib\\.builder\\build.build_info" );

		Array<const char*> args;
		args.add( "builder.exe" );
		args.add( "tests\\test_dynamic_lib\\build.cpp" );
		args.add( "--config=program" );

		s32 exitCode = RunProc( &args );

		TEMPER_CHECK_TRUE_M( exitCode == 0, "Exit code actually returned %d.\n", exitCode );

		DoBuildInfoPostTest( "test_dynamic_lib", "build.cpp" );
	}

	// run the program to make sure everything actually works
	{
		Array<const char*> args;
		args.add( "tests\\test_dynamic_lib\\bin\\test_dynamic_library_program.exe" );

		s32 exitCode = RunProc( &args );

		TEMPER_CHECK_TRUE_M( exitCode == 0, "Exit code actually returned %d.\n", exitCode );

		TEMPER_CHECK_TRUE( FileExists( testDynamicLibDLLPath ) );
	}
}

TEMPER_TEST( RebuildSkipping, TEMPER_FLAG_SHOULD_RUN ) {

}

TEMPER_TEST( GenerateVisualStudioSolution, TEMPER_FLAG_SHOULD_RUN ) {
	// generate the solution
	{
		Array<const char*> args;
		args.add( "builder.exe" );
		args.add( "tests\\test_generate_visual_studio_files\\generate_solution.cpp" );

		s32 exitCode = RunProc( &args );

		TEMPER_CHECK_TRUE_M( exitCode == 0, "Exit code actually returned %d.\n", exitCode );
	}

	// build the app project in the solution via MSBuild
	{
		Array<const char*> args;
		args.add( "C:\\Program Files\\Microsoft Visual Studio\\2022\\Community\\MSBuild\\Current\\Bin\\MSBuild.exe" );	// TODO(DM): query for this instead
		args.add( "tests\\test_generate_visual_studio_files\\visual_studio\\app.vcxproj" );
		args.add( "/property:Platform=x64" );

		s32 exitCode = RunProc( &args );

		TEMPER_CHECK_TRUE_M( exitCode == 0, "Exit code actually returned %d.\n", exitCode );
	}

	// run the program, make sure it returns the correct exit code
	{
		Array<const char*> args;
		args.add( "tests\\test_generate_visual_studio_files\\bin\\debug\\app\\the-app.exe" );

		s32 exitCode = RunProc( &args );

		TEMPER_CHECK_TRUE_M( exitCode == 69420, "Exit code actually returned %d.\n", exitCode );
	}
}

int main( int argc, char** argv ) {
	core_init( MEM_KILOBYTES( 64 ) );
	defer( core_shutdown() );

	TEMPER_RUN( argc, argv );

	int exitCode = TEMPER_GET_EXIT_CODE();

	return exitCode;
}