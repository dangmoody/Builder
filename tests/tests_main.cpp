#include "../src/core/core.cpp"

#define TEMPERDEV_ASSERT assert
#define TEMPER_IMPLEMENTATION
#include "temper/temper.h"

static bool8 FileExists( const char* filename ) {
	FileInfo fileInfo;
	File file = file_find_first( filename, &fileInfo );

	return file.ptr != INVALID_HANDLE_VALUE;
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

	s32 exitCode = process_join( &process );

	return exitCode;
}

static void DoBuildInfoPreTest( const char* buildInfoFilename ) {
	if ( FileExists( buildInfoFilename ) ) {
		file_delete( buildInfoFilename );
	}

	TEMPER_CHECK_TRUE( !FileExists( buildInfoFilename ) );
}

static void DoBuildInfoPostTest( const char* testName, const char* buildSourceFile ) {
	const char* buildSourceFileNoExtension = paths_remove_file_extension( buildSourceFile );

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
	array_add( &args, "builder.exe" );
	array_add( &args, sourceFile );

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
	array_add( &args, "builder.exe" );
	array_add( &args, sourceFile );
	array_add( &args, tprintf( "--config=%s", config ) );

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
	array_add( &args, "builder.exe" );
	array_add( &args, buildSourceFile );

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
		array_add( &args, "builder.exe" );
		array_add( &args, buildSourceFile );

		s32 exitCode = RunProc( &args );

		TEMPER_CHECK_TRUE_M( exitCode == 0, "Exit code actually returned %d.\n", exitCode );
	}

	TEMPER_CHECK_TRUE( FileExists( "tests\\test_third_party_libraries\\bin\\sdl_test.exe" ) );

	DoBuildInfoPostTest( "test_third_party_libraries", "build.cpp" );

	{
		Array<const char*> args;
		array_add( &args, "tests\\test_third_party_libraries\\bin\\sdl_test.exe" );

		s32 testProgramExitCode = RunProc( &args );

		TEMPER_CHECK_TRUE( testProgramExitCode == 0 );
	}
}

TEMPER_TEST( Compile_StaticLibrary, TEMPER_FLAG_SHOULD_RUN ) {
	// build the static library itself
	{
		DoBuildInfoPreTest( "tests\\test_static_lib\\lib\\.builder\\lib.build_info" );

		Array<const char*> args;
		array_add( &args, "builder.exe" );
		array_add( &args, "tests\\test_static_lib\\lib\\lib.cpp" );

		s32 exitCode = RunProc( &args );

		TEMPER_CHECK_TRUE_M( exitCode == 0, "Exit code actually returned %d.\n", exitCode );

		DoBuildInfoPostTest( "test_static_lib\\lib", "lib.cpp" );
	}

	// now build the exe that uses it
	{
		DoBuildInfoPreTest( "tests\\test_static_lib\\program\\.builder\\build.build_info" );

		Array<const char*> args;
		array_add( &args, "builder.exe" );
		array_add( &args, "tests\\test_static_lib\\program\\build.cpp" );

		s32 exitCode = RunProc( &args );

		TEMPER_CHECK_TRUE_M( exitCode == 0, "Exit code actually returned %d.\n", exitCode );

		DoBuildInfoPostTest( "test_static_lib\\program", "build.cpp" );
	}

	// run the program to make sure everything actually works
	{
		Array<const char*> args;
		array_add( &args, "tests\\test_static_lib\\program\\bin\\test_static_library_program.exe" );

		s32 exitCode = RunProc( &args );

		TEMPER_CHECK_TRUE_M( exitCode == 0, "Exit code actually returned %d.\n", exitCode );
	}
}

TEMPER_TEST( Compile_DynamicLibrary, TEMPER_FLAG_SHOULD_RUN ) {
	const char* testDynamicLibDLLPath = "tests\\test_dynamic_lib\\program\\bin\\test_dynamic_lib.dll";

	if ( FileExists( testDynamicLibDLLPath ) ) {
		file_delete( testDynamicLibDLLPath );
		TEMPER_CHECK_TRUE( !FileExists( testDynamicLibDLLPath ) );
	}

	// build the dynamic library itself
	{
		DoBuildInfoPreTest( "tests\\test_dynamic_lib\\lib\\.builder\\build.build_info" );

		Array<const char*> args;
		array_add( &args, "builder.exe" );
		array_add( &args, "tests\\test_dynamic_lib\\lib\\build.cpp" );

		s32 exitCode = RunProc( &args );

		TEMPER_CHECK_TRUE_M( exitCode == 0, "Exit code actually returned %d.\n", exitCode );

		DoBuildInfoPostTest( "test_dynamic_lib\\lib", "build.cpp" );
	}

	// now build the exe that uses it
	{
		DoBuildInfoPreTest( "tests\\test_dynamic_lib\\program\\.builder\\build.build_info" );

		Array<const char*> args;
		array_add( &args, "builder.exe" );
		array_add( &args, "tests\\test_dynamic_lib\\program\\build.cpp" );

		s32 exitCode = RunProc( &args );

		TEMPER_CHECK_TRUE_M( exitCode == 0, "Exit code actually returned %d.\n", exitCode );

		DoBuildInfoPostTest( "test_dynamic_lib\\program", "build.cpp" );
	}

	// run the program to make sure everything actually works
	{
		Array<const char*> args;
		array_add( &args, "tests\\test_dynamic_lib\\program\\bin\\test_dynamic_library_program.exe" );

		s32 exitCode = RunProc( &args );

		TEMPER_CHECK_TRUE_M( exitCode == 0, "Exit code actually returned %d.\n", exitCode );

		TEMPER_CHECK_TRUE( FileExists( testDynamicLibDLLPath ) );
	}
}

int main( int argc, char** argv ) {
	core_init( MEM_KILOBYTES( 64 ), MEM_KILOBYTES( 64 ) );
	defer( core_shutdown() );

	TEMPER_RUN( argc, argv );

	return TEMPER_GET_EXIT_CODE();
}