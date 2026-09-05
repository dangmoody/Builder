#define TEMPER_IMPLEMENTATION
#include "temper/temper.h"

#define BUILDER_IMPLEMENTATION
#include "../builder.h"

#if defined( _WIN32 )
#define TEST_DEBUG_BREAK __debugbreak
#elif defined( __linux__ )
#define TEST_DEBUG_BREAK __builtin_trap
#endif

typedef enum {
	COMPILER_CLANG	= 0,
	COMPILER_CLANG_CL,
	COMPILER_GCC,
#ifdef _WIN32
	COMPILER_MSVC,
#endif

	// c++ specific compilers
	COMPILER_CLANGPP,
	COMPILER_GPP,

	COMPILER_COUNT
} compiler_t;

#ifdef _WIN32
builderMSVCInstall_t g_msvcInstall = { 0 };
#endif

static const char *Test_GetCompilerPath( const compiler_t compiler ) {
	switch ( compiler ) {
		case COMPILER_CLANG:	return "../tools/clang/bin/clang";
		case COMPILER_CLANG_CL:	return "../tools/clang/bin/clang-cl";
		case COMPILER_GCC:		return "../tools/gcc/bin/gcc";
#ifdef _WIN32
		case COMPILER_MSVC:		return g_msvcInstall.compilerPath;
#endif
		case COMPILER_CLANGPP:	return "../tools/clang/bin/clang++";
		case COMPILER_GPP:		return "../tools/gcc/bin/g++";
	}

	assert( false && "Bad compiler_t specified." );

	return NULL;
}

typedef struct {
	const char	**fileExtensionsToDelete;
	uint32_t	fileExtensionsToDeleteCount;

	const char	**foldersToDelete;
	uint32_t	foldersToDeleteCount;

	const char	**filesToExclude;
	uint32_t	filesToExcludeCount;

	// filled out by the callback
	StringList	deferredFilesToDelete;
	StringList	deferredFoldersToDelete;
} testCleanupContext_t;

static bool Test_DeleteFile( const char *filename ) {
#if defined( _WIN32 )
	if ( !DeleteFile( filename ) ) {
		Builder_Error( "Failed to delete file \"%s\": GetLastError(): 0x%X\n", filename, GetLastError() );
		return false;
	}
#elif defined( __linux__ )
	if ( remove( filename ) != 0 ) {
		int err = errno;
		Builder_Error( "Failed to delete file \"%s\": errno: %d\n", filename, err );
		return false;
	}
#endif

	return true;
}

static bool Test_DeleteFolder( const char *folder ) {
#if defined( _WIN32 )
	if ( !RemoveDirectory( folder ) ) {
		Builder_Error( "Failed to delete folder \"%s\": GetLastError(): 0x%X\n", folder, GetLastError() );
		return false;
	}
#elif defined( __linux__ )
	if ( rmdir( folder ) != 0 ) {
		int err = errno;
		Builder_Error( "Failed to delete folder \"%s\": errno: %d\n", folder, err );
		return false;
	}
#endif

	return true;
}

