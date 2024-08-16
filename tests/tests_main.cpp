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

TEMPER_TEST( Compile_Basic, TEMPER_FLAG_SHOULD_RUN ) {
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

	TEMPER_CHECK_TRUE( folder_exists( "tests\\test_basic\\.builder" ) );
	TEMPER_CHECK_TRUE( FileExists( "tests\\test_basic\\.builder\\test_basic.cpp.build_info" ) );
	TEMPER_CHECK_TRUE( FileExists( "tests\\test_basic\\.builder\\test_basic.cpp.dll" ) );
	TEMPER_CHECK_TRUE( FileExists( "tests\\test_basic\\.builder\\test_basic.cpp.exp" ) );
	TEMPER_CHECK_TRUE( FileExists( "tests\\test_basic\\.builder\\test_basic.cpp.ilk" ) );
	TEMPER_CHECK_TRUE( FileExists( "tests\\test_basic\\.builder\\test_basic.cpp.lib" ) );
	TEMPER_CHECK_TRUE( FileExists( "tests\\test_basic\\.builder\\test_basic.cpp.pdb" ) );
}

TEMPER_TEST_PARAMETRIC( Compile_SetBuilderOptions, TEMPER_FLAG_SHOULD_RUN, const char* config ) {
	const char* sourceFile = "tests\\test_set_builder_options\\test_set_builder_options.cpp";

	TEMPER_CHECK_TRUE( FileExists( sourceFile ) );

	if ( FileExists( "tests\\test_set_builder_options\\.builder\\test_set_builder_options.cpp.build_info" ) ) {
		file_delete( "tests\\test_set_builder_options\\.builder\\test_set_builder_options.cpp.build_info" );
	}

	TEMPER_CHECK_TRUE( !FileExists( "tests\\test_set_builder_options\\.builder\\test_set_builder_options.cpp.build_info" ) );

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

	TEMPER_CHECK_TRUE( folder_exists( "tests\\test_set_builder_options\\.builder" ) );
	TEMPER_CHECK_TRUE( FileExists( "tests\\test_set_builder_options\\.builder\\test_set_builder_options.cpp.build_info" ) );
	TEMPER_CHECK_TRUE( FileExists( "tests\\test_set_builder_options\\.builder\\test_set_builder_options.cpp.dll" ) );
	TEMPER_CHECK_TRUE( FileExists( "tests\\test_set_builder_options\\.builder\\test_set_builder_options.cpp.exp" ) );
	TEMPER_CHECK_TRUE( FileExists( "tests\\test_set_builder_options\\.builder\\test_set_builder_options.cpp.ilk" ) );
	TEMPER_CHECK_TRUE( FileExists( "tests\\test_set_builder_options\\.builder\\test_set_builder_options.cpp.lib" ) );
	TEMPER_CHECK_TRUE( FileExists( "tests\\test_set_builder_options\\.builder\\test_set_builder_options.cpp.pdb" ) );
}

TEMPER_INVOKE_PARAMETRIC_TEST( Compile_SetBuilderOptions, "release" );
TEMPER_INVOKE_PARAMETRIC_TEST( Compile_SetBuilderOptions, "debug" );

int main( int argc, char** argv ) {
	core_init( MEM_KILOBYTES( 64 ), MEM_KILOBYTES( 64 ) );
	defer( core_shutdown() );

	TEMPER_RUN( argc, argv );

	return TEMPER_GET_EXIT_CODE();
}