static void Test_OnGeneratedFilesFound( arena_t *resultsArena, fileInfo_t *fileInfo, void *data ) {
	BUILDER_ASSERT( resultsArena );
	BUILDER_ASSERT( fileInfo );
	BUILDER_ASSERT( data );

	testCleanupContext_t *context = (testCleanupContext_t *) data;

	if ( fileInfo->isDirectory ) {
		for ( int32_t folderIndex = 0; folderIndex < context->foldersToDeleteCount; folderIndex++ ) {
			const char *folderToCheck = context->foldersToDelete[folderIndex];

			if ( Builder_StringEquals( fileInfo->filename, folderToCheck ) ) {
				// printf( "Found folder \"%s\"\n", fileInfo->filename );

				const char *fullFilename = Builder_FormatString( resultsArena, "%s", fileInfo->fullFilename );
				Builder_AddStringsInternal( &context->deferredFoldersToDelete, (const char *[]) { fullFilename }, 1 );
			}
		}
	} else {
		for ( int32_t excludeFileIndex = 0; excludeFileIndex < context->filesToExcludeCount; excludeFileIndex++ ) {
			if ( Builder_StringEquals( fileInfo->filename, context->filesToExclude[excludeFileIndex] ) ) {
				return;
			}
		}

		for ( int32_t fileExtensionIndex = 0; fileExtensionIndex < context->fileExtensionsToDeleteCount; fileExtensionIndex++ ) {
			const char *fileExtensionToCheck = context->fileExtensionsToDelete[fileExtensionIndex];

			if ( Builder_PathEndsWith( fileInfo->filename, fileExtensionToCheck ) ) {
				// printf( "Found file \"%s\"\n", fileInfo->filename );

				const char *fullFilename = Builder_FormatString( resultsArena, "%s", fileInfo->fullFilename );
				Builder_AddStringsInternal( &context->deferredFilesToDelete, (const char *[]) { fullFilename }, 1 );
			}
		}
	}
}

TEMPER_TEST_PARAMETRIC( TestBuild, TEMPER_FLAG_SHOULD_RUN, const char *testFolder, const char *programFilename, const int32_t expectedExitCode ) {
	arena_t testScratch = { 0 };

	const char *buildSourceFile = Builder_FormatString( &testScratch, "%s/build.c", testFolder );
	const char *buildEXEFilename = Builder_FormatString( &testScratch, "%s/build%s", testFolder, Builder_GetFileExtensionFromBinaryType( BINARY_TYPE_EXE ) );

	arenaRewindSpot_t testScratchStart = Builder_ArenaTell( &testScratch );

	for ( int32_t compilerIndex = 0; compilerIndex < COMPILER_COUNT; compilerIndex++ ) {
		compiler_t compiler = (compiler_t) compilerIndex;

		const char *compilerName = NULL;
		switch ( compiler ) {
			case COMPILER_CLANG:	compilerName = "clang";		break;
			case COMPILER_CLANG_CL: compilerName = "clang-cl";	break;
			case COMPILER_GCC:		compilerName = "gcc";		break;
#ifdef _WIN32
			case COMPILER_MSVC:		compilerName = "msvc";		break;
#endif
			case COMPILER_CLANGPP:	compilerName = "clang++";	break;
			case COMPILER_GPP:		compilerName = "g++";		break;
		}

		TEMPER_CHECK_TRUE( compilerName );

		printf( "======= Building \"%s\" for compiler: %s =======\n", buildSourceFile, compilerName );

		Builder_RewindArena( &testScratch, &testScratchStart );

		// initial build test
		{
			char *output = NULL;

			stringBuilder_t sb = { 0 };
			StringBuilder_Appendf( &testScratch, &sb, "%s ", Test_GetCompilerPath( COMPILER_CLANG ) );
			StringBuilder_Appendf( &testScratch, &sb, "-o " );
			StringBuilder_Appendf( &testScratch, &sb, "%s ", buildEXEFilename );
			StringBuilder_Appendf( &testScratch, &sb, "%s ", buildSourceFile );
			char *buildCMDArgs = StringBuilder_ToString( &testScratch, &sb, NULL );

			printf( "Raw compile args: %s\n", buildCMDArgs );

			int32_t buildCMDExitCode = Builder_RunProcess( &testScratch, buildCMDArgs, false, &output );

			printf( "%s\n", output );

			TEMPER_CHECK_TRUE_QM( buildCMDExitCode == 0, "Failed to do initial build of %s via the raw compiler argument.\n", buildSourceFile );
		}

		// run the build EXE
		{
			char *output = NULL;

			stringBuilder_t sb = { 0 };
			StringBuilder_Appendf( &testScratch, &sb, "%s ", buildEXEFilename );
			StringBuilder_Appendf( &testScratch, &sb, "--%s ", compilerName );
			const char *buildArgs = StringBuilder_ToString( &testScratch, &sb, NULL );

			printf( "Test build.exe args: %s\n", buildArgs );

			int32_t buildEXEExitCode = Builder_RunProcess( &testScratch, buildArgs, false, &output );

			printf( "%s\n", output );

			TEMPER_CHECK_TRUE_QM( buildEXEExitCode == 0, "Failed to run \"%s\".\n", buildEXEFilename );
		}

		// run the program we just built
		{
			char *output = NULL;
			int32_t programExitCode = Builder_RunProcess( &testScratch, programFilename, false, &output );

			printf( "%s\n", output );

			TEMPER_CHECK_TRUE_M( programExitCode == expectedExitCode, "Failed to run \"%s\".\n", programFilename );
		}

		// delete all generated files and folders
		{
			testCleanupContext_t context = {
				.fileExtensionsToDeleteCount = 10,
				.fileExtensionsToDelete = (const char *[]) {
					".builder-dependencies",
					".exe",
					".dll",
					".lib",
					".pdb",
					".exp",
					".ilk",
					".so",
					".a",
					".o"
				},

				.foldersToDeleteCount = 3,
				.foldersToDelete = (const char *[]) {
					"bin",
					"intermediate",
					"visual_studio",
				},

				.filesToExcludeCount = 2,
				.filesToExclude = (const char *[]) {
					"build.exe",
					"build",
				},
			};

			bool visited = Builder_VisitFiles( &testScratch, testFolder, BUILDER_FILE_VISIT_FILES | BUILDER_FILE_VISIT_FOLDERS | BUILDER_FILE_VISIT_RECURSIVE, Test_OnGeneratedFilesFound, &context );
			TEMPER_CHECK_TRUE( visited );

			for ( builderStringChunk_t *chunk = context.deferredFilesToDelete.head; chunk; chunk = chunk->next ) {
				for ( uint32_t fileIndex = 0; fileIndex < chunk->count; fileIndex++ ) {
					const char *filename = chunk->items[fileIndex];

					bool deleted = Test_DeleteFile( filename );

					TEMPER_CHECK_TRUE_M( deleted, "Failed to delete file \"%s\".  The tests should properly clean up after themselves.\n", filename );
				}
			}

			for ( builderStringChunk_t *chunk = context.deferredFoldersToDelete.head; chunk; chunk = chunk->next ) {
				for ( uint32_t folderIndex = 0; folderIndex < chunk->count; folderIndex++ ) {
					const char *folder = chunk->items[folderIndex];

					bool deleted = Test_DeleteFolder( folder );

					TEMPER_CHECK_TRUE_M( deleted, "Failed to delete folder \"%s\".  The tests should properly clean up after themselves.\n", folder );
				}
			}
		}
	}
}

TEMPER_INVOKE_PARAMETRIC_TEST( TestBuild, "test_build_single_file",    "test_build_single_file/test_build_single_file",       0 );
TEMPER_INVOKE_PARAMETRIC_TEST( TestBuild, "test_build_multiple_files", "test_build_multiple_files/test_build_multiple_files", 0 );
TEMPER_INVOKE_PARAMETRIC_TEST( TestBuild, "test_build_static_lib",     "test_build_static_lib/test_static_lib_program",       5 );
TEMPER_INVOKE_PARAMETRIC_TEST( TestBuild, "test_build_dynamic_lib",    "test_build_dynamic_lib/test_dynamic_lib_program",     5 );
TEMPER_INVOKE_PARAMETRIC_TEST( TestBuild, "test_build_sdl3",           "test_build_sdl3/bin/sdl-demo-app",                    0 );

int main( int argc, char **argv ) {
	arena_t arena = { 0 };

#ifdef _WIN32
	Builder_GetMSVCInstall( &arena, &g_msvcInstall );
#endif

	TEMPER_RUN( argc, argv );

	int exitCode = TEMPER_GET_EXIT_CODE();

	if ( exitCode != 0 ) {
		TEST_DEBUG_BREAK();
	}

	return exitCode;
}
